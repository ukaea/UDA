
#ifndef IDAM_MANAGEFILES_H
#define IDAM_MANAGEFILES_H

#include "idamclientserver.h"
#include "idamserver.h"
#include "idamserverfiles.h"

// Initialise the File List

void initIdamFileList(IDAMFILELIST *idamfiles);

// Add a New Open File to the List or re-Open an existing record
// returns 1 if the handle already exists, 0 if not

int addIdamFile(IDAMFILELIST *idamfiles, int type, char *filename, void *handle);

// Search for an Open File in the List
// Returns an opaque pointer to the appropriate handle if found, NULL othewise.

void *getOpenIdamFile(IDAMFILELIST *idamfiles, int type, char *filename);

// Search for a Closed File in the List

int getClosedIdamFile(IDAMFILELIST *idamfiles, int type, char *filename);
void closeIdamFile(IDAMFILELIST *idamfiles, char *filename);
void closeIdamFiles(IDAMFILELIST *idamfiles);
void purgeStalestIdamFile(IDAMFILELIST *idamfiles);

#endif // IDAM_MANAGEFILES_H

