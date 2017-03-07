// Create a Socket Connection to the IDAM server with a randomised time delay between connection attempts
//
//----------------------------------------------------------------

#include "createConnection.h"

#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/tcp.h>

#include <clientserver/errorLog.h>
#include <clientserver/manageSockets.h>
#include <client/udaClient.h>
#include <client/getEnvironment.h>

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
    while ((rc = connect(clientSocket, (struct sockaddr*)&server, sizeof(server))) && errno == EINTR) {}
    serrno = errno;

    if (rc < 0 || (serrno != 0 && serrno != EINTR)) {

// Try again for a maximum number of tries with a random time delay between attempts

        int i, ps;
        float delay;
        ps = getpid();
        srand((unsigned int)ps);                        // Seed the random number generator with the process id
        delay = MAX_SOCKET_DELAY * ((float)rand() / (float)RAND_MAX);        // random delay
        sleep(delay);                            // wait period
        for (i = 0; i < MAX_SOCKET_ATTEMPTS; i++) {                // try again
            errno = 0;
            while ((rc = connect(clientSocket, (struct sockaddr*)&server, sizeof(server))) && errno == EINTR) {}
            serrno = errno;

            if (rc == 0) break;

            delay = MAX_SOCKET_DELAY * ((float)rand() / (float)RAND_MAX);
            sleep(delay);
        }

        if (rc < 0 && strcmp(environment->server_host, environment->server_host2) != 0) {
            // Abandon principal Host - attempt secondary host
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
                delay = MAX_SOCKET_DELAY * ((float)rand() / (float)RAND_MAX);
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
               (char*)&window_size, sizeof(window_size));

    setsockopt(clientSocket, SOL_SOCKET, SO_RCVBUF,
               (char*)&window_size, sizeof(window_size));

// Other Socket Options

    on = 1;
    if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on)) < 0) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamCreateConnection", -1, "Error Setting KEEPALIVE on Socket");
        close(clientSocket);
        clientSocket = -1;
        return -1;
    }
    on = 1;
    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)) < 0) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamCreateConnection", -1, "Error Setting NODELAY on Socket");
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

    return 0;
}
