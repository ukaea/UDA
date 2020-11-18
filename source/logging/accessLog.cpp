/*---------------------------------------------------------------
* Server Access Log
*
* Log Format: Conforms to the Common Log Format for the first 6 fields
*
*		client address, client userid, date, client request,
*		error code, data bytes returned
* plus:		error message, elapsed time, client version, server version
*		client process id
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
#include <clientserver/udaTypes.h>
#if defined(SERVERBUILD) || defined(FATCLIENT)
#  include <server/serverPlugin.h>
#  include <server/udaServer.h>
#endif

unsigned int countDataBlockSize(DATA_BLOCK* data_block, CLIENT_BLOCK* client_block)
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

void udaAccessLog(int init, CLIENT_BLOCK client_block, REQUEST_BLOCK request, SERVER_BLOCK server_block,
                  const PLUGINLIST* pluginlist, const ENVIRONMENT* environment)
{
    int err = 0;

#ifndef FATCLIENT
    int socket;
    socklen_t addrlen;
#endif
#ifndef IPV6PROTOCOL
#  ifndef FATCLIENT
    struct sockaddr_in addr;
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
    static char accessdate[DATELENGTH];     // The Calendar Time as a formatted String

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
    double elapsedtime = (double)((et_end.tv_sec - et_start.tv_sec) * 1000);    // millisecs

    if (et_end.tv_usec < et_start.tv_usec) {
        elapsedtime = elapsedtime - 1.0 + (double)(1000000 + et_end.tv_usec - et_start.tv_usec) / 1000.0;
    } else {
        elapsedtime = elapsedtime + (double)(et_end.tv_usec - et_start.tv_usec) / 1000.0;
    }

    // Write the Log Record & Flush the fd

    size_t wlen = strlen(host) + 1 +
                  strlen(client_block.uid) + 1 +
                  strlen(accessdate) + 1 +
                  strlen(request.signal) + 1 +
                  strlen(request.tpass) + 1 +
                  strlen(request.path) + 1 +
                  strlen(request.file) + 1 +
                  strlen(request.format) + 1 +
                  strlen(request.archive) + 1 +
                  strlen(request.device_name) + 1 +
                  strlen(request.server) + 1 +
                  strlen(msg) + 1 +
                  strlen(client_block.DOI) + 1 +
                  1024;

    if (wlen < MAXMETA) {
        char* work = (char*)malloc(MAXMETA * sizeof(char));

        sprintf(work, "%s - %s [%s] [%d %s %d %d %s %s %s %s %s %s %s] %d %d [%s] %f %d %d [%d %d] [%s]",
                host, client_block.uid, accessdate, static_cast<int>(request.request), request.signal,
                request.exp_number, request.pass, request.tpass, request.path, request.file, request.format,
                request.archive, request.device_name, request.server, err, (int)totalDataBlockSize, msg,
                elapsedtime, client_block.version, server_block.version, client_block.pid, server_block.pid,
                client_block.DOI);

        udaLog(UDA_LOG_ACCESS, "%s\n", work);

        // Save Provenance with socket stream protection

        udaServerRedirectStdStreams(0);
        udaProvenancePlugin(&client_block, &request, nullptr, nullptr, pluginlist, work, environment);
        udaServerRedirectStdStreams(1);

        free(work);

    } else {
        udaLog(UDA_LOG_ACCESS, "%s - %s [%s] [%d %s %d %d %s %s %s %s %s %s %s] %d %d [%s] %f %d %d [%d %d] [%s]\n",
                host, client_block.uid, accessdate, request.request, request.signal, request.exp_number,
                request.pass, request.tpass, request.path, request.file, request.format, request.archive,
                request.device_name, request.server, err, (int)totalDataBlockSize, msg,
                elapsedtime, client_block.version, server_block.version, client_block.pid, server_block.pid,
                client_block.DOI);
    }

}

#endif // defined(SERVERBUILD) || defined(FATCLIENT)
