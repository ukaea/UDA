// Tests for xdr_authentication_block — the wire decoder for the bearer token.
//
// This is the first thing an unauthenticated peer reaches, so it is worth testing on its
// own: it allocates based on a length field that peer controls, and on the server it runs
// once per request rather than once per connection.

#include <catch2/catch_test_macros.hpp>

#include <clientserver/udaStructs.h>
#include <clientserver/xdrlib.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef __APPLE__
#  include <rpc/xdr.h>
#else
#  include <rpc/types.h>
#  include <rpc/xdr.h>
#endif

namespace {

// Mirrors the hard ceiling in xdrlib.cpp. Kept as a literal deliberately: if the limit in
// the implementation changes, this test should fail and make someone think about it.
constexpr unsigned int MAX_AUTH_PAYLOAD_LENGTH = 16384;

// Encode a block into a caller-owned buffer and return the bytes used.
unsigned int encode(const AUTHENTICATION_BLOCK& in, std::vector<char>& buffer)
{
    XDR xdrs;
    xdrmem_create(&xdrs, buffer.data(), static_cast<unsigned int>(buffer.size()), XDR_ENCODE);
    auto* mutable_in = const_cast<AUTHENTICATION_BLOCK*>(&in);
    const bool ok = xdr_authentication_block(&xdrs, mutable_in) != 0;
    const unsigned int used = xdr_getpos(&xdrs);
    xdr_destroy(&xdrs);
    REQUIRE(ok);
    return used;
}

bool decode_into(const std::vector<char>& buffer, unsigned int len, AUTHENTICATION_BLOCK& out)
{
    XDR xdrs;
    xdrmem_create(&xdrs, const_cast<char*>(buffer.data()), len, XDR_DECODE);
    const bool ok = xdr_authentication_block(&xdrs, &out) != 0;
    xdr_destroy(&xdrs);
    return ok;
}

AUTHENTICATION_BLOCK make_block(const std::string& token, unsigned int type = 1)
{
    AUTHENTICATION_BLOCK b = {};
    b.authentication_type = type;
    b.payload_length      = static_cast<unsigned int>(token.size());
    b.payload             = reinterpret_cast<unsigned char*>(strdup(token.c_str()));
    return b;
}

} // namespace

TEST_CASE("an authentication block round-trips", "[xdr_auth]")
{
    const std::string token = "header.payload.signature";
    AUTHENTICATION_BLOCK in = make_block(token);

    std::vector<char> buffer(4096);
    const unsigned int used = encode(in, buffer);

    AUTHENTICATION_BLOCK out = {};
    REQUIRE(decode_into(buffer, used, out));

    REQUIRE(out.authentication_type == in.authentication_type);
    REQUIRE(out.payload_length == token.size());
    REQUIRE(out.payload != nullptr);
    REQUIRE(std::string(reinterpret_cast<char*>(out.payload), out.payload_length) == token);

    free(in.payload);
    free(out.payload);
}

TEST_CASE("the decoded payload is null-terminated", "[xdr_auth]")
{
    // The decoder allocates payload_length + 1 and zeroes it, so callers that treat the
    // payload as a C string do not run off the end.
    const std::string token = "abc";
    AUTHENTICATION_BLOCK in = make_block(token);

    std::vector<char> buffer(4096);
    const unsigned int used = encode(in, buffer);

    AUTHENTICATION_BLOCK out = {};
    REQUIRE(decode_into(buffer, used, out));
    REQUIRE(out.payload[out.payload_length] == '\0');

    free(in.payload);
    free(out.payload);
}

TEST_CASE("an oversized payload_length is rejected before allocating", "[xdr_auth]")
{
    // A hostile peer controls this field. The decoder must refuse rather than attempt the
    // allocation, so the cap is enforced ahead of any read of the payload itself.
    std::vector<char> buffer(64);
    XDR xdrs;
    xdrmem_create(&xdrs, buffer.data(), static_cast<unsigned int>(buffer.size()), XDR_ENCODE);
    unsigned int type = 1;
    unsigned int huge = MAX_AUTH_PAYLOAD_LENGTH + 1;
    REQUIRE(xdr_u_int(&xdrs, &type) != 0);
    REQUIRE(xdr_u_int(&xdrs, &huge) != 0);
    const unsigned int used = xdr_getpos(&xdrs);
    xdr_destroy(&xdrs);

    AUTHENTICATION_BLOCK out = {};
    REQUIRE_FALSE(decode_into(buffer, used, out));
    REQUIRE(out.payload == nullptr);
}

TEST_CASE("a payload exactly at the limit is accepted", "[xdr_auth]")
{
    // The boundary belongs to the valid side: the check rejects strictly greater.
    const std::string token(MAX_AUTH_PAYLOAD_LENGTH, 'x');
    AUTHENTICATION_BLOCK in = make_block(token);

    // XDR encodes each char of the vector as its own 4-byte unit, so the wire form of a
    // payload is roughly four times its length. The 16 KB cap therefore bounds the
    // allocation at 16 KB but the transfer at about 64 KB — worth knowing when sizing
    // buffers or reasoning about what the cap actually limits.
    std::vector<char> buffer(4 * MAX_AUTH_PAYLOAD_LENGTH + 4096);
    const unsigned int used = encode(in, buffer);

    AUTHENTICATION_BLOCK out = {};
    REQUIRE(decode_into(buffer, used, out));
    REQUIRE(out.payload_length == MAX_AUTH_PAYLOAD_LENGTH);

    free(in.payload);
    free(out.payload);
}

TEST_CASE("decoding twice into the same block does not orphan the first payload",
          "[xdr_auth][leak]")
{
    // The server re-receives the client block, bearer token included, on every request,
    // so this decoder runs repeatedly against the same struct. It must release the
    // previous buffer rather than overwrite the pointer: overwriting leaked up to
    // MAX_AUTH_PAYLOAD_LENGTH per request for the life of a connection.
    //
    // A unit test cannot observe the leak directly. What it can pin down is the ownership
    // contract that makes the leak impossible: after a second decode the block holds the
    // second payload, and freeing it once is correct and sufficient.
    const std::string first  = "first.token.value";
    const std::string second = "second.token.value.longer";

    AUTHENTICATION_BLOCK in_a = make_block(first);
    AUTHENTICATION_BLOCK in_b = make_block(second);

    std::vector<char> buf_a(4096), buf_b(4096);
    const unsigned int used_a = encode(in_a, buf_a);
    const unsigned int used_b = encode(in_b, buf_b);

    AUTHENTICATION_BLOCK out = {};
    REQUIRE(decode_into(buf_a, used_a, out));
    REQUIRE(std::string(reinterpret_cast<char*>(out.payload), out.payload_length) == first);

    REQUIRE(decode_into(buf_b, used_b, out));
    REQUIRE(out.payload_length == second.size());
    REQUIRE(std::string(reinterpret_cast<char*>(out.payload), out.payload_length) == second);

    free(in_a.payload);
    free(in_b.payload);
    free(out.payload); // exactly one free is correct: the decoder owns and replaces
}

TEST_CASE("a zero-length payload clears a previously decoded one", "[xdr_auth][leak]")
{
    // The same ownership rule on the other branch: a client that stops sending a token
    // must not leave the previous one attached to the block.
    AUTHENTICATION_BLOCK in_a = make_block("a.token.value");
    AUTHENTICATION_BLOCK in_empty = {};
    in_empty.authentication_type = 0;
    in_empty.payload_length = 0;
    in_empty.payload = nullptr;

    std::vector<char> buf_a(4096), buf_empty(4096);
    const unsigned int used_a = encode(in_a, buf_a);
    const unsigned int used_empty = encode(in_empty, buf_empty);

    AUTHENTICATION_BLOCK out = {};
    REQUIRE(decode_into(buf_a, used_a, out));
    REQUIRE(out.payload != nullptr);

    REQUIRE(decode_into(buf_empty, used_empty, out));
    REQUIRE(out.payload_length == 0);
    REQUIRE(out.payload == nullptr);

    free(in_a.payload);
}

TEST_CASE("discard mode consumes the payload without allocating it", "[xdr_auth][session]")
{
    // After the handshake the server has no use for the token — the session runs on the
    // claims verified once, at the handshake. But the client still sends the block with
    // every request, because the wire format is fixed by the protocol version. So the
    // bytes must still be read, or the XDR stream desynchronises and every subsequent
    // field is garbage. Discard mode reads them and drops them.
    const std::string token = "header.payload.signature";
    AUTHENTICATION_BLOCK in = make_block(token);

    std::vector<char> buffer(4096);
    const unsigned int used = encode(in, buffer);

    AUTHENTICATION_BLOCK out = {};

    udaDiscardAuthenticationPayload(true);
    REQUIRE(decode_into(buffer, used, out));
    udaDiscardAuthenticationPayload(false);

    // The header fields are still decoded — only the payload is dropped.
    REQUIRE(out.authentication_type == in.authentication_type);
    REQUIRE(out.payload_length == token.size());

    // Nothing was allocated, and nothing is left for a caller to mistake for a live token.
    REQUIRE(out.payload == nullptr);

    free(in.payload);
}

TEST_CASE("discard mode leaves the stream positioned correctly", "[xdr_auth][session]")
{
    // The real risk of dropping a field is reading the wrong number of bytes. Encode an
    // auth block followed by a sentinel, discard the block, and check the sentinel is
    // still exactly where it should be.
    const std::string token = "a.reasonably.long.token.value.for.this.purpose";
    AUTHENTICATION_BLOCK in = make_block(token);
    const unsigned int sentinel_value = 0xABCDEF01;

    std::vector<char> buffer(8192);
    XDR xdrs;
    xdrmem_create(&xdrs, buffer.data(), static_cast<unsigned int>(buffer.size()), XDR_ENCODE);
    REQUIRE(xdr_authentication_block(&xdrs, &in) != 0);
    unsigned int sentinel_out = sentinel_value;
    REQUIRE(xdr_u_int(&xdrs, &sentinel_out) != 0);
    const unsigned int used = xdr_getpos(&xdrs);
    xdr_destroy(&xdrs);

    AUTHENTICATION_BLOCK out = {};
    unsigned int sentinel_in = 0;

    udaDiscardAuthenticationPayload(true);
    xdrmem_create(&xdrs, buffer.data(), used, XDR_DECODE);
    REQUIRE(xdr_authentication_block(&xdrs, &out) != 0);
    REQUIRE(xdr_u_int(&xdrs, &sentinel_in) != 0);
    xdr_destroy(&xdrs);
    udaDiscardAuthenticationPayload(false);

    REQUIRE(sentinel_in == sentinel_value);
    REQUIRE(out.payload == nullptr);

    free(in.payload);
}

TEST_CASE("discard mode releases a payload decoded before it was enabled", "[xdr_auth][session]")
{
    // The handshake decodes the token for real; discard mode is switched on afterwards.
    // The first request to arrive in discard mode must not leave the handshake's buffer
    // attached to the block.
    AUTHENTICATION_BLOCK in = make_block("first.real.token");
    std::vector<char> buffer(4096);
    const unsigned int used = encode(in, buffer);

    AUTHENTICATION_BLOCK out = {};
    REQUIRE(decode_into(buffer, used, out));
    REQUIRE(out.payload != nullptr);

    udaDiscardAuthenticationPayload(true);
    REQUIRE(decode_into(buffer, used, out));
    udaDiscardAuthenticationPayload(false);

    REQUIRE(out.payload == nullptr);

    free(in.payload);
}
