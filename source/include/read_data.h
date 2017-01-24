//! $LastChangedRevision: 54 $
//! $LastChangedDate: 2008-06-23 11:26:51 +0100 (Mon, 23 Jun 2008) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/include/read_data.h $

/*******************************************************
 *            Header File for read_data                *
 *******************************************************
 * created 10/11/2000 by Hendrik Meyer                 *
 * 20/05/2004 free_data_struct helper routine added    *
 *******************************************************/

// 02Apr2008 dgm    C++ test added for inclusion of extern "C"
//--------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

/* Needed header files */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

//--------------------------------- Changes for Data Engine -----------
// #include "ida3.h"
#include "/usr/local/fusion/ida/ida3.h"
//---------------------------------------------------------------------

/* Constants */
#define LFILE 256 /* length of filenames */
#define LPATH 1000 /* length of search path */
#define F_LENGTH 5001  /* length of feature vector 
              last word contains information whether the feature 
              is known, or unknown */
#define F_DT 1e-3      /* time interval for feature item */

/* Type and structure definitions */


typedef struct {
    float     number;
    char      date[11];
} VERSION;

typedef struct {
    char filename[IDA_FSIZE+1];
    char date[9];
    char time[9];
    short machine;
    short type;
    long shotnr;
    char itemname[256];
    float devoff;
    float devrng;
    short devres;
    float calfac;
    float caloff;
    long nod;
    unsigned short datpck;
} INFO;

typedef struct {
    double  *  vector;
    char      label[IDA_USIZE+IDA_LSIZE+4];
} AXIS;

typedef struct {
    int       error;
    VERSION version;
    INFO      info;
    double  **  * data;
    char      label[IDA_USIZE+IDA_LSIZE+4];
    long      dimensions[3];
    AXIS      xaxis;
    AXIS      yaxis;
    AXIS      taxis;
    double  **  * data_error;
} DATA;

/* function headers */

int search_path(char *, char *,char ** *);

int ida3_name(long, char *, char *);

DATA read_data(long, char *);

DATA read_item(char *, ida_file_ptr *, short *); /* reads single ida item */

int errmsg(ida_err); /* prints out error messages */

double ** * calloc_field(long,long,long); /* allocates a field with l,n,m
                       indices*/

void get_units(const char *,char *,char *);

void set_label(char *, char *,char *);

int write_data(long, char *, ida_file_ptr *, DATA);

int write_status(long, ida_file_ptr *, int, const char *);

int write_passnr(long, ida_file_ptr *, int, const char *);

int write_features(long, ida_file_ptr *, long [F_LENGTH], const char *);

int write_version(long, ida_file_ptr *, double, const char *);

int write_data_idl(int , void **);

int rd_write_netcdf(long, const char *, int, DATA, int); /* write NETCDF data*/

void free_data_struct( DATA); /* Free pointers in data structure */
/* EOF */

#ifdef __cplusplus
}
#endif
