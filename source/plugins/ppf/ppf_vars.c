#include "ppf_vars.h"

#include <string.h>
#include <stdarg.h>

#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <clientserver/stringUtils.h>

#include "ppf.h"

extern FILE * g_dbgout;

int printf_(FILE * fp, const char * fmt, ...)
{
    va_list list;
    int i = 0;

    if (fp) {
        fprintf(fp, ":dsand:");

        va_start(list, fmt);
        i = vfprintf(fp, fmt, list);
        va_end(list);
    }
    return i;
}

int nvl_count(const NAMEVALUELIST * nvl)
{
    return nvl->pairCount;
}

const NAMEVALUE * nvl_pair(const NAMEVALUELIST * nvl, int idx)
{
    return nvl->nameValue + idx;
}

const char * nv_name(const NAMEVALUE * nv)
{
    return nv->name;
}

const char * nv_value(const NAMEVALUE * nv)
{
    return nv->value;
}

const NAMEVALUE * nvl_name_find(const NAMEVALUELIST * nvl, const char * name)
{
    const NAMEVALUE * the_nv = NULL;
    int i = 0;
    int n = nvl_count(nvl);

    for (i = 0; !the_nv && i < n; ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);

        if (STR_EQUALS(name, nv_name(nv)))
            the_nv = nv;
    }

    return the_nv;
}

int PPFGO_init(VAR_PPFGO * var)
{
    int err = 0;
    var->shot = 0;
    var->seq = 0;
    var->err = 0;
    return err;
}


int PPFGO_setup(VAR_PPFGO * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    unsigned int err = 0;
    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        //const char* val = nv_value( nv );

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS(STR_SHOT, name)) {
            var->shot = atoi(name);
        }
        else if (STR_EQUALS(STR_SEQ, name)) {
            var->seq = atoi(name);
        }
        else
            err = ERR_PPFGO_SETUP;
    }
    return err;
}


int PPFGO_execute(VAR_PPFGO * var)
{
    int err = 0;

    int * shot = &(var->shot);
    int * seq = &(var->seq);
    int lunit = 0;

    PPFGO(shot, seq, &lunit, &(var->err));
    return err;
}

int PPFGO_DATABLOCK(const VAR_PPFGO * var, DATA_BLOCK * db)
{
    int err = 0;
    initDataBlock(db);

    int * pData = (int *) malloc(1 * sizeof(int));
    pData[0] = var->err;


    db->data_n = 1;

    db->data_type = UDA_TYPE_INT;
    db->data = (char *) pData;

    return err;
}


int PDMSEQ_init(VAR_PDMSEQ * f)
{
    int err = 0;
    f->pulse = 0;
    f->seq = 0;
    f->err = 0;
    memset(f->szOwner, 0, 9);
    memset(f->szDDA, 0, 5);
    return err;
}


int PDMSEQ_setup(VAR_PDMSEQ * f, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;
    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS(STR_PULSE, name))
            f->pulse = atoi(val);
        else if (STR_EQUALS(STR_SEQ, name))
            f->seq = atoi(val);
        else if (STR_EQUALS(STR_OWNER, name))
            strcpy(f->szOwner, val);
        else if (STR_EQUALS(STR_DDA, name))
            strcpy(f->szDDA, val);
        else {
            err = ERR_PDMSEQ_SETUP;
        }
    }
    return err;
}

int PDMSEQ_execute(VAR_PDMSEQ * f)
{
    int err = 0;

    int * pulse = &(f->pulse);
    int * seq = &(f->seq);

    char * pszOwner = f->szOwner;
    char * pszDDA = f->szDDA;

    int * f_err = &(f->err);

    PDMSEQ(pulse, pszOwner, pszDDA, seq, f_err, strlen(pszOwner), strlen(pszDDA));

    return err;
}

int PDMSEQ_DATABLOCK(const VAR_PDMSEQ * var, DATA_BLOCK * db)
{
    int err = 0;
    initDataBlock(db);

    int * pData = (int *) malloc(2 * sizeof(int));
    pData[0] = var->err;
    pData[1] = var->seq;

    db->data_n = 2;

    db->data_type = UDA_TYPE_INT;
    db->data = (char *) pData;

    return err;
}


int PPFUID_init(VAR_PPFUID * var)
{
    int err = 0;
    var->err = 0;
    memset(var->szUID, 0, 9);
    strcpy(var->rw, "R");
    return err;
}

int PPFUID_setup(VAR_PPFUID * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;

    var->err = 0;

    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS(STR_USERID, name))
            strcpy(var->szUID, val);
        else if (STR_EQUALS(STR_RW_FLAG, name))
            strcpy(var->rw, val);
        else
            err = ERR_PPFUID_SETUP;
    }

    return err;
}

int PPFUID_execute(VAR_PPFUID * var)
{
    int err = 0;

    PPFUID(var->szUID, var->rw, strlen(var->szUID), strlen(var->rw));

    return err;
}

int PPFUID_DATABLOCK(const VAR_PPFUID * var, DATA_BLOCK * db)
{
    int err = 0;
    initDataBlock(db);

    int * pData = (int *) malloc(1 * sizeof(int));
    pData[0] = var->err;
    //pData[1] = var->seq;

    db->data_n = 1;

    db->data_type = UDA_TYPE_INT;
    db->data = (char *) pData;
    return err;
}

int PPFCLO_init(VAR_PPFCLO * var)
{
    int err = 0;
    var->shot = 0;
    var->seq = 0;
    var->unit = 0;
    var->pvers = 0;
    var->err = 0;
    memset(var->prgname, 0, 9);
    return err;
}

int PPFCLO_setup(VAR_PPFCLO * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;
    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS(STR_SHOT, name))
            var->shot = atoi(val);
        else if (STR_EQUALS(STR_SEQ, name))
            var->seq = atoi(val);
        else if (STR_EQUALS(STR_PRGNAME, name))
            strcpy(var->prgname, val);
        else if (STR_EQUALS(STR_PVERS, name))
            var->pvers = atoi(val);
        else {
            err = ERR_PPFCLO_SETUP;
        }
    }
    return err;
}

int PPFCLO_execute(VAR_PPFCLO * var)
{
    int err = 0;
    int * shot = &(var->shot);
    int * seq = &(var->shot);
    int unit = 0;
    char * prgnam = var->prgname;
    int * pvers = &(var->pvers);

    *seq = -1;

    PPFCLO(shot, seq, &unit, prgnam, pvers, &(var->err), strlen(prgnam));
    return err;
}

int PPFCLO_DATABLOCK(const VAR_PPFCLO * var, DATA_BLOCK * db)
{
    int err = 0;
    initDataBlock(db);

    int * pData = (int *) malloc(2 * sizeof(int));
    pData[0] = var->err;
    pData[1] = var->seq;

    db->data_n = 2;

    db->data_type = UDA_TYPE_INT;
    db->data = (char *) pData;
    return err;
}

int PPFERR_init(VAR_PPFERR * var)
{
    int err = 0;
    memset(var->rname, 0, 7);
    memset(var->msg, 0, ERR_MSG_LEN);
    var->msg[ERR_MSG_LEN] = '\0';

    var->err_code = 0;
    var->err = 0;
    return err;
}

int PPFERR_setup(VAR_PPFERR * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;

    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS(STR_RNAME, name))
            strcpy(var->rname, val);
        else if (STR_EQUALS(STR_ERRCODE, name))
            var->err_code = atoi(val);
        else {
            err = ERR_PPFERR_SETUP;
        }
    }

    return err;
}

int PPFERR_execute(VAR_PPFERR * var)
{
    int err = 0;
    memset(var->msg, 0, ERR_MSG_LEN);
    PPFERR(var->rname, &(var->err_code), var->msg, &(var->err), strlen(var->rname), ERR_MSG_LEN);

    return err;
}

int PPFERR_DATABLOCK(const VAR_PPFERR * var, DATA_BLOCK * db)
{
    int err = 0;
    initDataBlock(db);

    int l = strlen(var->msg);
    char * pszData = malloc(l + 1);

    strcpy(pszData, var->msg);

    db->data_type = UDA_TYPE_STRING;
    db->data = pszData;
    db->data_n = l + 1;

    return err;
}

int PPFSQI_init(VAR_PPFSQI * var)
{
    int err = 0;

    var->shot = 0;
    var->pqi = 0;
    var->err = 0;

    return err;
}

int PPFSQI_setup(VAR_PPFSQI * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;

    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS(STR_SHOT, name))
            var->shot = atoi(val);
        else if (STR_EQUALS(STR_PQI, name))
            var->pqi = atoi(val);

        else {
            err = ERR_PPFSQI_SETUP;
        }
    }

    return err;
}

int PPFSQI_execute(VAR_PPFSQI * var)
{
    int err = 0;

    PPFSQI(&(var->shot), &(var->pqi), &(var->err));

    return err;
}


int PPFSQI_DATABLOCK(const VAR_PPFSQI * var, DATA_BLOCK * db)
{
    int * pData = (int *) malloc(sizeof(int));
    int err = 0;

    initDataBlock(db);

    pData[0] = var->err;

    db->data_type = UDA_TYPE_INT;
    db->data = (char *) pData;
    db->data_n = 1;

    return err;
}


int PPFGQI_init(VAR_PPFGQI * var)
{
    int err = 0;

    var->shot = 0;
    var->pqi = 0;
    var->err = 0;

    return err;
}

int PPFGQI_setup(VAR_PPFGQI * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;

    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS(STR_SHOT, name))
            var->shot = atoi(val);
        else {
            err = ERR_PPFGQI_SETUP;
        }
    }

    return err;
}

int PPFGQI_execute(VAR_PPFGQI * var)
{
    int err = 0;

    PPFGQI(&(var->shot), &(var->pqi), &(var->err));

    return err;
}

int PPFGQI_DATABLOCK(const VAR_PPFGQI * var, DATA_BLOCK * db)
{
    int err = 0;

    int * pData = (int *) malloc(2 * sizeof(int));

    initDataBlock(db);

    pData[0] = var->err;
    pData[1] = var->pqi;

    db->data_type = UDA_TYPE_INT;
    db->data = (char *) pData;
    db->data_n = 2;

    return err;
}

int PPFGID_init(VAR_PPFGID * var)
{

    int err = 0;
    memset(var->userid, 0, 9);
    strcpy(var->rw, "r");
    return err;
}

int PPFGID_setup(VAR_PPFGID * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;
    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS(STR_RW_FLAG, name))
            strcpy(var->userid, val);
        else {
            err = ERR_PPFGID_SETUP;
        }
    }
    return err;
}

int PPFGID_execute(VAR_PPFGID * var)
{
    int err = 0;
    PPFGID(var->userid, var->rw, strlen(var->userid), 1);
    var->err = 0;
    return err;
}

int PPFGID_DATABLOCK(const VAR_PPFGID * var, DATA_BLOCK * db)
{
    int err = 0;

    int l = strlen(var->userid);
    char * pData = malloc(l + 1);

    strcpy(pData, var->userid);

    initDataBlock(db);

    db->data_type = UDA_TYPE_STRING;
    db->data = pData;
    db->data_n = l + 1;

    return err;
}

int PPFPOK_init(VAR_PPFPOK * var)
{
    int err = 0;
    var->err = 0;
    return err;
}

int PPFPOK_setup(VAR_PPFPOK * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;
    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else {
            err = ERR_PPFPOK_SETUP;
        }
    }

    return err;
}

int PPFPOK_execute(VAR_PPFPOK * var)
{
    int err = 0;

    PPFPOK(&(var->err));

    return err;
}

int PPFPOK_DATABLOCK(const VAR_PPFPOK * var, DATA_BLOCK * db)
{
    int err = 0;
    int * pData = (int *) malloc(sizeof(int));

    initDataBlock(db);

    pData[0] = var->err;
    //pData[1] = var->pqi;

    db->data_type = UDA_TYPE_INT;
    db->data = (char *) pData;
    db->data_n = 1;

    return err;
}

int PDAINF_init(VAR_PDAINF * var)
{
    int err = 0;

    memset(var->prgnam, 0, PRGNAM_LEN + 1);
    memset(var->ppfnam, 0, PPFNAM_LEN + 1);
    var->version = -1;
    var->err = 0;

    return err;
}

int PDAINF_setup(VAR_PDAINF * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;
    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        //const char* val = nv_value( nv );

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else {
            err = ERR_PDAINF_SETUP;
        }
    }

    return err;
}

int PDAINF_execute(VAR_PDAINF * var)
{
    int err = 0;

    PDAINF(var->prgnam, &(var->version), var->ppfnam, &(var->err), PRGNAM_LEN, PPFNAM_LEN);

    return err;
}


int PDAINF_DATABLOCK(const VAR_PDAINF * var, DATA_BLOCK * db)
{
    int i;
    int err = 0;

    int * pErrData = (int *) malloc(sizeof(int));
    *pErrData = var->err;

    initDataBlock(db);

    db->data_type = UDA_TYPE_INT;
    db->data = (char *) pErrData;
    db->data_n = 1;

    db->rank = 2;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; ++i) {
        const char * the_psz = (0 == i) ? var->prgnam : var->ppfnam;

        initDimBlock(&(db->dims[i]));

        int l = strlen(the_psz);

        char * pDimData = malloc((l + 1));
        strcpy(pDimData, the_psz);
        pDimData[l] = 0;

        db->dims[i].dim = pDimData;
        db->dims[i].dim_n = l + 1;
    }

    return err;
}

int PPFSEQ_init(VAR_PPFSEQ * var)
{
    int err = 0;
    var->shot = 0;
    var->err = 0;
    var->pSeqNums = NULL;
    var->pModDates = NULL;
    var->pModTimes = NULL;
    var->pDDAs = NULL;
    var->ndda = 0;

    return err;
}

int PPFSEQ_setup(VAR_PPFSEQ * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;
    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("ndda", name)) {
            var->ndda = atoi(val);
        }
        else if (STR_EQUALS("shot", name)) {
            var->shot = atoi(val);
        }
        else {
            err = ERR_PPFSEQ_SETUP;
        }
    }

    var->pSeqNums = NULL;
    var->pModDates = NULL;
    var->pModTimes = NULL;
    var->pDDAs = NULL;

    return err;
}

int PPFSEQ_DATABLOCK(const VAR_PPFSEQ * var, DATA_BLOCK * db)
{
    int l;
    int err = 0;

    initDataBlock(db);

    int * pData = (int *) malloc(1 * sizeof(int));
    pData[0] = var->err;

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = (char *) pData;

    db->rank = 2;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    initDimBlock(&(db->dims[0]));
    initDimBlock(&(db->dims[1]));
    /*
      char* buffer = "HELLO";
      int l = strlen( buffer );
      db->dims[0].dim = malloc( l + 1 );
      strcpy( db->dims[0].dim, buffer );
      db->dims[0].dim_n = l + 1;//var->ndda;


      int sz = var->ndda * sizeof( int );
      int* pdata = (int*)(malloc( sz ) );
      printf_( g_dbgout, "NDDA=%d\n", var->ndda );
      for( i = 0; i < var->ndda; ++i )
      {
      pdata[i] = i;//var->pDDAs[i];
      }
    */


    int sz = sizeof(int);
    int * pi = malloc(sz);
    *pi = var->ndda;
    db->dims[0].dim = (char *) pi;
    db->dims[0].dim_n = sz;
    strcpy(db->dims[1].dim_label, "NDDA");

    //printf_( g_dbgout, ":%s\n", var->pDDAs );
    l = (var->ndda * 4);
    db->dims[1].dim = malloc(l + 1);
    db->dims[1].dim_n = l + 1;

    strcpy(db->dims[1].dim_label, "DDAs");
    strcpy(db->dims[1].dim, var->pDDAs);
    /*

      for( i = 0; i < db->rank; ++i )
      {
      const char* the_psz = ( 0 == i ) ? var->prgnam : var->ppfnam;

      initDimBlock( &(db->dims[i]) );

      int l = strlen( the_psz );

      char* pDimData = malloc( (l+1) );
      strcpy( pDimData, the_psz );
      pDimData[ l ]= 0;

      //db->dims[i].data_type = UDA_TYPE_STRING;
      db->dims[i].dim = pDimData;
      db->dims[i].dim_n = l+1;
      }

    */

    return err;
}

int PPFSEQ_execute(VAR_PPFSEQ * var)
{
    int err = 0;

    int * ndda = &(var->ndda);
    int * pSeqNums = (int *) (malloc(sizeof(int) * *ndda));
    int * pModDates = (int *) (malloc(sizeof(int) * *ndda));
    int * pModTimes = (int *) (malloc(sizeof(int) * *ndda));

    char * pDDAs = malloc((4 * *ndda) + 1);

    PPFSEQ(&(var->shot), ndda, pDDAs, pSeqNums, pModDates, pModTimes, &(var->err), 4 * *ndda);

    var->pSeqNums = pSeqNums;
    var->pModDates = pModDates;
    var->pModTimes = pModTimes;
    var->pDDAs = pDDAs;
    var->pDDAs[*ndda * 4] = '\0';

    return err;
}


int PPFSEQ_free(VAR_PPFSEQ * var)
{
    int err = 0;

    if (!var->pSeqNums) {
        free(var->pSeqNums);
        var->pSeqNums = NULL;
    }

    if (!var->pModDates) {
        free(var->pModDates);
        var->pModDates = NULL;
    }

    if (!var->pModTimes) {
        free(var->pModTimes);
        var->pModTimes = NULL;
    }

    if (!var->pDDAs) {
        free(var->pDDAs);
        var->pDDAs = NULL;
    }

    return err;
}


int PPFINF_init(VAR_PPFINF * var)
{
    int err = 0;
    var->shot = 0;
    var->seq = 0;
    var->comlen = 0;
    var->err = 0;

    memset(var->iwdat, 0, 12 * sizeof(int));
    var->pszComment = NULL;
    var->pszDDAs = NULL;

    return err;
}

int PPFINF_setup(VAR_PPFINF * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);


    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("comlen", name)) {
            //assert( comlen >=0 );
            v->comlen = atoi(val);
            v->pszComment = malloc((v->comlen * 4) + 1);
            memset(v->pszComment, 0, (v->comlen * 4));
        }
        else if (STR_EQUALS("ndda", name)) {
            v->ndda = atoi(val);
            v->pszDDAs = malloc((v->ndda * 4) + 1);
        }
        else {
            err = ERR_PPFINF_SETUP;
        }
    }
    return err;
}

int PPFINF_execute(VAR_PPFINF * v)
{
    int err = 0;
    int dummy = 0;

    PPFINF(&v->shot, &v->seq, &dummy, &v->comlen, v->iwdat, v->pszComment, &v->err, &v->ndda, v->pszDDAs, v->comlen * 4,
           v->ndda * 4);

    v->pszDDAs[v->ndda * 4] = '\0';
    v->pszComment[v->comlen] = '\0';

    return err;
}

int PPFINF_DATABLOCK(const VAR_PPFINF * v, DATA_BLOCK * db)
{
    int i = 0;
    int err = 0;

    initDataBlock(db);

    int * pData = (int *) malloc(1 * sizeof(int));
    pData[0] = v->err;

    db->data_type = UDA_TYPE_INT;
    db->data_n = 1;
    db->data = (char *) pData;

    db->rank = 5;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; ++i)
        initDimBlock(&(db->dims[i]));

    int sz = 2 * sizeof(int);
    int * pShotSeq = (int *) malloc(sz);
    pShotSeq[0] = v->shot;
    pShotSeq[1] = v->seq;

    db->dims[0].dim = (char *) pShotSeq;
    db->dims[0].dim_n = sz;
    strcpy(db->dims[0].dim_label, "shot:seq");

    sz = 12 * sizeof(int);
    db->dims[1].dim = malloc(sz);

    memcpy(db->dims[1].dim, v->iwdat, sz);
    db->dims[1].dim_n = sz;
    strcpy(db->dims[1].dim_label, "iwdat");

    char * p = v->pszComment;
    sz = strlen(p) + 1;
    db->dims[2].dim = malloc(sz);
    strcpy(db->dims[2].dim, p);
    db->dims[2].dim_n = sz;
    strcpy(db->dims[2].dim_label, "comment");

    //char* p = v->pszComment;
    sz = sizeof(int);
    db->dims[3].dim = malloc(sz);
    *(int *) db->dims[3].dim = v->ndda;
    db->dims[3].dim_n = sz;
    strcpy(db->dims[3].dim_label, "ndda");

    p = v->pszDDAs;
    sz = (4 * v->ndda) + 1;
    db->dims[4].dim = malloc(sz);
    strcpy(db->dims[4].dim, v->pszDDAs);
    db->dims[4].dim_n = sz;
    strcpy(db->dims[4].dim_label, "ddas");

    return err;
}


int DDAINF_init(VAR_DDAINF * v)
{
    int err = 0;

    v->shot = 0;
    v->err = 0;
    v->szDDA[0] = '\0';
    v->nwcom = 0;
    v->pszDDACom = NULL;
    v->ndt = 0;
    v->pszDTNAMS = NULL;
    v->lxtv = NULL;

    return err;
}

int DDAINF_setup(VAR_DDAINF * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int err = 0;
    int i = 0;
    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
        }
        else if (STR_EQUALS("dda", name)) {
            strcpy(v->szDDA, val);
        }
        else if (STR_EQUALS("nwcom", name)) {
            v->nwcom = atoi(val);

            if (v->nwcom > 0)
                v->pszDDACom = malloc((4 * v->nwcom) + 1);
        }
        else if (STR_EQUALS("ndt", name)) {
            v->ndt = atoi(val);
            if (v->ndt > 0)
                v->pszDTNAMS = malloc((4 * v->ndt) + 1);
        }
        else
            err = ERR_DDAINF_SETUP;
    }

    return err;
}

int DDAINF_execute(VAR_DDAINF * v)
{
    int err = 0;

    DDAINF(&v->shot, v->szDDA, &(v->nwcom), v->pszDDACom, &v->ndt, v->pszDTNAMS, v->lxtv, &(v->err), 5, 4 * v->nwcom,
           v->ndt * 4);

    return err;
}

int DDAINF_free(VAR_DDAINF * v)
{
    int err = 0;

    if (v) {
        if (v->pszDDACom) {
            free(v->pszDDACom);
            v->pszDDACom = NULL;
        }
        if (v->pszDTNAMS) {
            free(v->pszDTNAMS);
            v->pszDTNAMS = NULL;
        }
    }
    return err;
}

void DIM_string_set(DIMS * d, const char * psz)
{
    int l = strlen(psz);

    d->dim = malloc(l + 1);
    strcpy(d->dim, psz);

    d->dim_n = l + 1;
}

void DIM_label_set(DIMS * dim, const char * psz)
{
    strcpy(dim->dim_label, psz);
}

void DIM_int_set(DIMS * d, int i)
{
    const int sz = sizeof(int);
    d->dim = malloc(sz);
    *(int *) (d->dim) = i;
    d->dim_n = sz;
}

void DIM_ints_set(DIMS * d, const int * i, int count)
{
    const int sz = count * sizeof(int);

    if (sz > 0) {
        d->dim = malloc(sz);
        memcpy(d->dim, i, sz);
    }
    d->dim_n = sz;
}

void DIM_floats_set(DIMS * d, const float * i, int count)
{
    const int sz = count * sizeof(float);

    if (count > 0) {
        d->dim = malloc(sz);
        memcpy(d->dim, i, sz);
    }

    d->dim_n = sz;
}

int DDAINF_DATABLOCK(const VAR_DDAINF * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);


    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(1 * sizeof(int));
    *(int *) (db->data) = v->err;


    db->rank = 3;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; ++i)
        initDimBlock(&(db->dims[i]));

    const char * psz = v->pszDDACom;
    DIMS * dim = db->dims + 0;
    DIM_string_set(dim, psz);
    DIM_label_set(dim, "ddacom");

    dim = db->dims + 1;
    DIM_int_set(dim, v->ndt);
    DIM_label_set(dim, "ndt");

    dim = db->dims + 2;
    DIM_string_set(dim, v->pszDTNAMS);
    DIM_label_set(dim, "dtnams");

    return err;
}

int PPFDEL_init(VAR_PPFDEL * v)
{
    int err = 0;
    v->shot = 0;
    v->seq = 0;
    v->err = 0;
    return err;
}

int PPFDEL_setup(VAR_PPFDEL * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS(STR_SHOT, name)) {
            v->shot = atoi(val);
        }
        else if (STR_EQUALS(STR_SEQ, name)) {
            v->seq = atoi(val);
        }
        else
            err = ERR_PPFDEL_SETUP;

    }
    return err;
}

int PPFDEL_execute(VAR_PPFDEL * v)
{
    int err = 0;
    PPFDEL(&v->shot, &v->seq, &v->err);
    return err;
}

int PPFDEL_DATABLOCK(const VAR_PPFDEL * v, DATA_BLOCK * db)
{
    int err = 0;

    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    return err;
}

/*
int PPFWRI_init( VAR_PPFWRI* v )
{
  int err = 0;

  v->shot = 0;
  v->err = 0;
  memset( v->irdat, 0, 13 * sizeof(int) );
  memset( v->iwdat, 0, 13 * sizeof(int) );
  memset( v->ihdat, 0, 60 );
  v->szDDA[0] = '\0';
  v->szDTYPE[0] = '\0';

  v->data = NULL;
  v->x = NULL;
  v->t = NULL;

  return 0;
}

int PPFWRI_setup( VAR_PPFWRI* v, IDAM_PLUGIN_INTERFACE* ipi )
{
  int err = 0;

  const REQUEST_BLOCK* request_block = ipi->request_block;
  const NAMEVALUELIST* nvl = &(request_block->nameValueList);

  const PUTDATA_BLOCK_LIST* pdl = &(request_block->putDataBlockList);

  int i = 0;

  for( i = 0; i < pdl->blockCount; ++i )
    {
      const PUTDATA_BLOCK* pd = pdl->putDataBlock+i;
      const char* pszBlockName = pd->blockName;

      if( STR_EQUALS( pszBlockName, "ihdat") )
	memcpy( v->ihdat, pd->data, 60 );
      else if( STR_EQUALS( pszBlockName, "irdat") )
       memcpy( v->irdat, pd->data, 13 * sizeof( int ) );
      else if( STR_EQUALS( pszBlockName, "data" ) )
	v->data = (float*)(pd->data);

    }

  for( i = 0; i < nvl_count( nvl ); ++i )
    {
      const NAMEVALUE* nv = nvl_pair( nvl, i );
      const char* name = nv_name( nv );
      const char* val = nv_value( nv );

      if( STR_EQUALS( STR_FUNCTION_TAG, name ) )
	{
	}
      else if( STR_EQUALS( "shot", name ) )
	v->shot = atoi( val );
      else if ( STR_EQUALS( "dda", name ) )
	strcpy( v->szDDA, val );
      else if ( STR_EQUALS( "dtype", name ) )
	strcpy( v->szDTYPE, val );
      else
	err = ERR_PPFWRI_SETUP;
    }
  return err;
}
*/
int PDLPPF_init(VAR_PDLPPF * v)
{
    int err = 0;
    v->shot = 0;
    v->uid = 0;
    v->dup = 0;
    v->nseq = 0;
    v->lseq = NULL;
    v->ndda = 0;
    v->cdda = NULL;
    v->err = 0;
    return err;
}


int PDLPPF_setup(VAR_PDLPPF * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name))
            v->shot = atoi(val);
        else if (STR_EQUALS("uid", name))
            v->uid = atoi(val);
        else if (STR_EQUALS("dup", name))
            v->dup = atoi(val);
        else if (STR_EQUALS("nseq", name)) {
            v->nseq = atoi(val);
        }
            //else if( STR_EQUALS( "ndda", name ) )
            //	{
            //	  v->ndda = atoi( val );
            //	}
        else
            err = ERR_PDLPPF_SETUP;


    }
    if (!err) {
        v->uid = 1;
        v->dup = 1;
        //v->nseq = 1024;
        v->ndda = 3 * v->nseq;
        //v->cdda = malloc( (v->ndda * v->nseq * 4) + 1 );
        v->cdda = malloc((v->ndda * 3 * 4));
        v->lseq = (int *) (malloc(v->nseq * sizeof(int)));
        v->ldda = (int *) (malloc(v->nseq * sizeof(int)));
    }

    return err;
}

int PDLPPF_execute(VAR_PDLPPF * v)
{
    int err = 0;

    PDLPPF(&(v->shot), &(v->uid), &(v->dup), &(v->nseq), v->lseq, v->ldda, &v->ndda, v->cdda, &(v->err),
           0/*v->ndda * v->nseq * 4*/);

    return err;
}

int PDLPPF_free(VAR_PDLPPF * v)
{
    int err = 0;
    if (v) {
        if (v->lseq) {
            free(v->lseq);
            v->lseq = NULL;
        }

        if (v->ldda) {
            free(v->ldda);
            v->ldda = NULL;
        }

        if (v->cdda) {
            free(v->cdda);
            v->cdda = NULL;
        }
    }
    return err;
}

int PDLPPF_DATABLOCK(const VAR_PDLPPF * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;

    initDataBlock(db);

    db->data_n = 2;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 5;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; ++i)
        initDimBlock(&(db->dims[i]));

    DIMS * dim = db->dims + 0;
    DIM_int_set(dim, v->nseq);
    DIM_label_set(dim, "nseq");

    dim = db->dims + 1;
    DIM_ints_set(dim, v->lseq, v->nseq);
    DIM_label_set(dim, "lseq");

    dim = db->dims + 2;
    DIM_ints_set(dim, v->ldda, v->nseq);
    DIM_label_set(dim, "ldda");

    dim = db->dims + 3;
    DIM_int_set(dim, v->ndda);
    DIM_label_set(dim, "ndda");

    dim = db->dims + 4;
    DIM_string_set(dim, v->cdda);
    DIM_label_set(dim, "cdda");


    return err;
}

int PDLUSR_init(VAR_PDLUSR * v)
{
    int err = 0;
    v->shot = 0;
    v->nusers = 0;
    v->szDDA[0] = '\0';
    v->pszUsers = NULL;
    return err;
}

int PDLUSR_setup(VAR_PDLUSR * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
        }
        else if (STR_EQUALS("dda", name)) {
            strcpy(v->szDDA, val);
        }
        else if (STR_EQUALS("nusers", name)) {
            v->nusers = atoi(val);
        }
        else
            err = ERR_PDLUSR_SETUP;
    }

    if (!err)
        v->pszUsers = malloc((v->nusers * 8) + 1);

    return err;
}

int PDLUSR_execute(VAR_PDLUSR * v)
{
    int err = 0;

    PDLUSR(&(v->shot), v->szDDA, &(v->nusers), v->pszUsers, &(v->err), 4, v->nusers * 8);

    return err;
}

int PDLUSR_DATABLOCK(const VAR_PDLUSR * v, DATA_BLOCK * db)
{
    int i = 0;
    int err = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(1 * sizeof(int));//(char*)pData;
    *(int *) (db->data) = v->err;

    db->rank = 2;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; ++i)
        initDimBlock(&(db->dims[i]));

    DIMS * dim = db->dims + 0;
    DIM_int_set(dim, v->nusers);
    DIM_label_set(dim, "nusers");

    dim = db->dims + 1;
    DIM_string_set(dim, v->pszUsers);
    DIM_label_set(dim, "users");

    return err;
}


int PDLUSR_free(VAR_PDLUSR * v)
{
    int err = 0;
    if (v->pszUsers) {
        free(v->pszUsers);
        v->pszUsers = NULL;
    }
    return err;
}

int PDMSDT_init(VAR_PDMSDT * v)
{
    int err = 0;

    v->err = 0;
    v->idate = 0;
    v->szTime[0] = '\0';
    v->shot = 0;

    return err;
}

int PDMSDT_setup(VAR_PDMSDT * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("date", name))
            v->idate = atoi(val);
        else if (STR_EQUALS("time", name))
            strcpy(v->szTime, val);
        else
            err = ERR_PDMSDT_SETUP;
    }
    return err;
}

int PDMSDT_execute(VAR_PDMSDT * v)
{
    int err = 0;

    PDMSDT(&v->idate, v->szTime, &v->shot, &v->err, 8);

    return err;
}

int PDMSDT_DATABLOCK(const VAR_PDMSDT * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 1;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));
    for (i = 0; i < db->rank; i++)
        initDimBlock(&db->dims[i]);

    DIMS * dims = db->dims + 0;
    DIM_int_set(dims, v->shot);
    DIM_label_set(dims, "shot");

    return err;
}

int PDMSHT_init(VAR_PDMSHT * v)
{
    int err = 0;
    v->shot = 0;
    v->err = 0;
    return err;
}

int PDMSHT_execute(VAR_PDMSHT * v)
{
    int err = 0;

    PDMSHT(&v->shot);

    return err;
}

int PDMSHT_DATABLOCK(const VAR_PDMSHT * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 1;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));
    for (i = 0; i < db->rank; i++)
        initDimBlock(&db->dims[i]);

    DIMS * dims = db->dims + 0;
    DIM_int_set(dims, v->shot);
    DIM_label_set(dims, "shot");

    return err;
}

int PDSTAT_init(VAR_PDSTAT * v)
{
    int err = 0;
    v->err = 0;
    v->status = 0;
    memset(v->szLoginName, 0, 9);
    v->szLoginName[0] = '\0';

    v->szStatusString[0] = '\0';

    return err;
}

int PDSTAT_setup(VAR_PDSTAT * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        //const char* val = nv_value( nv );

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else
            err = ERR_PDSTAT_SETUP;
    }


    return err;
}

int PDSTAT_execute(VAR_PDSTAT * v)
{
    int err = 0;

    PDSTAT(v->szLoginName, &v->status, v->szStatusString, &v->err, 8, 60);

    return err;
}

int PDSTAT_DATABLOCK(const VAR_PDSTAT * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 3;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));
    for (i = 0; i < db->rank; i++)
        initDimBlock(&db->dims[i]);

    DIMS * dims = db->dims + 0;
    DIM_string_set(dims, v->szLoginName);
    DIM_label_set(dims, "LoginName");

    dims = db->dims + 1;
    DIM_int_set(dims, v->status);
    DIM_label_set(dims, "status");

    dims = db->dims + 2;
    DIM_string_set(dims, v->szStatusString);
    DIM_label_set(dims, "StatusString");

    return err;
}


int PDSTD_init(VAR_PDSTD * v)
{
    int err = 0;

    v->err = 0;
    v->shot = 0;
    v->time = 0;
    v->date = 0;
    return err;
}

int PDSTD_setup(VAR_PDSTD * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
        }
        else
            err = ERR_PDSTD_SETUP;
    }


    return err;
}

int PDSTD_execute(VAR_PDSTD * v)
{
    int err = 0;

    PDSTD(&v->shot, &v->time, &v->date, &v->err);


    return err;
}

int PDSTD_DATABLOCK(const VAR_PDSTD * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 2;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));
    for (i = 0; i < db->rank; i++)
        initDimBlock(&db->dims[i]);

    DIMS * dims = db->dims + 0;
    DIM_int_set(dims, v->date);
    DIM_label_set(dims, "date");

    dims = db->dims + 1;
    DIM_int_set(dims, v->time);
    DIM_label_set(dims, "time");

    return err;
}


int PDSRCH_init(VAR_PDSRCH * v)
{
    int err = 0;

    v->err = 0;
    v->shot = 0;
    v->seq = 0;
    v->szDDA[0] = '\0';
    v->pszDDAs = NULL;

    return err;
}

int PDSRCH_setup(VAR_PDSRCH * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
        }
        else if (STR_EQUALS("ndda", name)) {
            v->ndda = atoi(val);
            v->pszDDAs = malloc((4 * v->ndda) + 1);
            v->pszDDAs[0] = '\0';
        }
        else if (STR_EQUALS("dda", name)) {
            strcpy(v->szDDA, val);
        }
        else
            err = ERR_PDSRCH_SETUP;
    }

    return err;
}

int PDSRCH_execute(VAR_PDSRCH * v)
{
    int err = 0;


    PDSRCH(&v->shot, v->szDDA, &v->seq, &v->ndda, v->pszDDAs, &v->err, 4, v->ndda * 4);

    return err;
}

int PDSRCH_DATABLOCK(const VAR_PDSRCH * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 3;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; i++)
        initDimBlock(&db->dims[i]);

    DIMS * dims = db->dims + 0;
    DIM_int_set(dims, v->seq);
    DIM_label_set(dims, "seq");

    dims = db->dims + 1;
    DIM_int_set(dims, v->ndda);
    DIM_label_set(dims, "ndda");

    dims = db->dims + 2;
    DIM_string_set(dims, v->pszDDAs);
    DIM_label_set(dims, "ddalst");


    return err;
}

int PDSRCH_free(VAR_PDSRCH * v)
{
    int err = 0;
    if (v->pszDDAs) {
        free(v->pszDDAs);
        v->pszDDAs = NULL;
    }
    return err;
}


int PDTINF_init(VAR_PDTINF * v)
{
    int err = 0;

    v->err = 0;
    v->shot = 0;
    v->szDDA[0] = '\0';
    v->ndt = 0;

    v->pszDTNAMS = NULL;
    v->lxtv = NULL;
    v->pszDTComms = NULL;

    return err;
}

int PDTINF_setup(VAR_PDTINF * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
        }
        else if (STR_EQUALS("ndt", name)) {
            v->ndt = atoi(val);
            v->pszDTNAMS = malloc((4 * v->ndt) + 1);
            v->pszDTNAMS[0] = '\0';

            v->pszDTComms = malloc((v->ndt * 24) + 1);
            v->lxtv = (int *) malloc(v->ndt * 2 * sizeof(int));
        }
        else if (STR_EQUALS("dda", name)) {
            strcpy(v->szDDA, val);
        }
        else
            err = ERR_PDTINF_SETUP;
    }

    return err;
}

int PDTINF_execute(VAR_PDTINF * v)
{
    int err = 0;

    PDTINF(&v->shot, v->szDDA, &v->ndt, v->pszDTNAMS, v->lxtv, v->pszDTComms, &v->err, strlen(v->szDDA), 4 * v->ndt,
           24 * v->ndt
    );
    //	, strlen(),0,0);


    return err;
}

int PDTINF_free(VAR_PDTINF * v)
{
    int err = 0;

    if (v->pszDTNAMS) {
        free(v->pszDTNAMS);
        v->pszDTNAMS = NULL;
    }

    if (v->pszDTComms) {
        free(v->pszDTComms);
        v->pszDTComms = NULL;
    }

    if (v->lxtv) {
        free(v->lxtv);
        v->lxtv = NULL;
    }

    return err;
}


int PDTINF_DATABLOCK(const VAR_PDTINF * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 2;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; i++)
        initDimBlock(&db->dims[i]);

    DIMS * dims = db->dims + 0;
    DIM_int_set(dims, v->ndt);
    DIM_label_set(dims, "ndt");

    dims = db->dims + 1;
    DIM_string_set(dims, v->pszDTNAMS);
    DIM_label_set(dims, "dtnams");


    dims = db->dims + 2;
    DIM_string_set(dims, v->pszDTComms);
    DIM_label_set(dims, "stcomms");

    return err;
}


int PPFDAT_init(VAR_PPFDAT * v)
{
    int err = 0;

    v->err = 0;
    v->shot = 0;
    v->szDDA[0] = '\0';
    v->szDTYPE[0] = '\0';

    v->nx = v->nt = 0;

    v->data = NULL;
    v->x = NULL;
    v->t = NULL;

    return err;
}

int PPFDAT_setup(VAR_PPFDAT * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
        }
        else if (STR_EQUALS("seq", name)) {
            v->seq = atoi(val);
        }
        else if (STR_EQUALS("nx", name)) {
            v->nx = atoi(val);
            v->x = (float *) malloc(v->nx * sizeof(float));
        }
        else if (STR_EQUALS("nt", name)) {
            v->nt = atoi(val);
            v->t = (float *) malloc(v->nx * sizeof(float));
        }
        else if (STR_EQUALS("dda", name))
            strcpy(v->szDDA, val);
        else if (STR_EQUALS("dtype", name))
            strcpy(v->szDTYPE, val);
        else
            err = ERR_PPFDAT_SETUP;
    }

    if (!err)
        v->data = (float *) malloc(v->nt * v->nx * sizeof(float));

    return err;
}

int PPFDAT_execute(VAR_PPFDAT * v)
{
    int err = 0;

    PPFDAT(&v->shot, &v->seq, v->szDDA, v->szDTYPE, &v->nx, &v->nt, v->data, v->x, v->t, &v->err, 4, 4
    );


    return err;
}

int PPFDAT_free(VAR_PPFDAT * v)
{
    int err = 0;

    if (v->x) {
        free(v->x);
        v->x = NULL;
    }

    if (v->t) {
        free(v->t);
        v->t = NULL;
    }

    if (v->data) {
        free(v->data);
        v->data = NULL;
    }

    return err;
}


int PPFDAT_DATABLOCK(const VAR_PPFDAT * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 5;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; i++)
        initDimBlock(&db->dims[i]);

    DIMS * dims = db->dims + 0;
    DIM_int_set(dims, v->nx);
    DIM_label_set(dims, "nx");

    dims = db->dims + 1;
    DIM_floats_set(dims, v->x, v->nx);
    DIM_label_set(dims, "x");

    dims = db->dims + 2;
    DIM_int_set(dims, v->nx);
    DIM_label_set(dims, "nx");

    dims = db->dims + 3;
    DIM_floats_set(dims, v->t, v->nt);
    DIM_label_set(dims, "t");

    dims = db->dims + 4;
    DIM_floats_set(dims, v->data, v->nx * v->nt);
    DIM_label_set(dims, "data");

    return err;
}


int PPFDTI_init(VAR_PPFDTI * v)
{
    int err = 0;

    v->err = 0;
    v->shot = 0;
    v->szDDA[0] = '\0';
    v->szDTYPE[0] = '\0';

    v->nx = v->nt = 0;

    v->chFormat[1] = '\0';
    v->szUnits[0] = '\0';
    v->szComm[0] = '\0';
    v->systat = v->ustat = 0;

    return err;
}

int PPFDTI_setup(VAR_PPFDTI * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
        }
        else if (STR_EQUALS("seq", name)) {
            v->seq = atoi(val);
        }
        else if (STR_EQUALS("dda", name))
            strcpy(v->szDDA, val);
        else if (STR_EQUALS("dtype", name))
            strcpy(v->szDTYPE, val);
        else
            err = ERR_PPFDTI_SETUP;
    }

    return err;
}

int PPFDTI_execute(VAR_PPFDTI * v)
{
    int err = 0;

    PPFDTI(&v->shot, &v->seq, v->szDDA, v->szDTYPE, &v->nx, &v->nt, v->chFormat, v->szUnits, v->szComm, &v->systat,
           &v->ustat, &v->err, 4, 4, 1, 8, 24);

    return err;
}


int PPFDTI_DATABLOCK(const VAR_PPFDTI * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;


    db->rank = 7;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; i++)
        initDimBlock(&db->dims[i]);

    DIMS * dims = db->dims + 0;
    DIM_int_set(dims, v->nx);
    DIM_label_set(dims, "nx");

    dims = dims + 1;
    DIM_int_set(dims, v->nt);
    DIM_label_set(dims, "nt");

    dims = dims + 1;
    DIM_string_set(dims, v->chFormat);
    DIM_label_set(dims, "format");

    dims = dims + 1;
    DIM_string_set(dims, v->szUnits);
    DIM_label_set(dims, "units");

    dims = dims + 1;
    DIM_string_set(dims, v->szComm);
    DIM_label_set(dims, "comments");

    dims = dims + 1;
    DIM_int_set(dims, v->systat);
    DIM_label_set(dims, "systat");

    dims = dims + 1;
    DIM_int_set(dims, v->ustat);
    DIM_label_set(dims, "ustat");


    //dims = db->dims + 4;
    //DIM_floats_set( dims, v->data, v->nx*v->nt );
    //DIM_label_set( dims, "data" );

    return err;
}

int PPFDDA_init(VAR_PPFDDA * v)
{
    int err = 0;
    v->shot = 0;
    v->nseq = 0;
    v->pseqs = NULL;
    v->err = 0;
    v->szDDA[0] = '\0';
    return err;
}

int PPFDDA_setup(VAR_PPFDDA * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name))
            v->shot = atoi(val);
        else if (STR_EQUALS("dda", name))
            strcpy(v->szDDA, val);
        else if (STR_EQUALS("nseq", name)) {
            v->nseq = atoi(val);
            v->pseqs = (int *) malloc(v->nseq * sizeof(int));
        }
        else
            err = ERR_PPFDDA_SETUP;
    }

    return err;
}

int PPFDDA_execute(VAR_PPFDDA * v)
{
    int err = 0;

    PPFDDA(&v->shot, v->szDDA, v->pseqs, &v->nseq, &v->err, 4);

    return err;
}

int PPFDDA_free(VAR_PPFDDA * v)
{
    int err = 0;
    if (v->pseqs) {
        free(v->pseqs);
        v->pseqs = NULL;
    }

    return err;
}

int PPFDDA_DATABLOCK(const VAR_PPFDDA * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;


    db->rank = 2;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; i++)
        initDimBlock(&db->dims[i]);

    DIMS * dims = db->dims + 0;

    DIM_int_set(dims, v->nseq);
    DIM_label_set(dims, "nseq");

    dims = dims + 1;
    DIM_ints_set(dims, v->pseqs, v->nseq);
    DIM_label_set(dims, "seqs");

    return err;
}

int PPFGET_init(VAR_PPFGET * v)
{
    int err = 0;

    v->shot = 0;
    v->err = 0;
    memset(v->irdat, 0, 13 * sizeof(int));
    memset(v->iwdat, 0, 13 * sizeof(int));
    v->szDDA[0] = '\0';
    v->szDTYPE[0] = '\0';
    memset(v->ihdat, 0, 61);

    v->x = v->t = v->data = NULL;

    return err;
}

int PPFGET_setup(VAR_PPFGET * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    const REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    const unsigned int f_shot = 1;
    const unsigned int f_dda = f_shot << 1;
    const unsigned int f_dtype = f_dda << 1;
    const unsigned int f_irdat = f_dtype << 1;
    const unsigned int f_all = (f_shot | f_dda | f_dtype | f_irdat);

    unsigned int flag = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
            flag |= f_shot;
        }
        else if (STR_EQUALS("dda", name)) {
            strcpy(v->szDDA, val);
            flag |= f_dda;
        }
        else if (STR_EQUALS("dtype", name)) {
            strcpy(v->szDTYPE, val);
            flag |= f_dtype;
        }
    }

    //DATA_BLOCK* data_block    = ipi->data_block;
    const PUTDATA_BLOCK_LIST * pdl = &(request_block->putDataBlockList);

    for (i = 0; i < pdl->blockCount; ++i) {
        PUTDATA_BLOCK * pd = pdl->putDataBlock + i;

        if (STR_EQUALS("irdat", pd->blockName)) {
            memcpy(v->irdat, pd->data, 13 * sizeof(int));

            v->x = (float *) malloc(v->irdat[5] * sizeof(float));
            v->t = (float *) malloc(v->irdat[6] * sizeof(float));
            v->data = (float *) malloc(v->irdat[4] * sizeof(float));


            flag |= f_irdat;
        }
    }

    if (f_all != flag)
        err = ERR_PPFGET_SETUP;

    return err;
}

int PPFGET_execute(VAR_PPFGET * v)
{
    int err = 0;

    PPFGET(&v->shot, v->szDDA, v->szDTYPE, v->irdat, v->ihdat, v->iwdat, v->data, v->x, v->t, &v->err, 4, 4, 13);

    return err;
}

int PPFGET_free(VAR_PPFGET * v)
{
    int err = 0;

    if (v->data) {
        free(v->data);
        v->data = NULL;
    }

    if (v->x) {
        free(v->x);
        v->x = NULL;
    }

    if (v->t) {
        free(v->t);
        v->t = NULL;
    }

    return err;
}


int PPFGET_DATABLOCK(const VAR_PPFGET * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    if (!v->err) {
        db->rank = 5;
        db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

        for (i = 0; i < db->rank; i++)
            initDimBlock(&db->dims[i]);

        DIMS * dims = db->dims + 0;
        DIM_string_set(dims, v->ihdat);
        DIM_label_set(dims, "ihdat");

        dims = dims + 1;
        DIM_ints_set(dims, v->iwdat, 13);
        DIM_label_set(dims, "iwdat");

        dims = dims + 1;
        DIM_floats_set(dims, v->data, v->iwdat[4]);
        DIM_label_set(dims, "data");

        dims = dims + 1;
        DIM_floats_set(dims, v->x, v->iwdat[5]);
        DIM_label_set(dims, "x");

        dims = dims + 1;
        DIM_floats_set(dims, v->t, v->iwdat[6]);
        DIM_label_set(dims, "t");
    }

    return err;
}

int PPFGSF_init(VAR_PPFGSF * v)
{
    int err = 0;
    v->shot = 0;
    v->seq = 0;
    //v->pstats = NULL;
    v->err = 0;
    v->szDDA[0] = '\0';
    v->szDTYPE[0] = '\0';
    return err;
}


int PPFGSF_setup(VAR_PPFGSF * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    const REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    const unsigned int f_shot = 1;
    const unsigned int f_seq = f_shot << 1;
    const unsigned int f_dda = f_seq << 1;
    const unsigned int f_dtype = f_dda << 1;
    const unsigned int f_all = (f_shot | f_seq | f_dda | f_dtype);

    unsigned int flag = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
            flag |= f_shot;
        }
        else if (STR_EQUALS("seq", name)) {
            v->seq = atoi(val);
            flag |= f_seq;
        }
        else if (STR_EQUALS("dda", name)) {
            strcpy(v->szDDA, val);
            flag |= f_dda;
        }
        else if (STR_EQUALS("dtype", name)) {
            strcpy(v->szDTYPE, val);
            flag |= f_dtype;
        }
    }

    if (flag != f_all)
        err = ERR_PPFGSF_SETUP;

    return err;
}

int PPFGSF_execute(VAR_PPFGSF * v)
{
    int err = 0;

    PPFGSF(&v->shot, &v->seq, v->szDDA, v->szDTYPE, v->stats, &v->err, 4, 4);

    return err;
}


int PPFGSF_DATABLOCK(const VAR_PPFGSF * v, DATA_BLOCK * db)
{
    int err = 0;
    int i = 0;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 1;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; ++i)
        initDimBlock(&(db->dims[i]));

    DIM_ints_set(db->dims, v->stats, VAR_PPFGSF_MXSTATS);

    return err;
}

int PPFGTS_init(VAR_PPFGTS * v)
{
    int err = 0;
    v->shot = 0;
    v->szDDA[0] = '\0';
    v->szDTYPE[0] = '\0';

    memset(v->irdat, 0, 13 * sizeof(int));
    v->ihdat[60] = '\0';

    v->twant = 0.0;
    v->tgot = 0.0;
    v->data = NULL;
    v->x = NULL;
    return err;
}

int PPFGTS_setup(VAR_PPFGTS * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    const REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    const unsigned int f_shot = 1;
    const unsigned int f_dda = f_shot << 1;
    const unsigned int f_dtype = f_dda << 1;
    const unsigned int f_twant = f_dtype << 1;
    const unsigned int f_irdat = f_twant << 1;
    const unsigned int f_all = (f_shot | f_twant | f_dda | f_dtype | f_irdat);

    unsigned int flag = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
            flag |= f_shot;
        }
        else if (STR_EQUALS("dda", name)) {
            strcpy(v->szDDA, val);
            flag |= f_dda;
        }
        else if (STR_EQUALS("dtype", name)) {
            strcpy(v->szDTYPE, val);
            flag |= f_dtype;
        }
        else if (STR_EQUALS("twant", name)) {
            v->shot = atof(val);
            flag |= f_twant;
        }
    }


    const PUTDATA_BLOCK_LIST * pdl = &(request_block->putDataBlockList);

    for (i = 0; i < pdl->blockCount; ++i) {
        PUTDATA_BLOCK * pd = pdl->putDataBlock + i;

        if (STR_EQUALS("irdat", pd->blockName)) {
            memcpy(v->irdat, pd->data, 13 * sizeof(int));

            flag |= f_irdat;
        }
    }

    if (flag != f_all) {
        err = ERR_PPFGTS_SETUP;
    }
    else {
        v->x = (float *) malloc(v->irdat[5] * sizeof(float));
        v->data = (float *) malloc(v->irdat[4] * sizeof(float));
    }
    return err;
}

int PPFGTS_DATABLOCK(const VAR_PPFGTS * v, DATA_BLOCK * db)
{

    int err = 0;
    int i = 0;
    DIMS * dims = db->dims;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    if (!v->err) {

        db->rank = 2;
        db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

        for (i = 0; i < db->rank; ++i)
            initDimBlock(&(db->dims[i]));

        dims = db->dims + 0;
        DIM_string_set(dims, v->ihdat);
        DIM_label_set(dims, "ihdat");

        dims = db->dims + 1;
        DIM_ints_set(dims, v->iwdat, 13);
        DIM_label_set(dims, "iwdat");

        dims = db->dims + 2;
        DIM_floats_set(dims, v->data, v->iwdat[4]);
        DIM_label_set(dims, "data");

        dims = db->dims + 3;
        DIM_floats_set(dims, v->x, v->iwdat[5]);
        DIM_label_set(dims, "x");

        dims = db->dims + 4;
        DIM_floats_set(dims, &v->tgot, 1);
        DIM_label_set(dims, "tgot");
    }

    return err;
}


int PPFGTS_execute(VAR_PPFGTS * v)
{
    int err = 0;

    PPFGTS(&v->shot, v->szDDA, v->szDTYPE, &v->twant, v->irdat, v->ihdat, v->iwdat, v->data, v->x, &v->tgot, &v->err, 4,
           4, 61);

    return err;
}

int PPFGTS_free(VAR_PPFGTS * v)
{
    int err = 0;


    if (v->data) {
        free(v->data);
        v->data = NULL;
    }
    if (v->x) {
        free(v->x);
        v->x = NULL;
    }

    return err;
}

int PPFMOD_init(VAR_PPFMOD * v)
{
    int err = 0;

    v->shot = 0;
    v->time = 0;
    v->date = 0;
    v->seq = 0;
    v->moddate = 0;
    v->modtime = 0;
    v->modseq = 0;
    v->err = 0;

    return err;
}

int PPFMOD_setup(VAR_PPFMOD * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    const REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    const unsigned int f_shot = 1;
    const unsigned int f_all = (f_shot);

    unsigned int flag = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
            flag |= f_shot;
        }
    }

    if (flag != f_all)
        err = ERR_PPFMOD_SETUP;

    return err;
}

int PPFMOD_execute(VAR_PPFMOD * v)
{
    int err = 0;

    PPFMOD(&v->shot, &v->date, &v->time, &v->seq, &v->moddate, &v->modtime, &v->modseq, &v->err);

    return err;
}

int PPFMOD_DATABLOCK(const VAR_PPFMOD * v, DATA_BLOCK * db)
{

    int err = 0;

    int i = 0;
    DIMS * dims = db->dims;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 6;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; ++i)
        initDimBlock(&(db->dims[i]));

    dims = db->dims + 0;
    DIM_int_set(dims, v->date);
    DIM_label_set(dims, "idate");

    dims = db->dims + 1;
    DIM_int_set(dims, v->time);
    DIM_label_set(dims, "itime");

    dims = db->dims + 2;
    DIM_int_set(dims, v->seq);
    DIM_label_set(dims, "seq");

    dims = db->dims + 3;
    DIM_int_set(dims, v->moddate);
    DIM_label_set(dims, "moddate");

    dims = db->dims + 4;
    DIM_int_set(dims, v->modtime);
    DIM_label_set(dims, "modtime");

    dims = db->dims + 5;
    DIM_int_set(dims, v->modseq);
    DIM_label_set(dims, "modseq");


    return err;
}

int PPFONDISK_init(VAR_PPFONDISK * v)
{
    int err = 0;
    v->shot = 0;
    v->szDDA[0] = '\0';
    v->err = 0;
    v->ondisk = 0;
    return err;
}

int PPFONDISK_setup(VAR_PPFONDISK * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    const REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    const unsigned int f_shot = 1;
    const unsigned int f_dda = f_shot << 1;
    const unsigned int f_all = (f_shot | f_dda);

    unsigned int flag = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
            flag |= f_shot;
        }
        else if (STR_EQUALS("dda", name)) {
            strcpy(v->szDDA, val);
            flag |= f_dda;
        }
    }

    if (flag != f_all)
        err = ERR_PPFONDISK_SETUP;

    return err;
}

int PPFONDISK_execute(VAR_PPFONDISK * v)
{
    int err = 0;
    PPFONDISK(&v->shot, v->szDDA, &v->ondisk, &v->err, 4);
    return err;
}

int PPFONDISK_DATABLOCK(const VAR_PPFONDISK * v, DATA_BLOCK * db)
{
    int err = 0;

    int i = 0;
    DIMS * dims = db->dims;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 1;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; ++i)
        initDimBlock(&(db->dims[i]));

    dims = db->dims + 0;
    DIM_int_set(dims, v->ondisk);
    DIM_label_set(dims, "ondisk");


    return err;
}

int PPFOWNERINFO_init(VAR_PPFOWNERINFO * v)
{
    int err = 0;
    v->err = 0;
    v->seq = 0;
    v->shot = 0;
    v->ownerflag = 0;
    //v->szOwner[8] = v->szPrevOwner[8] = v->szAuthor[8] = '\0';

    memset(v->szOwner, 0, 9);
    memset(v->szPrevOwner, 0, 9);
    memset(v->szAuthor, 0, 9);
    return err;
}


int PPFOWNERINFO_setup(VAR_PPFOWNERINFO * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    const REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    const unsigned int f_shot = 1;
    const unsigned int f_seq = f_shot << 1;
    const unsigned int f_ownerflag = f_seq << 1;
    const unsigned int f_all = (f_shot | f_seq | f_ownerflag);

    unsigned int flag = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("shot", name)) {
            v->shot = atoi(val);
            flag |= f_shot;
        }
        else if (STR_EQUALS("seq", name)) {
            v->seq = atoi(val);
            flag |= f_seq;
        }
        else if (STR_EQUALS("flag", name)) {
            v->ownerflag = atoi(val);
            flag |= f_ownerflag;
        }
    }

    if (flag != f_all)
        err = ERR_PPFOWNERINFO_SETUP;

    return err;
}

int PPFOWNERINFO_execute(VAR_PPFOWNERINFO * v)
{
    int err = 0;

    PPFOWNERINFO(&v->shot, &v->seq, &v->ownerflag, v->szOwner, v->szPrevOwner, v->szAuthor, &v->err, 8, 8, 8);

    return err;

}

int PPFOWNERINFO_DATABLOCK(const VAR_PPFOWNERINFO * v, DATA_BLOCK * db)
{
    int err = 0;

    int i = 0;
    DIMS * dims = db->dims;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    db->rank = 3;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; ++i)
        initDimBlock(&(db->dims[i]));

    dims = db->dims + 0;
    DIM_string_set(dims, v->szOwner);
    DIM_label_set(dims, "owner");

    dims = db->dims + 1;
    DIM_string_set(dims, v->szPrevOwner);
    DIM_label_set(dims, "prevowner");

    dims = db->dims + 2;
    DIM_string_set(dims, v->szAuthor);
    DIM_label_set(dims, "author");

    return err;
}

int PPFSETDEVICE_init(VAR_PPFSETDEVICE * v)
{
    int err = 0;
    v->szDevice[0] = '\0';
    v->err = 0;
    return err;
}


int PPFSETDEVICE_setup(VAR_PPFSETDEVICE * v, IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    const REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    int i = 0;

    const unsigned int f_device = 1;
    const unsigned int f_all = (f_device);

    unsigned int flag = 0;

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS("device", name)) {
            strcpy(v->szDevice, val);
            flag |= f_device;
        }
    }

    if (flag != f_all)
        err = ERR_PPFSETDEVICE_SETUP;

    return err;
}

int PPFSETDEVICE_execute(VAR_PPFSETDEVICE * v)
{
    int err = 0;

    PPFSETDEVICE(v->szDevice, 8);

    return err;
}

int PPFSETDEVICE_DATABLOCK(const VAR_PPFSETDEVICE * v, DATA_BLOCK * db)
{
    int err = 0;

    //int i = 0;
    //DIMS* dims = db->dims;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;

    return err;
}

int PPFSIZ_init(VAR_PPFSIZ * v)
{
    int err = 0;

    v->szDDA[0] = '\0';
    v->szUser[0] = '0';

    v->shot = 0;
    v->seq = 0;
    v->nppf = 0;
    v->ndda = 0;
    v->ndtype = 0;
    v->err = 0;

    return err;
}

int PPFSIZ_setup(VAR_PPFSIZ * var, IDAM_PLUGIN_INTERFACE * ipi)
{
    REQUEST_BLOCK * request_block = ipi->request_block;
    const NAMEVALUELIST * nvl = &(request_block->nameValueList);

    unsigned int err = 0;
    int i = 0;

    int flag = 0;
    const int f_shot = 1;
    const int f_seq = f_shot << 1;
    const int f_dda = f_seq << 1;
    const int f_user = f_dda << 1;
    const int f_all = (f_shot | f_seq | f_dda | f_user);

    for (i = 0; i < nvl_count(nvl); ++i) {
        const NAMEVALUE * nv = nvl_pair(nvl, i);
        const char * name = nv_name(nv);
        const char * val = nv_value(nv);

        if (STR_EQUALS(STR_FUNCTION_TAG, name)) {
        }
        else if (STR_EQUALS(STR_SHOT, name)) {
            flag |= f_shot;
            var->shot = atoi(name);
        }
        else if (STR_EQUALS(STR_SEQ, name)) {
            flag |= f_seq;
            var->seq = atoi(val);
        }
        else if (STR_EQUALS("dda", name)) {
            flag |= f_dda;
            strcpy(var->szDDA, val);
        }
        else if (STR_EQUALS("user", name)) {
            flag |= f_user;
            strcpy(var->szUser, val);
        }
    }

    if (flag != f_all)
        err = ERR_PPFSIZ_SETUP;

    return err;
}

int PPFSIZ_execute(VAR_PPFSIZ * v)
{
    int err = 0;

    PPFSIZ(&v->shot, &v->seq, v->szDDA, v->szUser, &v->nppf, &v->ndda, &v->ndtype, &v->err, 4, 8);

    return err;
}

int PPFSIZ_DATABLOCK(const VAR_PPFSIZ * v, DATA_BLOCK * db)
{
    int err = 0;

    int i = 0;
    DIMS * dims = NULL;
    initDataBlock(db);

    db->data_n = 1;
    db->data_type = UDA_TYPE_INT;
    db->data = malloc(sizeof(int));
    *(int *) (db->data) = v->err;


    db->rank = 3;
    db->dims = (DIMS *) malloc(db->rank * sizeof(DIMS));

    for (i = 0; i < db->rank; ++i)
        initDimBlock(&(db->dims[i]));

    dims = db->dims;

    DIM_int_set(dims, v->nppf);
    DIM_label_set(dims, "nppf");

    dims = db->dims + 1;
    DIM_int_set(dims, v->ndda);
    DIM_label_set(dims, "ndda");

    dims = db->dims + 2;
    DIM_int_set(dims, v->ndtype);
    DIM_label_set(dims, "ndtype");

    return err;
}
