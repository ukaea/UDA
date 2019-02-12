// Create an IPv4 or IPv6 Socket Connection to the UDA server with a randomised time delay between connection attempts
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
#    include <sys/socket.h>
#    include <netinet/in.h>
#    include <arpa/inet.h>
#    include <netdb.h>
#    include <netinet/tcp.h>
#  endif
#  include <unistd.h>
#  include <strings.h>
#else
#  include <process.h>
#  include <ws2tcpip.h>
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

#define PORT_STRING	64

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

void localhostInfo(int *ai_family)
{
   char addr_buf[64];
   struct addrinfo *info = NULL, *result=NULL;
   getaddrinfo("localhost", NULL, NULL, &info);
   result = info;	// Take the first linked list member
   //for(result = info; result != NULL; result = result->ai_next)
   //{   
       if ( result->ai_family == AF_INET )
       {
          *ai_family = AF_INET;
	  inet_ntop(AF_INET, &((struct sockaddr_in *)result->ai_addr)->sin_addr, addr_buf, sizeof(addr_buf));
          UDA_LOG(UDA_LOG_DEBUG, "localhost Information: IPv4 - %s\n", addr_buf);
       }
       else
       {
          *ai_family = AF_INET6;
          inet_ntop(AF_INET6, &((struct sockaddr_in6 *)result->ai_addr)->sin6_addr, addr_buf, sizeof(addr_buf));
          UDA_LOG(UDA_LOG_DEBUG, "localhost Information: IPv6 - %s\n", addr_buf);
       }
   //}
   if(info) freeaddrinfo(info); 
}

void setHints(struct addrinfo *hints, const char* hostname)
{
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = 0; //AI_CANONNAME | AI_V4MAPPED | AI_ALL | AI_ADDRCONFIG ;
    hints->ai_protocol = 0;
    hints->ai_canonname = NULL;
    hints->ai_addr = NULL;
    hints->ai_next = NULL;
    
    // Localhost? Which IP family? (AF_UNSPEC gives an 'Unknown Error'!)
    
    if(!strcmp(hostname, "localhost")) localhostInfo(&hints->ai_family);
    
    // Is the address Numeric? Is it IPv6?
    
    if (strchr(hostname,':')){		// Appended port number should have been stripped off
        hints->ai_family = AF_INET6 ;
        hints->ai_flags = hints->ai_flags | AI_NUMERICHOST;
    } else {
        // How many '.' characters?
	char *p, *w = (char *)hostname;
	int lcount = 0, count = 1;
	while((p = strchr(w, '.')) != NULL){
	    w = &p[1];
	    count++;
	}
	if(count == 1) return;
	
	// make a list of address components
	char **list = (char **)malloc(count*sizeof(char *));
	char *work = strdup(hostname);
	w = work;
	while((p = strchr(w, '.')) != NULL && lcount < count){
	    p[0] = '\0';
	    list[lcount++] = strdup(w);
	    w = &p[1];
	}
	list[lcount++] = strdup(w);
	
	// Are all address components numeric?
	int i, isNumeric=1;
	for (i=0;i<lcount;i++){
	   int j, lstr=strlen(list[i]);
	   for (j=0; j<lstr; j++) isNumeric = isNumeric & (list[i][j] >= '0' && list[i][j] <= '9');
	}
	free(work);
	for (i=0;i<lcount;i++) free(list[i]);
	free(list);
	
	if(isNumeric) hints->ai_flags = hints->ai_flags | AI_NUMERICHOST; 
    }
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


    // Identify the UDA server host and is the socket IPv4 or IPv6?

    const char* hostname = environment->server_host;
    char serviceport[PORT_STRING];

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
    sprintf(serviceport, "%d", environment->server_port);

    // Does the host name contain the SSL protocol prefix? If so strip this off

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    if (!strncasecmp(hostname, "SSL://", 6)) {           // Should be stripped already if via the HOST client configuration file
        strcpy(environment->server_host, &hostname[6]);  // Replace
        putUdaClientSSLProtocol(1);
    } else {
        if(hostId >= 0 && udaClientGetHostSSL(hostId)){
            putUdaClientSSLProtocol(1);
        } else { 
            putUdaClientSSLProtocol(0);
        }   
    }
#endif

    // Resolve the Host and the IP protocol to be used (Hints not used)
    
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0};
    setHints(&hints, hostname);
    
    errno = 0;
    if((rc = getaddrinfo(hostname, serviceport, &hints, &result)) != 0 || errno != 0){
       addIdamError(SYSTEMERRORTYPE, __func__, rc, (char *)gai_strerror(rc));
       if (rc == EAI_SYSTEM || errno != 0) addIdamError(SYSTEMERRORTYPE, __func__, errno, "");
       if (result) freeaddrinfo(result);
       return -1;
    }
    
    if ( result->ai_family == AF_INET )
       UDA_LOG(UDA_LOG_DEBUG, "Socket Connection is IPv4\n");
    else
       UDA_LOG(UDA_LOG_DEBUG, "Socket Connection is IPv6\n");

    errno = 0;
    clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);   

    if (clientSocket < 0 || errno != 0) {
        if (errno != 0) {
            addIdamError(SYSTEMERRORTYPE, __func__, errno, "");
        } else {
            addIdamError(CODEERRORTYPE, __func__, -1, "Problem Opening Socket");
        }
	if (clientSocket != -1)
#ifndef _WIN32
            close(clientSocket);
#else
            closesocket(clientSocket);
#endif	 
        clientSocket = -1;
	freeaddrinfo(result);
        return -1;
    }

    // Connect to server
    
    errno = 0;
    while ((rc = connect(clientSocket, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}
    
    if (rc < 0 || (errno != 0 && errno != EINTR)) {

        // Try again for a maximum number of tries with a random time delay between attempts

        int i, ps;
        ps = getpid();
        srand((unsigned int)ps);                                                // Seed the random number generator with the process id
        unsigned int delay = max_socket_delay > 0 ? (unsigned int)(rand() % max_socket_delay) : 0;         // random delay
        sleep(delay);
	    errno = 0;                                                           // wait period
        for (i = 0; i < max_socket_attempts; i++) {                             // try again
            while ((rc = connect(clientSocket, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}

            if (rc == 0 && errno == 0)  break;

            delay = max_socket_delay > 0 ? (unsigned int)(rand() % max_socket_delay) : 0;
            sleep(delay);                            // wait period
        }
	
	    if(rc != 0 || errno != 0){
            UDA_LOG(UDA_LOG_DEBUG, "Connect errno = %d\n", errno);
            UDA_LOG(UDA_LOG_DEBUG, "Connect rc = %d\n", rc);
            UDA_LOG(UDA_LOG_DEBUG, "Unable to connect to primary host: %s on port %s\n", hostname, serviceport);
 	    }

        // Abandon the principal Host - attempt to connect to the secondary host
        if (rc < 0 && strcmp(environment->server_host, environment->server_host2) != 0 && strlen(environment->server_host2) > 0) {
	    
	        freeaddrinfo(result);
	        result = NULL;
#ifndef _WIN32
            close(clientSocket);
#else
            closesocket(clientSocket);
#endif	 
            clientSocket = -1;
	        udaClientPutHostNameId(-1);
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
            } else if ((hostId = udaClientFindHostByName(hostname)) >= 0) {    // No alias found
                int port = udaClientGetHostPort(hostId);
                if (port > 0 && environment->server_port2 != port) {
                    environment->server_port2 = port;
                }
            }
	        udaClientPutHostNameId(hostId);
            sprintf(serviceport, "%d", environment->server_port2);

            // Does the host name contain the SSL protocol prefix? If so strip this off

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
            if (!strncasecmp(hostname, "SSL://", 6)){		// Should be stripped already if via the HOST client configuration file
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

            // Resolve the Host and the IP protocol to be used (Hints not used)

	        setHints(&hints, hostname);

            errno = 0;
            if ((rc = getaddrinfo(hostname, serviceport, &hints, &result)) != 0 || errno != 0) {
               addIdamError(SYSTEMERRORTYPE, __func__, rc, (char *)gai_strerror(rc));
               if (rc == EAI_SYSTEM || errno != 0) addIdamError(SYSTEMERRORTYPE, __func__, errno, "");
	           if (result) freeaddrinfo(result);
               return -1;
            }
            errno = 0;
            clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);   

            if (clientSocket < 0 || errno != 0) {
                if (errno != 0) {
                    addIdamError(SYSTEMERRORTYPE, __func__, errno, "");
                } else {
                    addIdamError(CODEERRORTYPE, __func__, -1, "Problem Opening Socket");
                }
                if (clientSocket != -1)
#ifndef _WIN32
                    close(clientSocket);
#else
                    closesocket(clientSocket);
#endif	 
	            clientSocket = -1;
	            freeaddrinfo(result);
                return -1;
            }

            for (i = 0; i < max_socket_attempts; i++) {
                while ((rc = connect(clientSocket, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}
                if (rc == 0) {
                    int port = environment->server_port2;                // Swap data so that accessor function reports correctly
                    environment->server_port2 = environment->server_port;
                    environment->server_port = port;
                    char *name = (char*)malloc((strlen(environment->server_host2) + 1) * sizeof(char));
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
            if (errno != 0) {
                addIdamError(SYSTEMERRORTYPE, __func__, errno, "");
            } else {
                addIdamError(CODEERRORTYPE, __func__, -1, "Unable to Connect to Server Stream Socket");
            }
            if (clientSocket != -1)
#ifndef _WIN32
                close(clientSocket);
#else
                closesocket(clientSocket);
#endif	 
	        clientSocket = -1;
	        if(result) freeaddrinfo(result);
            return -1;
        }
    }
    
    if(result) freeaddrinfo(result);

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
    int rc;
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

    // As we have waited to be told that there is data to be read, if nothing
    // arrives, then there must be an error

    if (!rc) {
        rc = -1;
        if (errno != 0 && errno != EINTR) {
            addIdamError(SYSTEMERRORTYPE, __func__, rc, "");
        }
        addIdamError(CODEERRORTYPE, __func__, rc, "No Data waiting at Socket when Data Expected!");
    }

    return rc;
}
