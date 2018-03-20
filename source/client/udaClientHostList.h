#ifndef UDA_CLIENT_HOSTLIST_H
#define UDA_CLIENT_HOSTLIST_H

#define HOST_MCOUNT		100		// Maximum initial number of hosts that can be registered
#define HOST_MSTEP		10		// Increase heap by 10 records once the maximum is exceeded
#define HOST_STRING		256

typedef struct {
    char hostalias[HOST_STRING];
    char hostname[HOST_STRING];
    char certificate[HOST_STRING];
    char key[HOST_STRING];
    char ca_certificate[HOST_STRING];
    int port;
    int isSSL;
} HOSTDATA;

typedef struct {
    int count;		// Number of hosts in list
    int mcount;	// Allocated Size of the List
    HOSTDATA* hosts;    // List of host aliases and SSL certificate locations
} HOSTLIST;

void udaClientAllocHostList(int count);
void udaClientFreeHostList(void);
void udaClientInitHostData(HOSTDATA* host);
int udaClientFindHostByAlias(const char* alias);
int udaClientFindHostByName(const char* name);
void udaClientInitHostList(void);
char* udaClientGetHostName(int id);
char* udaClientGetHostAlias(int id);
int udaClientGetHostPort(int id);
char* udaClientGetHostCertificatePath(int id);
char* udaClientGetHostKeyPath(int id);
char* udaClientGetHostCAPath(int id);
int udaClientGetHostSSL(int id);
void udaClientPutHostNameId(int id);
int udaClientGetHostNameId(void);

#endif // UDA_CLIENT_HOSTLIST_H
