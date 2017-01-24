//! $LastChangedRevision: 38 $
//! $LastChangedDate: 2008-02-21 09:41:00 +0000 (Thu, 21 Feb 2008) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/clientserver/md5Sum.c $

/*---------------------------------------------------------------
* Returns an MD5 Checksum String for a Block of Bytes in Memory
*
* Input Arguments:	char *bp	Pointer to a Block of Memory
*			int size	Size of the Memory block in Bytes
*
* Returns:		md5check	checksum String of Minimum Length 2*MD5_SIZE+1
*					allocated in the calling routine, e.g.,
*					char md5check[2*MD5_SIZE+1]="";
*
* Calls
*
* Notes:
*
* ToDo:
*
* Change History
*
* 16Oct2006	D.G.Muir	Original Version
* 18Feb2008	D.G.Muir	Make it dependent on a Compiler Option: MD5SUM
*-----------------------------------------------------------------------------*/

#include "md5Sum.h"

#include <string.h>

#ifdef MD5SUM
#  include <sys/stat.h>
#  include <md5.h>

void md5Sum(char *bp, int size, char *md5check) {
    int i;
    unsigned char signature[MD5_SIZE]="";
    MD5_CTX context;

    MD5_Init(&context);
    MD5_Update(&context, bp, size);
    MD5_Final(signature, &context);

    md5check[2*MD5_SIZE]='\0';

    for(i=0; i<MD5_SIZE; i++) {
        sprintf(md5check+2*i, "%02x", signature[i]);
    }
}
#else

void md5Sum(char *bp, int size, char *md5check) {
    strcpy(md5check, "");
    return;
}

#endif
