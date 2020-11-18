#ifndef UDA_PLUGIN_TESTPLUGIN_TESTPLUGIN_H
#define UDA_PLUGIN_TESTPLUGIN_TESTPLUGIN_H

#include <plugins/udaPlugin.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TESTUDT
   #include "udtc.h"
   #include <pthread.h>

   typedef int bool;
   int g_IP_Version   = AF_INET;		// IPv4 family of addresses
   int g_Socket_Type  = SOCK_STREAM;		// use reliable transport layer protocol with Aknowledgements (default TCP)

   char g_Localhost[] = "192.168.16.88";	//"192.168.16.125"; //"127.0.0.1"; // "192.168.16.88";	// Client IP address (*** passed as a parameter)
   int g_Server_Port  = 50000;			// port number (*** passed as a parameter)

   int g_TotalNum     = 1000000;		// Test data 
   
LIBRARY_API int tcp_connect(SYSSOCKET *ssock, int port);
LIBRARY_API int c_connect(UDTSOCKET *usock, int port);
LIBRARY_API int createUDTSocket(int *usock, int port, int rendezvous);
LIBRARY_API int createTCPSocket(SYSSOCKET *ssock, int port, bool rendezvous);
#endif

extern int testplugin(IDAM_PLUGIN_INTERFACE *idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_TESTPLUGIN_TESTPLUGIN_H
