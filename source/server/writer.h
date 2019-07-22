#ifndef IDAM_SERVER_WRITER_H
#define IDAM_SERVER_WRITER_H

#include <unistd.h>
#include <fcntl.h>

#ifdef _WIN32
#  include <winsock2.h> // must be included before connection.h to avoid macro redefinition in rpc/types.h
#else
#  include <sys/select.h>
#endif

#define MIN_BLOCK_TIME	1000
#define MAX_BLOCK_TIME	10000

#ifdef __cplusplus
extern "C" {
#endif

extern int server_tot_block_time;
extern int server_timeout;

void setSelectParms(int fd, fd_set *rfds, struct timeval *tv);
void updateSelectParms(int fd, fd_set *rfds, struct timeval *tv);
int Writeout(void *iohandle, char *buf, int count);

/*
//-----------------------------------------------------------------------------------------
// This routine is only called when the Server expects to Read something from the Client
//
// There are two time constraints:
//
//	The Maximum Blocking period is 1ms when reading
//	A Maximum number (MAXLOOP) of blocking periods is allowed before this time
//	is modified: It is extended to 100ms to minimise server resource consumption. 
//
// When the Server is in a Holding state, it is listening to the Socket for either a
// Closedown or a Data request. 
//
// Three Global variables are used to control the Blocking timeout
//
//	min_block_time
//	max_block_time 
//	tot_block_time
//
// A Maximum time (MAXBLOCK in seconds) from the last Data Request is permitted before the Server Automatically
// closes down.	
//-----------------------------------------------------------------------------------------
*/
int Readin(void *iohandle, char *buf, int count);

#endif // IDAM_SERVER_WRITER_H
