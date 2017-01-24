/*---------------------------------------------------------------
* IDAM Client Data Cache
*
*----------------------------------------------------------------
* Change History:
*
* 11Jun2012   D.G.Muir	Original Version

*----------------------------------------------------------------
Contents of DATA_BLOCK structures written to a local cache
Date/Time Stamp used to build in automatic obsolescence.
Unique hash key used to identify cached data using signal/source arguments
Cache located in directory given by environment variables IDAM_CACHE_DIR, IDAM_CACHE_TABLE
Maximum number of cached files prevents ...
Maximum size ...
Cache is communal: shared database table

Database table: 7 columns separated by a delimiting character ';'
Record 1 has table statistics: Record count, dead record count and eof offset.
The first record has a fixed length of 30 characters with a termination line feed

Columns: (each string is of arbitrary length)

unsigned int status
unsigned int Hash key
unsigned long long timestamp
char Filename[]
unsigned long long properties
char signal[]	- cannot contain a ';' character
char source[]
*/

#include "idamFileCache.h"

#include <fcntl.h>
#include <include/idamclientserver.h>

#include "idamclient.h"
#include "idamgenstruct.h"
#include "idamErrorLog.h"
#include "TrimString.h"
#include "struct.h"
#include "createClientXDRStream.h"

#ifndef SERVERBUILD
#  include "idamclientprivate.h"
#endif

//unsigned int xcrc32(const unsigned char *buf, int len, unsigned int init);

// Cache database table lock/unlock
// NULL File handle returned if there is no cache

int idamClientLockCache(FILE **db, short type) {

    // Types of Lock: F_RDLCK, F_WRLCK, or F_UNLCK

    FILE *fh;
    struct flock lock;
    struct timespec requested;
    struct timespec remaining;
    int err, rc, delay, count=0;

    // Open the Cache database table on locking and close on unlocking

    if(type == F_WRLCK) {					// Open the Database Table and apply an Exclusive lock
        char *dir   = getenv("IDAM_CACHE_DIR");		// Where the files are located
        char *table = getenv("IDAM_CACHE_TABLE");		// List of cached files

        *db = NULL;
        if(dir == NULL || table == NULL) return 0;	// No Cache? This is Not an Error!

        char *dbfile = (char *)malloc((strlen(dir)+strlen(table)+2)*sizeof(char));
        sprintf(dbfile, "%s/%s", dir, table);

        // Open the Cache database table

        errno = 0;
        fh = fopen(dbfile, "r+");	// ASCII file: Read with update (over-write)
        free((void *) dbfile);

        if(fh == NULL || errno != 0) {
            err = 999;
            if(errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamClientLockCache", errno, "");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientLockCache", err, "Unable to Open the Cache Database");
            if(fh != NULL) fclose(fh);
            return err;
        }

        *db = fh;				// Returned database table file handle

    } else

        fh = *db;


    int fd = fileno(fh);

    lock.l_whence = SEEK_SET;		// Relative to the start of the file
    lock.l_start  = 0;			// Lock applies from this byte
    lock.l_len    = 0;			// To the end of the file
    lock.l_type   = type;		// Lock type to apply

    rc = fcntl(fd, F_SETLK, &lock);  	// Manage the Cache table lock

    if(type == F_UNLCK) {
        *db = NULL;
        fclose(fh);	// Close the database table
    }

    if(rc == 0) return 0;		// Lock managed OK

    if(type == F_UNLCK) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientLockCache", err, "Cache file lock not released indicating problem with cache");
        return err;
    }

    // Problem obtaing an exclusive file lock - wait for a random time delay and try again

    delay = 1 + (int)(10.0 * (rand() / (RAND_MAX + 1.0)));	// Random delay 1->10
    requested.tv_sec  = 0;
    requested.tv_nsec = 1000*delay;				// Microsecs

    do {
        rc = nanosleep(&requested, &remaining);
        rc = fcntl(fd, F_SETLK, &lock);
    } while(rc == -1 && count++ < CACHE_MAXCOUNT);

    if(rc == -1 || count >= CACHE_MAXCOUNT) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientLockCache", err, "Unable to Lock the Cache Database");
        *db = NULL;
        fclose(fh);
        return err;
    }

    return 0;	// Lock obtained
}

// Time stamp test

int idamClientCacheTimeValid(unsigned long long timestamp) {
    struct timeval current;
    gettimeofday(&current, NULL);
    if(timestamp >= (unsigned long long)current.tv_sec) return 1;		// Timestamp OK
    return 0;
}

// Check the locked file time is within a reasonable time period - protect against possible bugs in file management

int idamClientCacheLockedTimeValid(unsigned long long timestamp) {
    struct timeval current;
    gettimeofday(&current, NULL);
    if((timestamp + CACHE_MAXLOCKTIME) >= (unsigned long long)current.tv_sec) return 1;		// Timestamp OK
    return 0;
}

// Test the File exists

int idamClientCacheFileValid(char *filename) {
    return 1;
}

// Current table statistics

int idamClientGetCacheStats(FILE *db, unsigned long *recordCount, unsigned long *deadCount, unsigned long *endOffset, char csvChar) {
    int err;
    char *csv, *next;
    char work[CACHE_FIRSTRECORDLENGTH+1];
    rewind(db);
    if(fgets(work, CACHE_FIRSTRECORDLENGTH, db) == NULL) {
        err = 999;
        //addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientCache", err, "Unable to Read the Cache Database");
        return err;
    }
    LeftTrimString(TrimString(work));
    next = work;
    if((csv = strchr(next,csvChar)) != NULL) csv[0] = '\0';	// Split the record into fields using delimiter character
    TrimString(next);
    LeftTrimString(next);
    *recordCount = (unsigned int)strtoul(next, NULL, 10);
    next = &csv[1];
    if((csv = strchr(next,csvChar)) != NULL) csv[0] = '\0';
    *deadCount = (unsigned int) strtoul(next, NULL, 10);
    next = &csv[1];
    *endOffset = (unsigned int) strtoul(next, NULL, 10);
    return 0;
}

// Update statistics

void idamClientUpdateCacheStats(FILE *db, unsigned long recordCount, unsigned long deadCount, unsigned long endOffset, char csvChar) {
    int i, lstr;
    char work[CACHE_FIRSTRECORDLENGTH+1];
    rewind(db);
    sprintf(work, "%lu%c%lu%c%lu", recordCount, csvChar, deadCount, csvChar, endOffset);		// Updated statistics record
    lstr = (int)strlen(work);
    for(i=lstr; i<CACHE_FIRSTRECORDLENGTH; i++) work[i] = ' ';					// Pack remaining record with space chars
    work[CACHE_FIRSTRECORDLENGTH-1] = '\n';
    work[CACHE_FIRSTRECORDLENGTH]   = '\0';
    fwrite(work, sizeof(char), CACHE_FIRSTRECORDLENGTH, db);					// Update
    fflush(db);
}

int idamClientPurgeCache(FILE *db, unsigned long recordCount, unsigned long *endOffset) {
    int i, lstr, count;
    unsigned short status;
    unsigned long dbkey, validRecordCount = 0;

    unsigned long long timestamp;
    char filename[MAXFILENAME];

    char csvChar = ';';
    char buffer[STRING_LENGTH];
    char *p, *csv, *next, *work;

    char **table;
    unsigned long long *timestamplist;

    char *dir = getenv("IDAM_CACHE_DIR");		// Where the files are located

    // Remove dead records to compact the database table

    fseek(db, CACHE_FIRSTRECORDLENGTH, SEEK_SET);		// rewind the file to the beginning of the second record

    table = (char **)malloc(recordCount*sizeof(char *));
    timestamplist = (unsigned long long *)malloc(recordCount*sizeof(unsigned long long));

    count = 0;
    while(count++ < recordCount && !feof(db) && fgets(buffer, STRING_LENGTH, db) != NULL) {	// Read each record

        LeftTrimString(TrimString(buffer));
        dbkey = 0;

        if(buffer[0] == '#') {			// Preserve comments
            table[validRecordCount] = (char *)malloc((sizeof(buffer)+1)*sizeof(char));
            strcpy(table[validRecordCount++], buffer);
            continue;
        }

        if(buffer[0] == '\n') continue;

        lstr = (int)strlen(buffer)+1;
        work = (char *)malloc(lstr*sizeof(char));
        strcpy(work, buffer);
        next = work;

        for(i=0; i<4; i++) {
            if((csv = strchr(next,csvChar)) != NULL) csv[0] = '\0';
            TrimString(next);
            LeftTrimString(next);
            switch(i) {
            case 0:	// Status
                status = (unsigned short)strtoul(next, NULL, 10);
                break;
            case 1:	// Hash key
                dbkey = (unsigned int)strtoul(next, NULL, 10);
                break;
            case 2:	// Time Stamp
                timestamp = strtoull(next, NULL, 10);
                break;
            case 3:	// Cached Filename
                strcpy(filename, next);
                if((p = strchr(filename,'\n')) != NULL) p[0] = '\0';	// Remove training line feed
                break;
            default:
                break;
            }
            if(status == CACHE_DEADRECORD) break;	// Skip this record - it's marked as dead! purge by ignoring it
            if(csv != NULL) next = &csv[1];	// Next element starting point
        }
        if(dbkey != 0) {				// Check the records is valid (always if locked by a process)
            if((status == CACHE_LOCKEDRECORD && idamClientCacheLockedTimeValid(timestamp)) ||
                    (idamClientCacheTimeValid(timestamp) && idamClientCacheFileValid(filename))) {
                table[validRecordCount] = work;			// reuse this heap allocation
                timestamplist[validRecordCount] = timestamp;	// Sort on this to purge oldest records
                strcpy(table[validRecordCount++], buffer);
            } else {
                char *dbfile = (char *)malloc((strlen(dir)+strlen(filename)+2)*sizeof(char));
                sprintf(dbfile, "%s/%s", dir, filename);
                remove(dbfile);					// Delete the cached file and ignore the record - purge
                free((void *) dbfile);
            }
        }
    }

    // Drop the oldest record?

    if(validRecordCount > CACHE_MAXRECORDS-1) validRecordCount = CACHE_MAXRECORDS-1;

    // Write the compacted table in time order (youngest first) following a sort on the timestamp

    fseek(db, CACHE_FIRSTRECORDLENGTH, SEEK_SET);	// Position at the start of record 2

    errno = 0;
    for(i=0; i<validRecordCount; i++) {
        lstr = (int)strlen(table[i]);
        count = fwrite(table[i], sizeof(char), lstr, db);	// Write all valid records
        if(count != lstr || errno != 0) {
            int err = 999;
            //if(errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamClientCache", errno, "");
            //addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientCache", err, "Unable to Open the Cache Database");
            return -err;
        }

        free(table[i]);
    }
    //free(table);
    //free(timestamplist);

    *endOffset = ftell(db);			// Append or overwrite new records from this location (End of active records)

    fwrite("\n", sizeof(char), 1, db);		// Insert a new line at the end of the valid set of records


    // Update statistics

    idamClientUpdateCacheStats(db, validRecordCount, 0, *endOffset, csvChar);

    return validRecordCount;

}

int idamClientReadCache(REQUEST_BLOCK *request_block, DATA_BLOCK *data_block, char *filename) {
    int err, rc = 0;
    XDR XDRInput;
    XDR *xdrs = clientInput;
    FILE *xdrfile = NULL;
    void *data = NULL;

    err = 0;

    if(filename == NULL || filename[0] == '\0') return 0;

    do {		// Error Trap

        // Create input xdr file stream

        errno = 0;

        if((xdrfile = fopen(filename, "rb")) == NULL || errno != 0) {	// Read cached file
            err = 999;
            if(errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamClientReadCache", errno, "");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientReadCache", err, "Unable to Open the Cached Data File");
            break;
        }

        xdr_destroy(xdrs);	// *** xdrs not initialised?
        XDRstdioFlag = 1;
        xdrstdio_create(&XDRInput, xdrfile, XDR_DECODE);
        xdrs = &XDRInput;						// Switch from stream to file

        // Initialise structure passing mechanism

        logmalloclist = (LOGMALLOCLIST *)malloc(sizeof(LOGMALLOCLIST));
        initLogMallocList(logmalloclist);
        userdefinedtypelist = (USERDEFINEDTYPELIST *)malloc(sizeof(USERDEFINEDTYPELIST));
        USERDEFINEDTYPE *udt_received = (USERDEFINEDTYPE *)malloc(sizeof(USERDEFINEDTYPE));
        initUserDefinedTypeList(userdefinedtypelist);

        rc = xdr_userdefinedtypelist(xdrs, userdefinedtypelist);		// receive the full set of known named structures

        if(!rc) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientReadCache", err, "Failure receiving Structure Definitions");
            break;
        }

        initUserDefinedType(udt_received);

        rc = rc && xdrUserDefinedTypeData(xdrs, udt_received, &data);		// receive the Data

        if(!rc) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientReadCache", err, "Failure receiving Data and Structure Definition");
            break;
        }

        // Close the stream and file

        fflush(xdrfile);
        fclose(xdrfile);

        // Switch back to the normal xdr record stream

        idamCreateXDRStream();
        xdrs = clientInput;
        XDRstdioFlag = 0;

        if(!strcmp(udt_received->name,"SARRAY")) {			// expecting this carrier structure

            GENERAL_BLOCK *general_block = (GENERAL_BLOCK *)malloc(sizeof(GENERAL_BLOCK));

            SARRAY *s = (SARRAY *)data;
            if(s->count != data_block->data_n) {				// check for consistency
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientReadCache", err, "Inconsistent S Array Counts");
                break;
            }
            data_block->data         = (char *) fullNTree;		// Global Root Node with the Carrier Structure containing data
            data_block->opaque_block = (void *) general_block;
            general_block->userdefinedtype     = udt_received;
            general_block->userdefinedtypelist = userdefinedtypelist;
            general_block->logmalloclist       = logmalloclist;
            general_block->lastMallocIndex     = 0;
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientReadCache", err, "Name of Received Data Structure Incorrect");
            break;
        }

    } while(0);

    return err;
}

// Identify the name of the required cache file

int idamClientGetCacheFilename(REQUEST_BLOCK *request_block, char **cacheFilename) {
    int i, rc = 0;
    unsigned short status, outcome;
    unsigned long position, endOffset;
    unsigned long recordCount, deadCount;
    FILE *db = NULL;

    unsigned int key, dbkey;

    unsigned long long timestamp;
    char filename[MAXFILENAME];
    //unsigned long long properties = 0;

    char csvChar = ';';
    char buffer[STRING_LENGTH];
    char signal[STRING_LENGTH];
    char source[STRING_LENGTH];
    char *p, *csv, *next;

    // Lock the database

    if((rc = idamClientLockCache(&db, F_WRLCK)) != 0 || db == NULL) return rc;

    // Generate a Hash Key (not guaranteed unique)

    key = -1;
    key = xcrc32((const unsigned char *)request_block->signal,(int)strlen(request_block->signal),key);	// combine CRC has keys
    key = xcrc32((const unsigned char *)request_block->source,(int)strlen(request_block->source),key);

    // Sequentially read all records and test against the hash key (small file so not that inefficient)

    outcome = CACHE_NORECORD;

    // Table stats

    idamClientGetCacheStats(db, &recordCount, &deadCount, &endOffset, csvChar);

    fseek(db, CACHE_FIRSTRECORDLENGTH, SEEK_SET);	// Position at the start of record 2
    position = ftell(db);

    while(!feof(db) && fgets(buffer, STRING_LENGTH, db) != NULL) {
        LeftTrimString(TrimString(buffer));
        do {
            dbkey = 0;
            if(buffer[0] == '#') break;
            if(buffer[0] == '\n') break;
            next = buffer;

            for(i=0; i<7; i++) {
                if((csv = strchr(next,csvChar)) != NULL) csv[0] = '\0';	// Split the record into fields using delimiter character
                TrimString(next);
                LeftTrimString(next);
                switch(i) {
                case 0:	// Status
                    status = (unsigned short)strtoul(next, NULL, 10);
                    break;
                case 1:	// Hash key
                    dbkey = (unsigned int)strtoul(next, NULL, 10);
                    break;
                case 2:	// Time Stamp
                    timestamp = strtoull(next, NULL, 10);
                    break;
                case 3:	// Cached Filename
                    strcpy(filename, next);
                    if((p = strchr(filename,'\n')) != NULL) p[0] = '\0';	// Remove training line feed
                    break;
                case 4:	// Properties
                    //properties = (unsigned long long)strtoull(next, NULL, 10);
                    break;
                case 5:	// signal
                    strcpy(signal, next);
                    if((p = strchr(signal,'\n')) != NULL) p[0] = '\0';
                    break;
                case 6:	// source
                    strcpy(source, next);
                    if((p = strchr(source,'\n')) != NULL) p[0] = '\0';
                    break;
                default:
                    break;
                }
                if(status == CACHE_DEADRECORD) break;	// Don't need the remaining information on this record!
                if(csv != NULL) next = &csv[1];		// Next element starting point
            }
        } while(0);

        if(status == CACHE_DEADRECORD || buffer[0] == '#' || buffer[0] == '\n') continue;

        if(key == dbkey) { 			// Key found: Use this record if within time limitation and the file exists
            if(idamClientCacheTimeValid(timestamp) && idamClientCacheFileValid(filename)) {
                if((strcmp(request_block->signal, signal) != 0) && (strcmp(request_block->source, source) !=0)) {		// Additional Check
                    outcome = CACHE_RECORDFOUND;
                    break;
                } 		//  keep searching as signal and source strings do not match - possible hash key duplicate
            } else {
                if(status == CACHE_LIVERECORD ||
                        (status == CACHE_LOCKEDRECORD && idamClientCacheLockedTimeValid(timestamp))) {	// Record needs purging but only if not locked
                    outcome = CACHE_PURGERECORD;
                    deadCount++;
                    break;
                }
            }
        }

        position = ftell(db);	// Start of next record
    }


    if(outcome == CACHE_RECORDFOUND) {		// Record found and Timestamp OK

        int lstr = (int)strlen(filename)+1;
        *cacheFilename = (char *)malloc(lstr*sizeof(char));
        strcpy(*cacheFilename, filename);

// Lock the cache file record using the status value only - not a record lock on the table

        fseek(db, position, SEEK_SET); 	// Goto start of last record read and change status to LOCKED
        sprintf(buffer, "%d%c", CACHE_LOCKEDRECORD, csvChar);
        fwrite(buffer, sizeof(char), 2, db);

    } else

        if(outcome == CACHE_PURGERECORD) {

// Cache has expired. Remove table entry and delete the data file

            fseek(db, position, SEEK_SET); 	// Goto start of last record read and mark for removal: status changed to DEAD
            sprintf(buffer, "%d%c", CACHE_DEADRECORD, csvChar);
            fwrite(buffer, sizeof(char), 2, db);

// If there are too many dead records, then compact the database table

            if(deadCount >= CACHE_MAXDEADRECORDS) {
                recordCount = idamClientPurgeCache(db, recordCount, &endOffset);
                deadCount = 0;
            } else {
                char *dir   = getenv("IDAM_CACHE_DIR");		// Where the files are located
                char *dbfile = (char *)malloc((strlen(dir)+strlen(filename)+2)*sizeof(char));	// Erase cache file
                sprintf(dbfile, "%s/%s", dir, filename);
                remove(dbfile);
                free((void *) dbfile);
            }

            idamClientUpdateCacheStats(db, recordCount, deadCount, endOffset, csvChar);

        }

// Free	the Lock

    rc = idamClientLockCache(&db, F_UNLCK);	// release lock

    return rc;
}


// Write a new record

int idamClientWriteCache(char *filename) {
    int err, rc = 0;
    FILE *db = NULL;

    REQUEST_BLOCK *request_block = request_block_ptr;	// Global pointer to the request block

    unsigned short status = CACHE_LIVERECORD;
    unsigned long key, recordCount, deadCount, endOffset;

    unsigned long long timestamp;
    unsigned long long properties = 0;

    char csvChar = ';';
    char buffer[STRING_LENGTH];

// Generate a Hash Key (not guaranteed unique)

    key = -1;
    key = xcrc32((const unsigned char *)request_block->signal,(int)strlen(request_block->signal),key);	// combine CRC has keys
    key = xcrc32((const unsigned char *)request_block->source,(int)strlen(request_block->source),key);

// Generate a timestamp

    struct timeval current;
    gettimeofday(&current, NULL);

    timestamp = (unsigned long long)current.tv_sec + CACHE_HOURSVALID *3600 + 60;

// Create the new record string

    sprintf(buffer, "%d%c%lu%c%llu%c%s%c%llu%c%s%c%s\n", status,csvChar, key, csvChar,
            timestamp, csvChar, filename, csvChar, properties, csvChar, request_block->signal,
            csvChar, request_block->source);

// Append the new record to the database table

    if((rc = idamClientLockCache(&db, F_WRLCK)) != 0 || db == NULL) return rc;

// Current table statistics

    if((err = idamClientGetCacheStats(db, &recordCount, &deadCount, &endOffset, csvChar)) != 0) {
        rc = idamClientLockCache(&db, F_UNLCK);	// Free	the Lock and File
        return err;
    }

// Too many records when the new record is added?

    if(recordCount >= CACHE_MAXRECORDS-1 || deadCount >= CACHE_MAXDEADRECORDS) {		// purge dead records + oldest active record
        recordCount = idamClientPurgeCache(db, recordCount, &endOffset);
        deadCount = 0;
    }

// Append a new record at the file position (always the end of the valid set of records)

    if(endOffset == 0) {
        fseek(db, 0, SEEK_END);
        endOffset = ftell(db);
    }

    fseek(db, endOffset, SEEK_SET);
    fwrite(buffer, sizeof(char), strlen(buffer), db);
    endOffset = ftell(db);
    fwrite("\n", sizeof(char), 1, db);

    fflush(db);
    recordCount++;

// Update statistics

    idamClientUpdateCacheStats(db, recordCount, deadCount, endOffset, csvChar);

    idamClientLockCache(&db, F_UNLCK);	// release lock

    return 0;

}

/*
//=========================================================================================
// Test the cache

   lock the cache database using fcntl (wait with random delay until lock is free if locked)
   query the table for the required data

   if the data is available from the cache
        test the timestamp
	if timestamp OK
		read the data
		free the lock
	else
		remove database entry
		remove cached file
		free the lock
		access the original data source
   else
	free the lock
	access the original data source

   if new data accessed
   	lock the cache database
	add new record
	write new cache file
	free the lock
*/

//=========================================================================================
/* crc32.c
   Copyright (C) 2009, 2011 Free Software Foundation, Inc.

   This file is part of the libiberty library.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   In addition to the permissions in the GNU General Public License, the
   Free Software Foundation gives you unlimited permission to link the
   compiled version of this file into combinations with other programs,
   and to distribute those combinations without any restriction coming
   from the use of this file.  (The General Public License restrictions
   do apply in other respects; for example, they cover modification of
   the file, and distribution when not linked into a combined
   executable.)

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
*/

//#ifdef HAVE_CONFIG_H
//#include "config.h"
//#endif

// #include "libiberty.h"

/* This table was generated by the following program.  This matches
   what gdb does.

   #include <stdio.h>

   int
   main ()
   {
     int i, j;
     unsigned int c;
     int table[256];

     for (i = 0; i < 256; i++)
       {
	 for (c = i << 24, j = 8; j > 0; --j)
	   c = c & 0x80000000 ? (c << 1) ^ 0x04c11db7 : (c << 1);
	 table[i] = c;
       }

     printf ("static const unsigned int crc32_table[] =\n{\n");
     for (i = 0; i < 256; i += 4)
       {
	 printf ("  0x%08x, 0x%08x, 0x%08x, 0x%08x",
		 table[i + 0], table[i + 1], table[i + 2], table[i + 3]);
	 if (i + 4 < 256)
	   putchar (',');
	 putchar ('\n');
       }
     printf ("};\n");
     return 0;
   }

   For more information on CRC, see, e.g.,
   http://www.ross.net/crc/download/crc_v3.txt.  */

static const unsigned int crc32_table[] =
{
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

/*

@deftypefn Extension {unsigned int} crc32 (const unsigned char *@var{buf}, @
  int @var{len}, unsigned int @var{init})

Compute the 32-bit CRC of @var{buf} which has length @var{len}.  The
starting value is @var{init}; this may be used to compute the CRC of
data split across multiple buffers by passing the return value of each
call as the @var{init} parameter of the next.

This is intended to match the CRC used by the @command{gdb} remote
protocol for the @samp{qCRC} command.  In order to get the same
results as gdb for a block of data, you must pass the first CRC
parameter as @code{0xffffffff}.

This CRC can be specified as:

  Width  : 32
  Poly   : 0x04c11db7
  Init   : parameter, typically 0xffffffff
  RefIn  : false
  RefOut : false
  XorOut : 0

This differs from the "standard" CRC-32 algorithm in that the values
are not reflected, and there is no final XOR value.  These differences
make it easy to compose the values of multiple blocks.

@end deftypefn

*/

unsigned int
xcrc32 (const unsigned char *buf, int len, unsigned int init)
{
    unsigned int crc = init;
    while (len--)
    {
        crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
        buf++;
    }
    return crc;
}


