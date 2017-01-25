#ifndef IDAM_STRUCTURES_PARSEINCLUDEFILE_H
#define IDAM_STRUCTURES_PARSEINCLUDEFILE_H

#if defined(SERVERBUILD) || !defined(CLEANNAMESPACE25SEP14)

int parseIncludeFile(char* header);

#else
#  include "idamgenstruct.h"
int parseIdamIncludeFile(char *header, USERDEFINEDTYPELIST *userdefinedtypelist);
#endif

#endif // IDAM_STRUCTURES_PARSEINCLUDEFILE_H

