#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "ual_low_level.h"
#include "ual_low_level_mdsplus.h"

#define MAX_OPEN_EXP 10000
#define MAX_EXP_NAME 100
#define IS_MDS    1
#define IS_HDF5   2
#define IS_PUBLIC 3
#define IS_NONE   0

//--------------- Management of the list of objects (arrays of structures) -----------
#define MAX_OBJECTS 10000000   // maximum number of simultaneous objects

#ifndef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, 0, PTHREAD_MUTEX_ERRORCHECK_NP, 0, { 0, 0 } } }
#endif

static void* object[MAX_OBJECTS];       // array providing the correspondance between an integer object index and the real (void *) object pointer (for use in non C languages)
static int current_obj = -1;            // index of the last used object
//static pthread_mutex_t obj_mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP; // lock to avoid simultaneous access to the object arrays
static pthread_mutex_t obj_mutex = PTHREAD_MUTEX_INITIALIZER; // lock to avoid simultaneous access to the object arrays


// lock the object array
static void lock_obj()
{
    int status = pthread_mutex_lock(&obj_mutex);
    if (status) {
        printf("Error while locking list of objects\n");
        exit(EXIT_FAILURE);
    }
}

// unlock the object array
static void unlock_obj()
{
    int status = pthread_mutex_unlock(&obj_mutex);
    if (status) {
        printf("Error while unlocking list of objects\n");
        exit(EXIT_FAILURE);
    }
}

// clear object list before first use
static void initializeObjectList()
{
    int i;
    for (i = 0; i < MAX_OBJECTS; i++) {
        object[i] = 0;
    }
}

// get object pointer from its index
void* getObjectFromList(int idx)
{
    void* result;
    lock_obj();
    if (current_obj == -1 || !object[idx]) {  // the specified object does no exist
        printf("Error: trying to access unknown object\n");
        exit(EXIT_FAILURE);
    }
    result = object[idx];
    unlock_obj();
    return result;
}

// change the pointer associated to an index
void replaceObjectInList(int idx, void* obj)
{
    lock_obj();
    if (current_obj == -1 || !object[idx]) {  // the specified object does no exist
        printf("Error: trying to access unknown object\n");
        exit(EXIT_FAILURE);
    }
    object[idx] = obj;
    unlock_obj();
}

// add a new object pointer to the list
int addObjectToList(void* obj)
{
    int new_obj;
    lock_obj();
    if (current_obj == -1) {  // is it the first time we use the list?
        initializeObjectList(); // then clear the list
    }

    // find first unused location in the list
    new_obj = current_obj;
    do {
        new_obj++;
        if (new_obj >= MAX_OBJECTS) new_obj = 0;
    } while (object[new_obj] && new_obj != current_obj);

    // could not find any empty location?
    if (object[new_obj]) {
        return -1;
    }

    // assign the object to the found location
    object[new_obj] = obj;
    current_obj = new_obj;
    unlock_obj();
    return new_obj;
}

// delete an object pointer from the list
void removeObjectFromList(int idx)
{
    lock_obj();
    if (current_obj == -1 || !object[idx]) {  // the specified object does no exist
        printf("Error: trying to remove unknown object\n");
        exit(EXIT_FAILURE);
    }
    object[idx] = 0;
    unlock_obj();
}

//------------------------------- End of object management ---------------------------

//------------------------------Logging support---------------------------------------
//#define DEBUG_UAL 1

void logOperation(char* name, char* cpoPath, char* path, int expIdx, int dim1, int dim2, int dim3, int dim4, int dim5,
                  int dim6, int
                  dim7, void* data, double isTimed)
{
#ifdef DEBUG_UAL
    FILE *f = fopen("ual.txt", "a");
    fprintf(f, "%s ", name);
    if(cpoPath)
        fprintf(f, "%s ", cpoPath);
    if(path)
        fprintf(f, "%s ", path);
    fprintf(f, "%d\n", expIdx);
    fclose(f);
#endif
}

//---------------------------------------------------------------------------------

char* getMajorImasVersion(char* imasVersion)
{
    char* imasMajor_start = imasVersion;
    char* imasMajor_end;
    char* imasMajor;

    imasMajor_end = strchr(imasVersion, '.');
    if (imasMajor_end == NULL) {
        return imasVersion;
    }

    imasMajor = (char*)malloc(imasMajor_end - imasMajor_start + 1);

    memcpy(imasMajor, imasMajor_start, imasMajor_end - imasMajor_start);
    *(imasMajor + (imasMajor_end - imasMajor_start)) = '\0';

    return imasMajor;
}

//---------------------------------------------------------------------------------
static char message[10000];
char* errmsg = message;

static struct {
    char expName[MAX_EXP_NAME];
    int shot;
    int run;
    int idx;
    int mode;
} expInfo[MAX_OPEN_EXP];

char* imas_last_errmsg()
{
    return errmsg;
}

char* imas_reset_errmsg()
{
    message[0] = '\0';
    return errmsg;
}

static int getShot(int idx)
{
    return expInfo[idx].shot;
}

int ual_get_shot(int idx) { return getShot(idx); }