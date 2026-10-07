// uda_connect_check — make one real UDA request and report exactly how it ended.
//
// The offline tests and uda_token_check both stop short of the wire: they verify tokens,
// not connections. This drives the whole path — TCP, optional TLS, the protocol-11
// handshake with the authentication block, the server-side OIDC gate, and a plugin call —
// and reports the error code the client actually receives, which is the thing an operator
// sees and the thing the error-code table in docs/authentication.md promises.
//
//   uda_connect_check [--host H] [--port P] [--request "HELP::ping()"]
//                     [--repeat N] [--interval S] [--client-version N] [--quiet]
//
// --client-version makes the client announce an older protocol version. The wire format
// of the client block is driven entirely by that number — the authentication block is
// only serialised when both ends are at protocol 11 or above — so this produces a
// genuinely protocol-N client rather than an imitation of one. It is how the
// old-client-against-new-server compatibility row is tested without keeping an old build
// around.
//
// --repeat issues N requests on the one connection, --interval sleeps between them. The
// two together hold a connection open across a token's expiry, which is how the session
// lifetime rule is checked: the connection must stop serving once the token it was
// established with has expired. That is the path where the client
// block — bearer token included — is re-sent and re-decoded per request, so it is what
// exercises the per-request decode rather than just the handshake.
//
// The token is taken from UDA_AUTH_TOKEN, exactly as a real client does.
//
// Exit status:
//   0  request succeeded
//   2  server refused it   (the UDA error code and message are printed)
//   3  usage or local error

#include <client/accAPI.h>
#include <client/udaClient.h>
#include <client/udaGetAPI.h>

#include <cstdlib>
#include <cstring>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

// Defined in the client library; the version the client announces in the client block.
extern int client_version;

int main(int argc, char** argv)
{
    std::string host    = "localhost";
    std::string request = "HELP::ping()";
    int         port    = 56565;
    int         repeat  = 1;
    int         announce_version = 0; // 0 = leave the library default
    int         interval = 0;         // seconds to wait between repeated requests
    bool        quiet   = false;

    if (const char* h = std::getenv("UDA_HOST")) { host = h; }
    if (const char* p = std::getenv("UDA_PORT")) { port = std::atoi(p); }

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::cerr << "missing value for " << a << "\n"; std::exit(3); }
            return argv[++i];
        };
        if      (a == "--host")    host    = next();
        else if (a == "--port")    port    = std::atoi(next().c_str());
        else if (a == "--request") request = next();
        else if (a == "--repeat")  repeat  = std::atoi(next().c_str());
        else if (a == "--client-version") announce_version = std::atoi(next().c_str());
        else if (a == "--interval") interval = std::atoi(next().c_str());
        else if (a == "--quiet")   quiet   = true;
        else {
            std::cerr << "usage: " << argv[0]
                      << " [--host H] [--port P] [--request R] [--repeat N]"
                         " [--interval S] [--client-version N] [--quiet]\n";
            return 3;
        }
    }

    if (announce_version > 0) {
        client_version = announce_version;
    }

    putIdamServerHost(host.c_str());
    putIdamServerPort(port);

    int handle = -1;
    for (int n = 0; n < (repeat > 0 ? repeat : 1); ++n) {
        if (n > 0 && interval > 0) {
            if (!quiet) {
                std::cout << "  waiting " << interval << "s before request " << (n + 1)
                          << "..." << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds(interval));
        }
        handle = idamGetAPI(request.c_str(), "");
        if (handle < 0) {
            break;
        }
        if (getIdamErrorCode(handle) != 0) {
            break;
        }
    }

    // Two places carry the outcome: the handle's own error code, and the server's error
    // stack. An authentication refusal arrives in the server block, so it shows up in the
    // stack even when the handle reports a generic failure — report both.
    int         code = 0;
    std::string message;

    if (handle < 0) {
        code = handle;
    } else {
        code = getIdamErrorCode(handle);
        const char* msg = getIdamErrorMsg(handle);
        if (msg != nullptr) { message = msg; }
    }

    const int stack_size = getIdamServerErrorStackSize();
    for (int i = 0; i < stack_size; ++i) {
        const int rec = getIdamServerErrorStackRecordCode(i);
        if (rec != 0) {
            code = rec; // the server's own code is the authoritative one
            const char* m = getIdamServerErrorStackRecordMsg(i);
            if (m != nullptr && *m != '\0') { message = m; }
            break;
        }
    }

    if (code == 0 && handle >= 0) {
        if (!quiet) {
            std::cout << "OK request=" << request;
            if (repeat > 1) { std::cout << " repeat=" << repeat; }
            std::cout << "\n";
        }
        return 0;
    }

    if (!quiet) {
        std::cout << "REFUSED code=" << code << " message=" << message << "\n";
    }
    return 2;
}
