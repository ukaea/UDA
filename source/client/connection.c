// Create a Socket Connection to the IDAM server with a randomised time delay between connection attempts
//
//----------------------------------------------------------------
#ifdef _WIN32
#  include <winsock.h> // must be included before connection.h to avoid macro redefinition in rpc/types.h
#endif

#include "connection.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#ifdef __GNUC__
#  ifndef _WIN32
#    include <netdb.h>
#    include <netinet/tcp.h>
#  endif
#  include <unistd.h>
#  include <strings.h>
#else
#  include <process.h>
#  define strcasecmp _stricmp
#  define sleep(DELAY) Sleep((DWORD)((DELAY)*1E3))
#  define close(SOCK) closesocket(SOCK)
#  pragma comment(lib, "Ws2_32.lib")
#endif

#include <clientserver/errorLog.h>
#include <clientserver/manageSockets.h>
#include <client/udaClient.h>
#include <client/udaClientHostList.h>
#include <logging/logging.h>

#include "updateSelectParms.h"
#include "getEnvironment.h"

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include <authentication/udaSSL.h>
#endif

static int clientSocket = -1;
static SOCKETLIST client_socketlist;   // List of open sockets

int connectionOpen()
{
    return clientSocket != -1;
}

int reconnect(ENVIRONMENT* environment)
{
    int socketId = -1;

    int err = 0;

    // Save current client and server timer settings, Socket and XDR handles

    time_t tv_server_start0 = tv_server_start;
    int user_timeout0 = user_timeout;
    int clientSocket0 = clientSocket;
    XDR* clientInput0 = clientInput;
    XDR* clientOutput0 = clientOutput;

    // Identify the current Socket connection in the Socket List

    socketId = getSocketRecordId(&client_socketlist, clientSocket);

    // Instance a new server if the Client has changed the host and/or port number

    if (environment->server_reconnect) {
        time(&tv_server_start);         // Start a New Server AGE timer
        clientSocket = -1;              // Flags no Socket is open
        environment->server_change_socket = 0;   // Client doesn't know the Socket ID so disable
    }

    // Client manages connections through the Socket id and specifies which running server to connect to

    if (environment->server_change_socket) {
        if ((socketId = getSocketRecordId(&client_socketlist, environment->server_socket)) < 0) {
            err = NO_SOCKET_CONNECTION;
            addIdamError(CODEERRORTYPE, __func__, err, "The User Specified Socket Connection does not exist");
            return err;
        }

        // replace with previous timer settings and XDR handles

        tv_server_start = client_socketlist.sockets[socketId].tv_server_start;
        user_timeout = client_socketlist.sockets[socketId].user_timeout;
        clientSocket = client_socketlist.sockets[socketId].fh;
        clientInput = client_socketlist.sockets[socketId].Input;
        clientOutput = client_socketlist.sockets[socketId].Output;

        environment->server_change_socket = 0;
        environment->server_socket = client_socketlist.sockets[socketId].fh;
        environment->server_port = client_socketlist.sockets[socketId].port;
        strcpy(environment->server_host, client_socketlist.sockets[socketId].host);
    }

    // save Previous data if a previous socket existed

    if (socketId >= 0) {
        client_socketlist.sockets[socketId].tv_server_start = tv_server_start0;
        client_socketlist.sockets[socketId].user_timeout = user_timeout0;
        client_socketlist.sockets[socketId].fh = clientSocket0;
        client_socketlist.sockets[socketId].Input = clientInput0;
        client_socketlist.sockets[socketId].Output = clientOutput0;
    }

    return err;
}

int createConnection()
{
    int window_size = DB_READ_BLOCK_SIZE;        // 128K
    int rc;

    static int max_socket_delay = -1;
    static int max_socket_attempts = -1;

    if (max_socket_delay < 0) {
        char* env = getenv("UDA_MAX_SOCKET_DELAY");
        if (env == NULL) {
            max_socket_delay = 10;
        } else {
            max_socket_delay = (int)strtol(env, NULL, 10);
        }
    }

    if (max_socket_attempts < 0) {
        char* env = getenv("UDA_MAX_SOCKET_ATTEMPTS");
        if (env == NULL) {
            max_socket_attempts = 3;
        } else {
            max_socket_attempts = (int)strtol(env, NULL, 10);
        }
    }

    struct sockaddr_in server;

    ENVIRONMENT* environment = getIdamClientEnvironment();

    if (clientSocket >= 0) {
        return 0;                // Check Already Opened?
    }

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
	putUdaClientSSLSocket(clientSocket);
#endif

#ifdef _WIN32                            // Initialise WINSOCK Once only
	static unsigned int	initWinsock = 0;
	WORD sockVersion;
	WSADATA wsaData;
	if (!initWinsock) {
		sockVersion = MAKEWORD(2, 2);				// Select Winsock version 2.2
		WSAStartup(sockVersion, &wsaData);
		initWinsock = 1;
	}
#endif

    errno = 0;
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);        // create an unbound socket
    int serrno = errno;

    if (clientSocket < 0 || serrno != 0) {
        if (serrno != 0) {
            addIdamError(SYSTEMERRORTYPE, __func__, serrno, "");
        } else {
            addIdamError(CODEERRORTYPE, __func__, -1, "Problem Opening Socket");
        }
        return -1;
    }

    // UDA server host

    server.sin_family = AF_INET;
    const char* hostname = environment->server_host;

    // Check if the hostname is an alias for an IP address or domain name in the client configuration - replace if found

    int hostId = udaClientFindHostByAlias(hostname);
    if (hostId >= 0) {
        if ((hostname = udaClientGetHostName(hostId)) == NULL) {
            addIdamError(CODEERRORTYPE, __func__, -1, "The hostname is not recognised for the host alias provided!");
            return -1;
        }
        if (strcasecmp(environment->server_host, hostname) != 0) {
            strcpy(environment->server_host, hostname);    // Replace
        }
        int port = udaClientGetHostPort(hostId);
        if (port > 0 && environment->server_port != port) {
            environment->server_port = port;
        }
    } else if ((hostId = udaClientFindHostByName(hostname)) >= 0) {
        // No alias found, maybe the domain name or ip address is listed
        int port = udaClientGetHostPort(hostId);
        if (port > 0 && environment->server_port != port) {
            environment->server_port = port;
        }                // Replace if found and different
    }
    udaClientPutHostNameId(hostId);

    // Does the host name contain the SSL protocol prefix? If so strip this off

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    if(!strncasecmp(hostname, "SSL://", 6)){		// Should be stripped already if via the HOST client configuration file
        strcpy(environment->server_host, &hostname[6]);    // Replace
        putUdaClientSSLProtocol(1);
    } else {
        if(hostId >= 0 && udaClientGetHostSSL(hostId)){
            putUdaClientSSLProtocol(1);
        } else { 
            putUdaClientSSLProtocol(0);
        }   
    }
#endif

    // Resolve host

    errno = 0;
    struct hostent* host = gethostbyname(hostname);
    serrno = errno;

    if (host == NULL || serrno != 0) {
        if (serrno != 0) {
            addIdamError(SYSTEMERRORTYPE, __func__, serrno, "");
        } else {
            addIdamError(CODEERRORTYPE, __func__, -1, "Unknown Server Host");
        }
#ifndef _WIN32
        if (clientSocket != -1) close(clientSocket);
#else
        if (clientSocket != -1) closesocket(clientSocket);
#endif
        clientSocket = -1;
        return -1;
    }

    memcpy(&server.sin_addr, host->h_addr_list[0], host->h_length);

    // UDA server Port

    server.sin_port = htons(environment->server_port);

    // Connect to server

    errno = 0;
    while ((rc = connect(clientSocket, (struct sockaddr*)&server, sizeof(server))) && errno == EINTR) {}
    serrno = errno;

    if (rc < 0 || (serrno != 0 && serrno != EINTR)) {

        // Try again for a maximum number of tries with a random time delay between attempts

        int i, ps;
        ps = getpid();
        srand((unsigned int)ps);                                                // Seed the random number generator with the process id
        unsigned int delay = max_socket_delay > 0 ? (unsigned int)(rand() % max_socket_delay) : 0;         // random delay
        sleep(delay);                                                           // wait period
        for (i = 0; i < max_socket_attempts; i++) {                             // try again
            errno = 0;
            while ((rc = connect(clientSocket, (struct sockaddr*)&server, sizeof(server))) && errno == EINTR) {}
            serrno = errno;

            if (rc == 0) {
                break;
            }

            delay = max_socket_delay > 0 ? (unsigned int)(rand() % max_socket_delay) : 0;
            sleep(delay);                            // wait period
        }

        if (rc < 0 && strcmp(environment->server_host, environment->server_host2) != 0) {
            // Abandon principal Host - attempt secondary host
            hostname = environment->server_host2;

            // Check if the hostname is an alias for an IP address or name in the client configuration - replace if found

            int hostId = udaClientFindHostByAlias(hostname);
            if (hostId >= 0) {
                if ((hostname = udaClientGetHostName(hostId)) == NULL) {
                    addIdamError(CODEERRORTYPE, __func__, -1, "The hostname2 is not recognised for the host alias provided!");
                    return -1;
                }
                if (strcasecmp(environment->server_host2, hostname) != 0) {
                    strcpy(environment->server_host2, hostname);
                }
                int port = udaClientGetHostPort(hostId);
                if (port > 0 && environment->server_port2 != port) {
                    environment->server_port2 = port;
                }
                udaClientPutHostNameId(hostId);
            } else if ((hostId = udaClientFindHostByName(hostname)) >= 0) {    // No alias found
                int port = udaClientGetHostPort(hostId);
                if (port > 0 && environment->server_port2 != port) {
                    environment->server_port2 = port;
                }
            }

            // Does the host name contain the SSL protocol prefix? If so strip this off

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
            if(!strncasecmp(hostname, "SSL://", 6)){		// Should be stripped already if via the HOST client configuration file
                strcpy(environment->server_host2, &hostname[6]);    // Replace
                putUdaClientSSLProtocol(1);
            } else {
                if(hostId >= 0 && udaClientGetHostSSL(hostId)){
                    putUdaClientSSLProtocol(1);
                } else { 
                    putUdaClientSSLProtocol(0);
                }
            }
#endif

            // Resolve host

            errno = 0;
            host = gethostbyname(hostname);

            if (host == NULL || errno != 0) {
                if (errno != 0) {
                    addIdamError(SYSTEMERRORTYPE, __func__, errno, "");
                } else {
                    addIdamError(CODEERRORTYPE, __func__, -1, "Unknown Server Host");
                }
                if (clientSocket != -1) {
                    close(clientSocket);
                }
                clientSocket = -1;
                return -1;
            }
            memcpy(&server.sin_addr, host->h_addr_list[0], host->h_length);
            server.sin_port = htons(environment->server_port2);
            for (i = 0; i < max_socket_attempts; i++) {
                errno = 0;
                while ((rc = connect(clientSocket, (struct sockaddr*)&server, sizeof(server))) && errno == EINTR) {}
                serrno = errno;
                if (rc == 0) {
                    char* name;
                    int port;
                    port = environment->server_port2;                // Swap data so that accessor function reports correctly
                    environment->server_port2 = environment->server_port;
                    environment->server_port = port;
                    name = (char*)malloc((strlen(environment->server_host) + 1) * sizeof(char));
                    strcpy(name, environment->server_host2);
                    strcpy(environment->server_host2, environment->server_host);
                    strcpy(environment->server_host, name);
                    free((void*)name);
                    break;
                }
                delay = max_socket_delay > 0 ? (unsigned int)(rand() % max_socket_delay) : 0;
                sleep(delay);                            // wait period
            }
        }

        if (rc < 0) {
            if (serrno != 0) {
                addIdamError(SYSTEMERRORTYPE, __func__, serrno, "");
            } else {
                addIdamError(CODEERRORTYPE, __func__, -1, "Unable to Connect to Server Stream Socket");
            }
            if (clientSocket != -1) {
                close(clientSocket);
            }
            clientSocket = -1;
            return -1;
        }
    }

    // Set the receive and send buffer sizes

    setsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, (char*)&window_size, sizeof(window_size));

    setsockopt(clientSocket, SOL_SOCKET, SO_RCVBUF, (char*)&window_size, sizeof(window_size));

    // Other Socket Options

    int on = 1;
    if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on)) < 0) {
        addIdamError(CODEERRORTYPE, __func__, -1, "Error Setting KEEPALIVE on Socket");
        close(clientSocket);
        clientSocket = -1;
        return -1;
    }
    on = 1;
    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)) < 0) {
        addIdamError(CODEERRORTYPE, __func__, -1, "Error Setting NODELAY on Socket");
        close(clientSocket);
        clientSocket = -1;
        return -1;
    }

    // Add New Socket to the Socket's List

    addSocket(&client_socketlist, TYPE_IDAM_SERVER, 1, environment->server_host, environment->server_port,
              clientSocket);
    client_socketlist.sockets[getSocketRecordId(&client_socketlist, clientSocket)].Input = clientInput;
    client_socketlist.sockets[getSocketRecordId(&client_socketlist, clientSocket)].Output = clientOutput;
    environment->server_reconnect = 0;
    environment->server_change_socket = 0;
    environment->server_socket = clientSocket;

    // Write the socket number to the SSL functions

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    putUdaClientSSLSocket(clientSocket);
#endif

    return 0;
}

void closeConnection(int type)
{
    if (clientSocket >= 0 && type != 1) {
        closeClientSocket(&client_socketlist, clientSocket);
    } else {
        closeClientSockets(&client_socketlist);
    }

    clientSocket = -1;
}

int clientWriteout(void* iohandle, char* buf, int count)
{
#ifndef _WIN32
    void (* OldSIGPIPEHandler)();
#endif
    int rc = 0;
    size_t BytesSent = 0;

    fd_set wfds;
    struct timeval tv;

    idamUpdateSelectParms(clientSocket, &wfds, &tv);

    errno = 0;

    while (select(clientSocket + 1, NULL, &wfds, NULL, &tv) <= 0) {

        if (errno == ECONNRESET || errno == ENETUNREACH || errno == ECONNREFUSED) {
            if (errno == ECONNRESET) {
                UDA_LOG(UDA_LOG_DEBUG, "ECONNRESET error!\n");
                addIdamError(CODEERRORTYPE, __func__, -2, "ECONNRESET: The server program has crashed or closed the socket unexpectedly");
                return -2;
            } else {
                if (errno == ENETUNREACH) {
                    UDA_LOG(UDA_LOG_DEBUG, "ENETUNREACH error!\n");
                    addIdamError(CODEERRORTYPE, __func__, -3, "Server Unavailable: ENETUNREACH");
                    return -3;
                } else {
                    UDA_LOG(UDA_LOG_DEBUG, "ECONNREFUSED error!\n");
                    addIdamError(CODEERRORTYPE, __func__, -4, "Server Unavailable: ECONNREFUSED");
                    return -4;
                }
            }
        }

        idamUpdateSelectParms(clientSocket, &wfds, &tv);
    }

    /* UNIX version

     Ignore the SIG_PIPE signal.  Standard behaviour terminates
     the application with an error code of 141.  When the signal
     is ignored, write calls (when there is no server process to
     communicate with) will return with errno set to EPIPE
    */

#ifndef _WIN32
    if ((OldSIGPIPEHandler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
        addIdamError(CODEERRORTYPE, __func__, -1, "Error attempting to ignore SIG_PIPE");
        return -1;
    }
#endif

// Write to socket, checking for EINTR, as happens if called from IDL

    while (BytesSent < (unsigned int)count) {
#ifndef _WIN32
        while (((rc = (int)write(clientSocket, buf, count)) == -1) && (errno == EINTR)) {}
#else
        while (((rc = send(clientSocket, buf , count, 0)) == SOCKET_ERROR) && (errno == EINTR)) {}
#endif
        BytesSent += rc;
        buf += rc;
    }

// Restore the original SIGPIPE handler set by the application

#ifndef _WIN32
    if (signal(SIGPIPE, OldSIGPIPEHandler) == SIG_ERR) {
        addIdamError(CODEERRORTYPE, __func__, -1, "Error attempting to restore SIG_PIPE handler");
        return -1;
    }
#endif

    return rc;
}

int clientReadin(void* iohandle, char* buf, int count)
{
    int rc, serrno;
    fd_set rfds;
    struct timeval tv;

    int maxloop = 0;

    errno = 0;

    /* Wait till it is possible to read from socket */

    idamUpdateSelectParms(clientSocket, &rfds, &tv);

    while ((select(clientSocket + 1, &rfds, NULL, NULL, &tv) <= 0) && maxloop++ < MAXLOOP) {
        idamUpdateSelectParms(clientSocket, &rfds, &tv);        // Keep trying ...
    }

// Read from it, checking for EINTR, as happens if called from IDL

#ifndef _WIN32
    while (((rc = (int)read(clientSocket, buf, count)) == -1) && (errno == EINTR)) {}
#else
    while ((( rc=recv( clientSocket, buf, count, 0)) == SOCKET_ERROR ) && (errno == EINTR)) {}
#endif

    serrno = errno;

// As we have waited to be told that there is data to be read, if nothing
// arrives, then there must be an error

    if (!rc) {
        rc = -1;
        if (serrno != 0 && serrno != EINTR) {
            addIdamError(SYSTEMERRORTYPE, __func__, rc, "");
        }
        addIdamError(CODEERRORTYPE, __func__, rc, "No Data waiting at Socket when Data Expected!");
    }

    return rc;
}
