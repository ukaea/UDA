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

#include <clientserver/stringUtils.h>
#include "udaTypes.h"
#include "clientserver/errorLog.h"
#include "logging.h"

#if defined(SERVERBUILD) || defined(FATCLIENT)
#  include <sstream>
#  include <boost/format.hpp>
#endif

unsigned int countDataBlockListSize(const DATA_BLOCK_LIST* data_block_list, CLIENT_BLOCK* client_block)
{
    unsigned int total = 0;
    for (int i = 0; i < data_block_list->count; ++i) {
        total += countDataBlockSize(&data_block_list->data[i], client_block);
    }
    return total;
}

unsigned int countDataBlockSize(const DATA_BLOCK* data_block, CLIENT_BLOCK* client_block)
{
    int factor;
    DIMS dim;
    unsigned int count = sizeof(DATA_BLOCK);

    count += (unsigned int)(getSizeOf((UDA_TYPE)data_block->data_type) * data_block->data_n);

    if (data_block->error_type != UDA_TYPE_UNKNOWN) {
        count += (unsigned int)(getSizeOf((UDA_TYPE)data_block->error_type) * data_block->data_n);
    }
    if (data_block->errasymmetry) {
        count += (unsigned int)(getSizeOf((UDA_TYPE)data_block->error_type) * data_block->data_n);
    }

    if (data_block->rank > 0) {
        for (unsigned int k = 0; k < data_block->rank; k++) {
            count += sizeof(DIMS);
            dim = data_block->dims[k];
            if (!dim.compressed) {
                count += (unsigned int)(getSizeOf((UDA_TYPE)dim.data_type) * dim.dim_n);
                factor = 1;
                if (dim.errasymmetry) factor = 2;
                if (dim.error_type != UDA_TYPE_UNKNOWN) {
                    count += (unsigned int)(factor * getSizeOf((UDA_TYPE)dim.error_type) * dim.dim_n);
                }
            } else {;
                switch (dim.method) {
                    case 0:
                        count += +2 * sizeof(double);
                        break;
                    case 1:
                        for (unsigned int i = 0; i < dim.udoms; i++) {
                            count += (unsigned int)(*((long*)dim.sams + i) * getSizeOf((UDA_TYPE)dim.data_type));
                        }
                        break;
                    case 2:
                        count += dim.udoms * getSizeOf((UDA_TYPE)dim.data_type);
                        break;
                    case 3:
                        count += dim.udoms * getSizeOf((UDA_TYPE)dim.data_type);
                        break;
                }
            }
        }
    }

    if (client_block->get_meta) {
        count += sizeof(DATA_SYSTEM) + sizeof(SYSTEM_CONFIG) + sizeof(DATA_SOURCE) + sizeof(SIGNAL) +
                 sizeof(SIGNAL_DESC);
    }

    return count;
}

#if defined(SERVERBUILD) || defined(FATCLIENT)

void udaAccessLog(int init, CLIENT_BLOCK client_block, REQUEST_BLOCK request_block, SERVER_BLOCK server_block,
                  unsigned int total_datablock_size)
{
    int err = 0;

#ifndef FATCLIENT
    int socket;
    socklen_t addrlen;
#endif
#ifndef IPV6PROTOCOL
#  ifndef FATCLIENT
    struct sockaddr_in addr = {};
#  endif
    static char host[INET6_ADDRSTRLEN + 1];
#else
#  ifndef FATCLIENT
    struct sockaddr_in6 addr;
#  endif
    static char host[INET6_ADDRSTRLEN+1];
#endif
    static struct timeval et_start;
    static struct timeval et_end;
    static char accessdate[UDA_DATE_LENGTH];     // The Calendar Time as a formatted String

    errno = 0;

    if (init) {
        // Start of Access Log Record - at start of the request

        // Remote Host IP Address

#ifndef FATCLIENT
        socket = 0;
        memset(&addr, 0, sizeof(addr));
        addrlen = sizeof(addr);
#ifndef IPV6PROTOCOL
        addr.sin_family = AF_INET;
        if ((getpeername(socket, (struct sockaddr*)&addr, &addrlen)) == -1) {        // Socket Address
            strcpy(host, "-");
        } else {
            if (addrlen <= HOSTNAMELENGTH - 1) {
                strncpy(host, inet_ntoa(addr.sin_addr), addrlen);
                host[addrlen] = '\0';
            } else {
                strncpy(host, inet_ntoa(addr.sin_addr), HOSTNAMELENGTH - 1);
                host[HOSTNAMELENGTH - 1] = '\0';
            }
#endif
            convertNonPrintable2(host);
            TrimString(host);
            if (strlen(host) == 0) strcpy(host, "-");
        }
#else
        strcpy(host, "-");
#endif

        // Client's Userid: from the client_block structure

        // Request Start Time
        gettimeofday(&et_start, nullptr);

        // Calendar Time

        time_t calendar;
        time(&calendar);
        struct tm* broken = gmtime(&calendar);
#ifndef _WIN32
        asctime_r(broken, accessdate);
#else
        asctime_s(accessdate, DATELENGTH, broken);
#endif

        convertNonPrintable2(accessdate);
        TrimString(accessdate);

        // Client Request: From the request_block structure

        return;
    }

    // Write the Log Record

    // Error Code & Message

    const char* msg;
    if (server_block.idamerrorstack.nerrors > 0) {
        err = server_block.idamerrorstack.idamerror[0].code;
        msg = server_block.idamerrorstack.idamerror[0].msg;
    } else {
        err = 0;
        msg = "";
    }

    // Error Message: from data_block

    // Amount of Data Returned (Excluding Meta Data) from global: totalDataBlockSize;

    // Request Completed Time: Elasped & CPU

    gettimeofday(&et_end, nullptr);
    auto elapsedtime = (double)((et_end.tv_sec - et_start.tv_sec) * 1000);    // millisecs

    if (et_end.tv_usec < et_start.tv_usec) {
        elapsedtime = elapsedtime - 1.0 + (double)(1000000 + et_end.tv_usec - et_start.tv_usec) / 1000.0;
    } else {
        elapsedtime = elapsedtime + (double)(et_end.tv_usec - et_start.tv_usec) / 1000.0;
    }

    // Write the Log Record & Flush the fd

    for (int i = 0; i < request_block.num_requests; ++i) {
        auto request = request_block.requests[i];
        std::stringstream ss;
        ss << host << " - "
           << client_block.uid << " "
           << "[" << accessdate << "] "
           << "[" << request.request << " "
            << "";

        auto fmt = boost::format("%1% - %2% [%3%] [%4% %5% %6% %7% %8% %9% %10% %11% %12% %13% %14%] %15% %16% [%17%] %18% %19% %20% [%21% %22%] [%23%]")
            % host                  // 1
            % client_block.uid      // 2
            % accessdate            // 3
            % request.request       // 4
            % request.signal        // 5
            % request.exp_number    // 6
            % request.pass          // 7
            % request.tpass         // 8
            % request.path          // 9
            % request.file          // 10
            % request.format        // 11
            % request.archive       // 12
            % request.device_name   // 13
            % request.server        // 14
            % err                   // 15
            % total_datablock_size  // 16
            % msg                   // 17
            % elapsedtime           // 18
            % client_block.version  // 19
            % server_block.version  // 20
            % client_block.pid      // 21
            % server_block.pid      // 22
            % client_block.DOI;     // 23
        auto str = fmt.str();

        udaLog(UDA_LOG_ACCESS, "%s\n", str.c_str());

//        udaServerRedirectStdStreams(0);
//        udaProvenancePlugin(&client_block, &request, nullptr, nullptr, pluginlist, str.c_str(), environment);
//        udaServerRedirectStdStreams(1);
    }

}

#endif // defined(SERVERBUILD) || defined(FATCLIENT)
