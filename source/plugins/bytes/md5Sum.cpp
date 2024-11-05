/*---------------------------------------------------------------
 * Returns an MD5 Checksum String for a Block of Bytes in Memory
 *
 * Input Arguments:    char *bp    Pointer to a Block of Memory
 *            int size    Size of the Memory block in Bytes
 *
 * Returns:        md5check    checksum String of Minimum Length 2*Md5Size+1
 *                    allocated in the calling routine, e.g.,
 *                    char md5check[2*Md5Size+1]="";
 *
 * Calls
 *
 * Notes:
 *
 * ToDo:
 *
 *-----------------------------------------------------------------------------*/

#include "md5Sum.h"

#include <string.h>

#ifdef MD5SUM
#  include <md5.h>
#  include <sys/stat.h>

void md5Sum(char* bp, int size, char* md5check)
{
    unsigned char signature[Md5Size] = "";
    MD5_CTX context;

    MD5_Init(&context);
    MD5_Update(&context, bp, size);
    MD5_Final(signature, &context);

    md5check[2 * Md5Size] = '\0';

    for (i = 0; i < Md5Size; i++) {
        sprintf(md5check + 2 * i, "%02x", signature[i]);
    }
}
#else

void md5Sum(char* bp, int size, char* md5check)
{
    strcpy(md5check, "");
}

#endif
