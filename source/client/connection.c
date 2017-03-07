// Create a Socket Connection to the IDAM server with a randomised time delay between connection attempts
//
//----------------------------------------------------------------

#include "connection.h"

#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/tcp.h>

#ifndef _WIN32
#  include <unistd.h>
#  include <signal.h>
#endif

#include <clientserver/errorLog.h>
#include <clientserver/manageSockets.h>
#include <client/udaClient.h>
#include <logging/logging.h>

#include "updateSelectParms.h"
#include "getEnvironment.h"

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
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err,
                         "The User Specified Socket Connection does not exist");
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
    int on = 1;
    int rc, serrno;
    char* hostname;

    struct sockaddr_in server;
    struct hostent* host;

    ENVIRONMENT* environment = getIdamClientEnvironment();

    if (clientSocket >= 0) {
        return 0;                // Check Already Opened?

    }
#ifdef _WIN32                            // Initialise WINSOCK Once only
        static unsigned int	initWinsock = 0;
        WORD sockVersion;
        WSADATA wsaData;
        if(!initWinsock) {
            sockVersion = MAKEWORD(1, 1);				// Select Winsock version 1.1
            WSAStartup(sockVersion, &wsaData);
            initWinsock = 1;
        }
#endif

    errno = 0;
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);        // create an unbound socket
    serrno = errno;

    if (clientSocket < 0 || serrno != 0) {
        if (serrno != 0) {
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamCreateConnection", serrno, "");
        } else {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamCreateConnection", -1, "Problem Opening Socket");
        }
        return -1;
    }

// IDAM server host
    
    server.sin_family = AF_INET;
    hostname = environment->server_host;

// Resolve host

    errno = 0;
    host = gethostbyname(hostname);
    serrno = errno;

    if (host == NULL || serrno != 0) {
        if (serrno != 0) {
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamCreateConnection", serrno, "");
        } else {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamCreateConnection", -1, "Unknown Server Host");
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


// IDAM server Port

    server.sin_port = htons(environment->server_port);

// Connect to server

    errno = 0;
    while ((rc = connect(clientSocket, (struct sockaddr*) &server, sizeof(server))) && errno == EINTR) {}
    serrno = errno;

    if (rc < 0 || (serrno != 0 && serrno != EINTR)) {

// Try again for a maximum number of tries with a random time delay between attempts

        int i, ps;
        float delay;
        ps = getpid();
        srand((unsigned int) ps);                        // Seed the random number generator with the process id
        delay = MAX_SOCKET_DELAY * ((float) rand() / (float) RAND_MAX);        // random delay
        sleep(delay);                            // wait period
        for (i = 0; i < MAX_SOCKET_ATTEMPTS; i++) {                // try again
            errno = 0;
            while ((rc = connect(clientSocket, (struct sockaddr*) &server, sizeof(server))) && errno == EINTR) {}
            serrno = errno;

            if (rc == 0) break;

            delay = MAX_SOCKET_DELAY * ((float) rand() / (float) RAND_MAX);
            sleep(delay);
        }

        if (rc < 0 && strcmp(environment->server_host, environment->server_host2) !=
                      0) {        // Abandon principal Host - attempt secondary host
            hostname = environment->server_host2;
            errno = 0;
            host = gethostbyname(hostname);
            if (host == NULL || errno != 0) {
                if (errno != 0) {
                    addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamCreateConnection", errno, "");
                } else {
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamCreateConnection", -1, "Unknown Server Host");
                }
                if (clientSocket != -1) close(clientSocket);
                clientSocket = -1;
                return -1;
            }
            memcpy(&server.sin_addr, host->h_addr_list[0], host->h_length);
            server.sin_port = htons(environment->server_port2);
            for (i = 0; i < MAX_SOCKET_ATTEMPTS; i++) {
                errno = 0;
                while ((rc = connect(clientSocket, (struct sockaddr*) &server, sizeof(server))) && errno == EINTR) {}
                serrno = errno;
                if (rc == 0) {
                    char* name;
                    int port;
                    port = environment->server_port2;                // Swap data so that accessor function reports correctly
                    environment->server_port2 = environment->server_port;
                    environment->server_port = port;
                    name = (char*) malloc((strlen(environment->server_host) + 1) * sizeof(char));
                    strcpy(name, environment->server_host2);
                    strcpy(environment->server_host2, environment->server_host);
                    strcpy(environment->server_host, name);
                    free((void*) name);
                    break;
                }
                delay = MAX_SOCKET_DELAY * ((float) rand() / (float) RAND_MAX);
                sleep(delay);
            }
        }

        if (rc < 0) {
            if (serrno != 0) {
                addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamCreateConnection", serrno, "");
            } else {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamCreateConnection", -1,
                             "Unable to Connect to Server Stream Socket");
            }
            if (clientSocket != -1) close(clientSocket);
            clientSocket = -1;
            return -1;
        }
    }

// Set the receive and send buffer sizes

    setsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF,
               (char*) &window_size, sizeof(window_size));

    setsockopt(clientSocket, SOL_SOCKET, SO_RCVBUF,
               (char*) &window_size, sizeof(window_size));

// Other Socket Options

    on = 1;
    if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, (char*) &on, sizeof(on)) < 0) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamCreateConnection", -1, "Error Setting KEEPALIVE on Socket");
        close(clientSocket);
        clientSocket = -1;
        return -1;
    }
    on = 1;
    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*) &on, sizeof(on)) < 0) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamCreateConnection", -1, "Error Setting NODELAY on Socket");
        close(clientSocket);
        clientSocket = -1;
        return -1;
    }

// Add New Socket to the Socket's List

    addSocket(&client_socketlist, TYPE_IDAM_SERVER, 1, environment->server_host, environment->server_port, clientSocket);
    client_socketlist.sockets[getSocketRecordId(&client_socketlist, clientSocket)].Input = clientInput;
    client_socketlist.sockets[getSocketRecordId(&client_socketlist, clientSocket)].Output = clientOutput;
    environment->server_reconnect = 0;
    environment->server_change_socket = 0;
    environment->server_socket = clientSocket;

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
    void (* OldSIGPIPEHandler)();
    int rc = 0;
    size_t BytesSent = 0;

    fd_set wfds;
    struct timeval tv;

    idamUpdateSelectParms(clientSocket, &wfds, &tv);

    errno = 0;

    while (select(clientSocket + 1, NULL, &wfds, NULL, &tv) <= 0) {

        if (errno == ECONNRESET || errno == ENETUNREACH || errno == ECONNREFUSED) {
            if (errno == ECONNRESET) {
                IDAM_LOG(LOG_DEBUG, "idamClientWriteout: ECONNRESET error!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientWriteout", -2,
                             "ECONNRESET: The server program has crashed or closed the socket unexpectedly");
                return -2;
            } else {
                if (errno == ENETUNREACH) {
                    IDAM_LOG(LOG_DEBUG, "idamClientWriteout: ENETUNREACH error!\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientWriteout", -3,
                                 "Server Unavailable: ENETUNREACH");
                    return -3;
                } else {
                    IDAM_LOG(LOG_DEBUG, "idamClientWriteout: ECONNREFUSED error!\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientWriteout", -4,
                                 "Server Unavailable: ECONNREFUSED");
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

    if ((OldSIGPIPEHandler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientWriteout", -1, "Error attempting to ignore SIG_PIPE");
        return -1;
    }

// Write to socket, checking for EINTR, as happens if called from IDL

    while (BytesSent < count) {
#ifndef _WIN32
        while (((rc = (int) write(clientSocket, buf, count)) == -1) && (errno == EINTR)) { }
#else
        while ( ( (rc=send( clientSocket, buf , count, 0 )) == SOCKET_ERROR) && (errno == EINTR) ) {}
#endif
        BytesSent += rc;
        buf += rc;
    }

// Restore the original SIGPIPE handler set by the application

    if (signal(SIGPIPE, OldSIGPIPEHandler) == SIG_ERR) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientWriteout", -1,
                     "Error attempting to restore SIG_PIPE handler");
        return -1;
    }

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
    while (((rc = (int) read(clientSocket, buf, count)) == -1) && (errno == EINTR)) { }
#else
    while ((( rc=recv( clientSocket, buf, count, 0)) == SOCKET_ERROR ) && (errno == EINTR)) {}

    // if rc == 0 then socket is closed => server fail?

#endif

    serrno = errno;

// As we have waited to be told that there is data to be read, if nothing
// arrives, then there must be an error

    if (!rc) {
        rc = -1;
        if (serrno != 0 && serrno != EINTR) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamClientReadin", rc, "");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientReadin", rc,
                     "No Data waiting at Socket when Data Expected!");
    }

    return rc;
}
