/*---------------------------------------------------------------
 * Server Access Log
 *
 * Log Format: Conforms to the Common Log Format for the first 6 fields
 *
 *        client address, client userid, date, client request,
 *        error code, data bytes returned
 * plus:        error message, elapsed time, client version, server version
 *        client process id
 *
 *--------------------------------------------------------------*/

#include "accessLog.h"

#include <cerrno>
#include <cstdlib>
#ifndef _WIN32
#  include <arpa/inet.h>
#else
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

#include <sstream>
#include <fmt/format.h>

#include "clientserver/error_log.h"
#include "common/string_utils.h"
#include "logging.h"

constexpr size_t HostNameLength = 20;

using namespace uda::client_server;
using namespace uda::common;

void uda::logging::uda_access_log(int init, ClientBlock client_block, RequestBlock request_block,
                                ServerBlock server_block, unsigned int total_datablock_size)
{
    int err = 0;

#  ifndef FATCLIENT
    int socket;
    socklen_t addrlen;
#  endif
#  ifndef IPV6PROTOCOL
#    ifndef FATCLIENT
    struct sockaddr_in addr = {};
#    endif
    static char host[INET6_ADDRSTRLEN + 1];
#  else
#    ifndef FATCLIENT
    struct sockaddr_in6 addr;
#    endif
    static char host[INET6_ADDRSTRLEN + 1];
#  endif
    static struct timeval et_start;
    static struct timeval et_end;
    static char access_date[DateLength]; // The Calendar Time as a formatted String

    errno = 0;

    if (init) {
        // Start of Access Log Record - at start of the request

        // Remote Host IP Address

#  ifndef FATCLIENT
        socket = 0;
        memset(&addr, 0, sizeof(addr));
        addrlen = sizeof(addr);
#    ifndef IPV6PROTOCOL
        addr.sin_family = AF_INET;
        if (getpeername(socket, reinterpret_cast<sockaddr*>(&addr), &addrlen) == -1) { // Socket Address
            strcpy(host, "-");
        } else {
            if (addrlen <= HostNameLength - 1) {
                strncpy(host, inet_ntoa(addr.sin_addr), addrlen);
                host[addrlen] = '\0';
            } else {
                strncpy(host, inet_ntoa(addr.sin_addr), HostNameLength - 1);
                host[HostNameLength - 1] = '\0';
            }
#    endif
            convert_non_printable2(host);
            trim_string(host);
            if (strlen(host) == 0) {
                strcpy(host, "-");
            }
        }
#  else
        strcpy(host, "-");
#  endif

        // Client's Userid: from the client_block structure

        // Request Start Time
        gettimeofday(&et_start, nullptr);

        // Calendar Time

        time_t calendar;
        time(&calendar);
        struct tm* broken = gmtime(&calendar);
#  ifndef _WIN32
        asctime_r(broken, access_date);
#  else
        asctime_s(accessdate, DATELENGTH, broken);
#  endif

        convert_non_printable2(access_date);
        trim_string(access_date);

        // Client Request: From the request_block structure

        return;
    }

    // Write the Log Record

    // Error Code & Message

    const char* msg;
    if (!server_block.error_stack.empty()) {
        err = server_block.error_stack[0].code;
        msg = server_block.error_stack[0].msg;
    } else {
        err = 0;
        msg = "";
    }

    // Error Message: from data_block

    // Amount of Data Returned (Excluding Metadata) from global: totalDataBlockSize;

    // Request Completed Time: Elapsed & CPU

    gettimeofday(&et_end, nullptr);
    auto elapsed_time = static_cast<double>((et_end.tv_sec - et_start.tv_sec) * 1000); // milliseconds

    if (et_end.tv_usec < et_start.tv_usec) {
        elapsed_time = elapsed_time - 1.0 + static_cast<double>(1000000 + et_end.tv_usec - et_start.tv_usec) / 1000.0;
    } else {
        elapsed_time = elapsed_time + static_cast<double>(et_end.tv_usec - et_start.tv_usec) / 1000.0;
    }

    // Write the Log Record & Flush the fd

    for (size_t i = 0; i < request_block.size(); ++i) {
        auto request = request_block[i];

        UDA_LOG(UDA_LOG_ACCESS, "{} - {} [{}] [{} {} {} {} {} {} {} {} {} {} {}] {} {} [{}] {} {} {} [{} {}] [{}]",
                host                   // 1
                , client_block.uid     // 2
                , access_date           // 3
                , request.request      // 4
                , request.signal       // 5
                , request.exp_number   // 6
                , request.pass         // 7
                , request.tpass        // 8
                , request.path         // 9
                , request.file         // 10
                , request.format       // 11
                , request.archive      // 12
                , request.device_name  // 13
                , request.server       // 14
                , err                  // 15
                , total_datablock_size // 16
                , msg                  // 17
                , elapsed_time          // 18
                , client_block.version // 19
                , server_block.version // 20
                , client_block.pid     // 21
                , server_block.pid     // 22
                , client_block.DOI     // 23
                )
    }
}
