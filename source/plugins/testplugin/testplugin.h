
// Test Plugin Library

#ifndef IDAM_PLUGINS_TESTPLUGIN_TESTPLUGIN_H
#define IDAM_PLUGINS_TESTPLUGIN_TESTPLUGIN_H

// Change History:
//
// 25Oct2011	dgm	Original Version

#include <idamplugin.h>

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
   
int tcp_connect(SYSSOCKET *ssock, int port);
int c_connect(UDTSOCKET *usock, int port);
int createUDTSocket(int *usock, int port, int rendezvous);
int createTCPSocket(SYSSOCKET *ssock, int port, bool rendezvous);
     
#endif

extern int testplugin(IDAM_PLUGIN_INTERFACE *idam_plugin_interface);

static void testError1();
static void testError2();

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_TESTPLUGIN_TESTPLUGIN_H
