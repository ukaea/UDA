
/*-------------------------------------------------------------
 * Test the IDAM Server Plugin Interface
 *--------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "assert.h"


#pragma GCC diagnosic push
#pragma GCC diagnostic ignored "-Wall"

#include "idamclientserverpublic.h"
#include "idamclientpublic.h"
#include "idamgenstruct.h"

#pragma GCC diagnosic pop

void PDB_name_set( PUTDATA_BLOCK* pdb, char* pszName )
{
    printf( "%s\n", pszName );
    strcpy( pdb->blockName, pszName );
    pdb->blockNameLength = strlen( pszName );
}

void call_help()
{
    int handle = idamGetAPI("PPF2::help()", "");

    fprintf( stdout
             , "HELP Test #: %d  [%d] [%s]\n"
             , handle, getIdamErrorCode(handle), getIdamErrorMsg(handle));

    // Returned Data

    DATA_BLOCK *data_block = getIdamDataBlock(handle);

    printf("Data Type  %d\n", data_block->data_type);
    printf("Data Count %d\n", data_block->data_n);
    printf("Data Desc %s\n", data_block->data_desc);

    if( TYPE_STRING == data_block->data_type )
        printf("HelpData\n       %s\n", data_block->data);
    else
        printf("Data       Not a String!\n");
}

void call_ppfgo( int shot, int seq, int* err )
{
    char buffer[120];
    int handle = 0;
    sprintf( buffer, "PPF2::api(f=PPFGO, shot=%d, seq=%d)", shot, seq );

    handle = idamGetAPI( buffer, "");

    DATA_BLOCK* data_block = getIdamDataBlock(handle);

    if( TYPE_INT == data_block->data_type )
    {
        int* pData = (int*)(data_block->data);
        *err = pData[0];
        printf("PPFGO( ERR:%d )\n", *err );

    }
}

void call_ppfuid( char* pszUserId, char* pszRW, int* err )
{
    char buffer[120];

    int handle = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFUID, userid=%s, rw=%s)"
             , pszUserId, pszRW );

    handle = idamGetAPI( buffer, "");

    DATA_BLOCK* data_block = getIdamDataBlock(handle);

    if( data_block )
    {
        if( TYPE_INT == data_block->data_type )
        {
            int* pData = (int*)(data_block->data);
            printf("PPFUID( ERR:%d )\n", pData[0] );
            *err = pData[0];
        }
    }
}

void call_pdmseq( int pulse, int* seq, char* pszOwner, char* pszDDA, int* err)
{
    char buffer[120];

    int handle = 0;

    //char szOwner[9]="JETPPF";

    sprintf( buffer
             , "PPF2::api(f=PDMSEQ, pulse=%d, seq=%d, owner=%s, dda=%s)"
             , pulse, *seq, pszOwner, pszDDA );

    handle = idamGetAPI( buffer, "");

    DATA_BLOCK* data_block = getIdamDataBlock(handle);

    if( data_block )
    {
        if( TYPE_INT == data_block->data_type )
        {
            int* pData = (int*)(data_block->data);
            printf("PDMSEQ( ERR:%d,SEQ:%d )\n", pData[0], pData[1] );
            *err = pData[0];
            *seq = pData[1];
        }
    }

}

void call_ppfclo( int shot, int* seq, char* pszPrgName, int version, int* err )
{
    char buffer[120];

    int h = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFCLO, shot=%d, prgnam=%s, version=%d)"
             , shot, pszPrgName, version );

    h = idamGetAPI( buffer, "");
    fprintf( stdout
             , "PPFCLO Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* data_block = getIdamDataBlock( h );

    if( data_block )
    {
        if( TYPE_INT == data_block->data_type )
        {
            int* pData = (int*)(data_block->data);
            printf("PPFCLO( ERR:%d,SEQ:%d )\n", pData[0], pData[1] );
            *err = pData[0];
            *seq = pData[1];
        }
    }
}

void setup()
{
    setIdamProperty("verbose");
    setIdamProperty("debug");

    putIdamServerHost("localhost");
    putIdamServerPort(3090);		// Test Server

    char *host = getIdamServerHost();
    int port = getIdamServerPort();
    int version = getIdamClientVersion();

    int idamClientVersion = getIdamClientVersion();

    printf( "Client version = %d\n", idamClientVersion );
    printf( "IDAM server host = %s:%d\n", host,port );

    assert( idamClientVersion >= 7 && "IDAM client version too small" );
}

void tearDown(void)
{
    idamFreeAll();
}

void call_ppferr( char* rname, int err_code, char* msg, int* err )
{
    if( err_code )
    {
        char buffer[120];

        int handle = 0;

        sprintf( buffer
                 , "PPF2::api(f=PPFERR, rname=%s, error_code=%d)"
                 , rname, err_code );

        handle = idamGetAPI( buffer, "");

        DATA_BLOCK* data_block = getIdamDataBlock(handle);

        if( data_block )
        {
            //printf( "%s\n", data_block->data_label );
            //printf( "%s\n", data_block->data_units );
            if( TYPE_STRING == data_block->data_type )
            {
                char* pData = (char*)(data_block->data);
                printf("PPFERR( MSG:%s )\n", pData );
                strcpy( msg, pData );
                //*err = pData[0];
                //*seq = pData[1];
            }
        }
    }
}

void call_ppfsqi( int shot, int pqi, int* err )
{
    char buffer[120];

    int handle = 0;

    sprintf( buffer, "PPF2::api(f=PPFSQI, shot=%d, pqi=%d)", shot, pqi );

    handle = idamGetAPI( buffer, "");

    DATA_BLOCK* db = getIdamDataBlock(handle);

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            int* pData = (int*)(db->data);
            *err = pData[0];

            printf( "PPFSQI( ERR:%d)\n", *err );
        }
    }
}

void call_ppfgqi( int shot, int* pqi, int* err )
{
    char buffer[120];

    int handle = 0;

    sprintf( buffer, "PPF2::api(f=PPFGQI, shot=%d)", shot );

    handle = idamGetAPI( buffer, "");

    DATA_BLOCK* db = getIdamDataBlock(handle);
    *err = -100;
    if( db )
    {
        *pqi = -222;
        if( TYPE_INT == db->data_type )
        {
            int* pData = (int*)(db->data);
            *err = pData[0];
            *pqi = pData[1];

            printf( "PPFGQI( ERR:%d, pqi:%d)\n", *err, *pqi );
        }
    }
}

void call_ppfgid( char* rw, char* pszUserId, int* err )
{
    char buffer[120];

    int handle = 0;
    *err = 0;
    sprintf( buffer, "PPF2::api(f=PPFGID, rw=%s)", rw );

    handle = idamGetAPI( buffer, "");

    DATA_BLOCK* db = getIdamDataBlock(handle);
    if( db )
    {
        if( TYPE_STRING == db->data_type )
        {
            //char* pData = db->data;
            //*err = pData[0];

            strcpy( pszUserId, db->data );

            printf( "PPFGID( USR:%s)\n", pszUserId );
        }
    }
}

void call_ppfpok( int* err )
{
    char buffer[120];

    int handle = 0;

    sprintf( buffer, "PPF2::api(f=PPFPOK)" );

    handle = idamGetAPI( buffer, "");

    DATA_BLOCK* db = getIdamDataBlock(handle);
    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            int* pData = (int*)(db->data);

            *err = pData[0];
            printf( "PPFPOK( ERR:%d)\n", *err );
        }
    }
}

void call_pdainf( char* pszPrgName, int* version, char* pszPPFNam, int* err )
{
    char buffer[120];

    int h = 0;

    sprintf( buffer, "PPF2::api(f=PDAINF)" );

    h = idamGetAPI( buffer, "");
    printf( "PDAINF");

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            int* pData = (int*)(db->data);
            *err = pData[0];
            printf( "( ERR:%d)\n", *err );

            //char* pDimData = db->dims[0].dim;


            //printf( "STR %s:%s\n", pDimData, db->dims[1].dim );
        }
        else
            printf( ":NODATA\n");
    }
    else
        printf( ":XXXNODATA\n");
}

void call_ppfseq( int* shot, int* ndda, char* ddas, int* seqnum, int* err )
{
    int i = 0;
    int h = 0;
    PUTDATA_BLOCK pd;
    PUTDATA_BLOCK_LIST putDataBlockList;
    char buffer[120];

    //int* pModDates = (int*)(malloc( *ndda * sizeof(int) ));
    //int* pModTimes = (int*)(malloc( *ndda * sizeof(int) ));

    //PPFSEQ ( shot, ndda, ddas, seqnum, pModDates, pModTimes,
    //	   err, strlen( ddas ));

    initIdamPutDataBlock(&pd);


    printf ("CALL_PPFSEQ START\n");
    initIdamPutDataBlockList( &putDataBlockList );

    pd.data_type = TYPE_INT;
    pd.count     = 10;
    pd.rank      = 1;

    pd.blockNameLength = 5;
    pd.blockName = "TEST";
    pd.blockNameLength = strlen( pd.blockName );

    //int *idata = (int *)malloc( pd.count*sizeof(int) );
    int idata[10];

    for(i=0; i<pd.count; i++)
        idata[i] = i;

    pd.data = (char *)idata;

    addIdamPutDataBlockList(&pd, &putDataBlockList);

    printf( "PD BlockName=%s\n", pd.blockName );
    printf( "COUNT=%d\n", putDataBlockList.blockCount );

    sprintf( buffer, "PPF2::api(f=PPFSEQ, shot=%d, ndda=%d)", *shot, *ndda );
    printf( "%s\n", buffer );

    //h = idamPutListAPI("PPF2::api(f=PPFSEQ, a=21)", &putDataBlockList );
    //h = idamPutListAPI( buffer, &putDataBlockList );
    h = idamGetAPI( buffer, "" );//, &putDataBlockList );

    fprintf( stdout
             , "PPFSEQ Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            int* pData = (int*)(db->data);
            *err = pData[0];

            //int rank = db->rank;
            const int* pn = (int*)(db->dims[0].dim);
            const char* pszDDAs = db->dims[1].dim;

            printf( "(ERR:%d)\n", *err );;
            printf( "(RANK:%d)\n", db->rank );
            printf( "%s:%d:%s\n", db->dims[1].dim_label, *pn, pszDDAs );
        }
        else
            printf( ":NODATA\n");
    }
    else
        printf( ":XXXNODATA\n");



    /*
    for(i=0;i<putDataBlockList.blockCount;i++)
      if(putDataBlockList.putDataBlock[i].data != NULL)
        free((void *)putDataBlockList.putDataBlock[i].data);
    */
    freeIdamClientPutDataBlockList(&putDataBlockList);

    printf ("CALL_PPFSEQ END\n");

}


void call_ppfinf( int* shot, int* seq
                  , int comlen
                  , int* iwdat, char* pszComment
                  , int* err, int* ndda, char* pszDDAs )
{
    char buffer[120];
    int h;
    sprintf( buffer, "PPF2::api(f=PPFINF, comlen=%d, ndda=%d)", comlen, *ndda );
    printf( "%s\n", buffer );


    //memset( pszDDAs, 0, (*ndda) * 4 );
    pszDDAs[0]='\0';

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFINF Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        *err = *(int*)(db->data);

        if( ! (*err) )
        {
            int*pShotSeq = (int*)(db->dims[0].dim);

            *shot = pShotSeq[0];
            *seq = pShotSeq[1];
            memcpy( iwdat, db->dims[1].dim, db->dims[1].dim_n );

            strcpy( pszComment, db->dims[2].dim );
            pszComment[ db->dims[2].dim_n ] = '\0';

            *ndda = *(int*)(db->dims[3].dim);

            strcpy( pszDDAs, db->dims[4].dim );

            printf( "(ERR:%d)\n", *err );
            printf( "(SHOT:%d)\n", *shot );
            printf( "(SEQ:%d)\n", *seq );
            printf( "(l:%d)\n", db->dims[2].dim_n );
            printf( "(COMMENT:%s)\n", pszComment );
            printf( "(NDDA:%d)\n", *ndda );
            printf( "(DDAs:%s)\n", pszDDAs );
        }
    }
}

void call_ddainf( int shot
                  , char* pszDDA
                  , int* nwcom, char* pszDDACom
                  , int* ndt, char* pszDTNAMS, int* lxtv, int* perr )
{
    int h = 0;
    char buffer[120];

    sprintf( buffer, "PPF2::api(f=DDAINF, shot=%d, dda=%s, nwcom=%d, ndt=%d)"
             , shot, pszDDA, *nwcom, *ndt );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "DDAINF Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            *perr = *(int*)db->data;
            printf( "ERR:%d\n", *perr );

            DIMS* dim = db->dims + 0;
            strcpy( pszDDACom, dim->dim );
            *nwcom = dim->dim_n-1;

            dim = db->dims + 1;
            *ndt = *(int*)(dim->dim);

            dim = db->dims + 2;
            strcpy( pszDTNAMS, dim->dim );

            printf( "ddacom:%s:%d\n", pszDDACom, *nwcom );
            printf( "ndt:%d\n", *ndt );
            printf( "dtnams:%s:\n", pszDTNAMS );
        }
    }
}

void call_ppfdel( int shot, int seq, int* perr )
{
    int h = 0;
    char buffer[120];

    sprintf( buffer, "PPF2::api(f=PPFDEL, shot=%d, seq=%d)"
             , shot, seq );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFDEL Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db && TYPE_INT == db->data_type )
    {
        *perr = *(int*)db->data;
        printf( "ERR:%d\n", *perr );
    }
}

void call_ppfwri( int shot, char* pszDDA, char* pszDTYPE
                  , const int* irdat, const char* ihdat, int* iwdat
                  , float* data , float* x, float* t
                  , int* perr )
{
    int h = 0;
    PUTDATA_BLOCK pd;
    PUTDATA_BLOCK_LIST putDataBlockList;
    char buffer[120];

    initIdamPutDataBlock(&pd);
    initIdamPutDataBlockList( &putDataBlockList );

    pd.data_type = TYPE_INT;

    pd.count     = 13;
    pd.rank      = 1;
    pd.data = (char*)(irdat);
    pd.blockName = "irdat";
    pd.blockNameLength = strlen( pd.blockName );
    addIdamPutDataBlockList( &pd, &putDataBlockList );


    pd.data_type = TYPE_CHAR;
    pd.count     = 60;
    pd.data = (char*)(ihdat);
    pd.rank      = 1;
    pd.blockName = "ihdat";
    pd.blockNameLength = strlen( pd.blockName );
    addIdamPutDataBlockList( &pd, &putDataBlockList );

    pd.data_type = TYPE_INT;
    pd.count     = irdat[5] * irdat[6];
    pd.rank      = 1;
    pd.data = (char*)(data);
    pd.blockName = "data";
    pd.blockNameLength = strlen( pd.blockName );
    addIdamPutDataBlockList( &pd, &putDataBlockList );

    pd.count     = irdat[6];
    pd.rank      = 1;
    pd.data = (char*)(t);
    pd.blockName = "t";
    pd.blockNameLength = strlen( pd.blockName );
    addIdamPutDataBlockList( &pd, &putDataBlockList );

    pd.count     = irdat[5];
    pd.rank      = 1;
    pd.data = (char*)(x);
    pd.blockName = "x";
    pd.blockNameLength = strlen( pd.blockName );
    addIdamPutDataBlockList( &pd, &putDataBlockList );


    h = idamPutListAPI( buffer, &putDataBlockList );

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db && TYPE_INT == db->data_type )
    {
        int* pData = (int*)(db->data);
        *perr = pData[0];
    }
}

void call_pdlppf( int shot
                  , int uid, int dup
                  , int* nseq, int* seqs
                  , int* lddas, int* ndda, char* cdda
                  , int* err )
{
    int h = 0;
    char buffer[120];

    sprintf( buffer
             , "PPF2::api(f=PDLPPF, shot=%d, uid=%d, dup=%d, nseq=%d)"
             , shot, uid, dup, *nseq );
    printf ("%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PDLPPF Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            int* pData = (int*)(db->data);
            *err = pData[0];

            DIMS* dims = db->dims + 0;
            *nseq = *(int*)(dims->dim);

            dims = db->dims + 1;
            memcpy( seqs, dims->dim, dims->dim_n );

            dims = db->dims + 2;
            memcpy( lddas, dims->dim, dims->dim_n );

            dims = db->dims + 3;
            memcpy( ndda, dims->dim, dims->dim_n );

            dims = db->dims + 4;
            memcpy( cdda, dims->dim, dims->dim_n );

            int i = 0;
            printf( "NSEQ:%d\n", *nseq);
            printf( "SEQS:" );

            for( i = 0; i < *nseq; ++i )
                printf( "%d:", seqs[i] );
            printf( "\n" );

            printf( "NDDA:%d\n", *ndda);
            printf( "CDDA:%s\n", cdda);
        }
    }

}

void call_pdlusr( int* pulse, char* pszDDA, int* nusers, char* pszUsers, int* err)
{
    char buffer[120];
    int h;

    sprintf( buffer, "PPF2::api(f=PDLUSR, shot=%d, dda=%s, nusers=%d)"
             , *pulse, pszDDA, *nusers );
    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PDLUSR Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            *err = *(int*)(db->data);

            DIMS* dims = db->dims + 0;
            *nusers = *(int*)(dims->dim);

            dims = db->dims + 1;
            memcpy( pszUsers, dims->dim, dims->dim_n );


            printf( "ERR:%d\n", *err );
            printf( "NUSERS:%d\n", *nusers );
            printf( "USERS:%s:\n", pszUsers );
        }
    }
}

void call_pdmsdt( int idate, const char* pszTime, int* shot, int* err )
{
    char buffer[120];
    int h;

    sprintf( buffer, "PPF2::api(f=PDMSDT, date=%d, time=%s)", idate, pszTime );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PDMSDT Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            *err = *(int*)(db->data);

            DIMS* dims = db->dims + 0;
            memcpy( shot, dims->dim, dims->dim_n );

            printf( "ERR:%d\n", *err );
            printf( "SHOT:%d\n", *shot );
        }
    }
}

void call_pdmsht( int* shot )
{
    char buffer[120];
    int h;

    sprintf( buffer, "PPF2::api(f=PDMSHT)" );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PDMSHT Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            int err = *(int*)(db->data);

            DIMS* dims = db->dims + 0;
            memcpy( shot, dims->dim, dims->dim_n );

            printf( "ERR:%d\n", err );
            printf( "SHOT:%d\n", *shot );
        }
    }

}
void call_pdstat( char* pszLoginName, int* status, char* pszStatus, int* err )
{
    char buffer[120];
    int h;

    sprintf( buffer, "PPF2::api(f=PDSTAT)" );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PDSTAT Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            int err = *(int*)(db->data);

            DIMS* dims = db->dims + 0;
            memcpy( pszLoginName, dims->dim, dims->dim_n );

            dims = db->dims + 1;
            memcpy( status, dims->dim, dims->dim_n );

            dims = db->dims + 2;
            memcpy( pszStatus, dims->dim, dims->dim_n );

            printf( "ERR:%d\n", err );
            printf( "login:%s:\n", pszLoginName );
            printf( "status:%d:%s:\n", *status, pszLoginName );
        }
    }
}

void call_pdsrch( const int shot
                  , const char* pszDDA
                  , int* seq
                  , int* ndda, char* pszDDAList
                  , int* err )
{
    char buffer[120];
    int h;

    sprintf( buffer, "PPF2::api(f=PDSRCH, shot=%d, dda=%s, ndda=%d)"
             , shot, pszDDA, *ndda );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PDSRCH Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            *err = *(int*)(db->data);

            DIMS* dims = db->dims + 0;

            memcpy( seq, dims->dim, dims->dim_n );

            dims = db->dims + 1;
            memcpy( ndda, dims->dim, dims->dim_n );

            dims = db->dims + 2;
            memcpy( pszDDAList, dims->dim, dims->dim_n );

        }
    }
}

void call_pdstd(int shot, int* itime, int* idate, int* err )
{
    char buffer[120];
    int h;

    sprintf( buffer, "PPF2::api(f=PDSTD, shot=%d)", shot );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PDSDT Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            *err = *(int*)(db->data);

            DIMS* dims = db->dims + 0;
            memcpy( idate, dims->dim, dims->dim_n );

            dims = db->dims + 1;
            memcpy( itime, dims->dim, dims->dim_n );

            printf( "date:%d\n", *idate );
            printf( "time:%d\n", *itime );
        }
    }
}


void call_pdtinf(int shot
                 , const char* pszDDA
                 , int* ndt
                 , char* dtnams, int* lxtv, char* pszDTComms, int* err )
{
    char buffer[120];
    int h;

    sprintf( buffer
             , "PPF2::api(f=PDTINF, shot=%d, dda=%s, ndt=%d)"
             , shot, pszDDA, *ndt );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PDTINF Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            *err = *(int*)(db->data);


            printf( "ERR:%d\n", *err );
        }
    }

}

void call_ppfdat( int shot, int seq
                  , const char* pszDDA, const char* pszDTYPE
                  , int* nx, float* x
                  , int* nt, float* t
                  , float* data
                  , int* err )
{
    char buffer[120];
    int h;

    *err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFDAT, shot=%d, seq=%d, dda=%s, dtype=%s, nx=%d, nt=%d)"
             , shot, seq, pszDDA, pszDTYPE, *nx, *nt );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFDAT Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            DIMS* dims = NULL;

            *err = *(int*)(db->data);

            dims = db->dims + 0;
            memcpy( nx, dims->dim, dims->dim_n );

            dims = db->dims + 1;
            memcpy( x, dims->dim, dims->dim_n );

            dims = db->dims + 2;
            memcpy( nt, dims->dim, dims->dim_n );

            dims = db->dims + 3;
            memcpy( t, dims->dim, dims->dim_n );

            dims = db->dims + 4;
            memcpy( data, dims->dim, dims->dim_n );

            printf( "ERR:%d\n", *err );
        }
    }
}

void call_ppfdti( int shot, int seq
                  , char* pszDDA, char* pszDTYPE
                  , int* nx, int* nt
                  , char* pszFormat, char* pszUnits
                  , char* pszComm
                  , int* systat, int* ustat
                  , int* err )
{
    char buffer[120];
    int h;

    *err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFDTI, shot=%d, seq=%d, dda=%s, dtype=%s)"
             , shot, seq, pszDDA, pszDTYPE );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFDTI Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            DIMS* dims = NULL;
            *err = *(int*)(db->data);

            dims = db->dims;
            memcpy( nx, dims->dim, dims->dim_n );


            dims = dims + 1;
            memcpy( nt, dims->dim, dims->dim_n );

            dims = dims + 1;
            memcpy( pszFormat, dims->dim, dims->dim_n );

            dims = dims + 1;
            memcpy( pszUnits, dims->dim, dims->dim_n );

            dims = dims + 1;
            memcpy( pszComm, dims->dim, dims->dim_n );

            dims = dims + 1;
            memcpy( systat, dims->dim, dims->dim_n );

            dims = dims + 1;
            memcpy( ustat, dims->dim, dims->dim_n );

            printf( "ERR:%d\n", *err );

        }
    }
}

void call_ppfdda(int shot, char* pszDDA, int * nseq, int* pseq, int* err)
{   char buffer[120];
    int h;

    *err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFDDA, shot=%d, nseq=%d, dda=%s)"
             , shot, *nseq, pszDDA );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFDDA Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            DIMS* dims = NULL;
            *err = *(int*)(db->data);

            dims = db->dims + 0;
            memcpy( nseq, dims->dim, dims->dim_n );

            dims = db->dims + 1;
            memcpy( pseq, dims->dim, dims->dim_n );

            printf( "ERR:%d\n", *err );
        }
    }

}

void call_ppfget( int shot
                  , char* pszDDA, char* pszDTYPE
                  , int* irdat, char* pszIHDAT, int* iwdat
                  , float* data, float* x, float* t
                  , int* err)
{   char buffer[120];
    int h;

    *err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFGET, shot=%d, dda=%s, dtype=%s)"
             , shot, pszDDA, pszDTYPE );

    printf( "%s\n", buffer );


    //add the irdat array PUTDATA_BLOCK pd;
    PUTDATA_BLOCK pdb;
    PUTDATA_BLOCK_LIST pdbl;


    initIdamPutDataBlockList( &pdbl );

    initIdamPutDataBlock(&pdb);

    pdb.data_type = TYPE_INT;
    pdb.count     = 13;
    pdb.rank      = 1;

    pdb.data = (char *)irdat;
    pdb.blockName = "irdat";
    pdb.blockNameLength = strlen( pdb.blockName );

    addIdamPutDataBlockList( &pdb, &pdbl);

    h = idamPutListAPI( buffer, &pdbl );

    fprintf( stdout
             , "PPFGET Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            DIMS* dims = NULL;
            *err = *(int*)(db->data);

            printf( "ERR:%d\n", *err );

            if( !*err )
            {
                dims = db->dims + 0;
                memcpy( pszIHDAT, dims->dim, dims->dim_n );

                dims = db->dims + 1;
                memcpy( iwdat, dims->dim, dims->dim_n );

                dims = db->dims + 1;
                memcpy( data, dims->dim, dims->dim_n );

                dims = db->dims + 1;
                memcpy( x, dims->dim, dims->dim_n );

                dims = db->dims + 1;
                memcpy( t, dims->dim, dims->dim_n );
            }
        }
    }
}

void call_ppfgsf(int shot, int seq, char* pszDDA, char* pszDTYPE
                 , int* stats, int* err)
{

    char buffer[120];
    int h;

    *err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFGSF, shot=%d, seq=%d, dda=%s, dtype=%s)"
             , shot, seq, pszDDA, pszDTYPE );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFGSF Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            DIMS* dims = NULL;
            *err = *(int*)(db->data);

            printf( "ERR:%d\n", *err );

            dims  = db->dims + 0;
            memcpy( stats, dims->dim, dims->dim_n );
        }
    }
}

void cal_ppfgts( int shot
                 , char* pszDDA, char* pszDTYPE
                 , float twant, int* irdat, char* ihdat
                 , int* iwdat, float* data, float* x, float* tgot, int* err )


{
    char buffer[120];
    int h;

    *err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFGTS, shot=%d, dda=%s, dtype=%s, twant=%f)"
             , shot, pszDDA, pszDTYPE, twant );

    printf( "%s\n", buffer );


    //add the irdat array PUTDATA_BLOCK pd;
    PUTDATA_BLOCK pdb;
    PUTDATA_BLOCK_LIST pdbl;

    initIdamPutDataBlockList( &pdbl );

    initIdamPutDataBlock(&pdb);

    pdb.data_type = TYPE_INT;
    pdb.count     = 13;
    pdb.rank      = 1;

    pdb.data = (char *)irdat;
    pdb.blockName = "irdat";
    pdb.blockNameLength = strlen( pdb.blockName );

    addIdamPutDataBlockList( &pdb, &pdbl);

    h = idamPutListAPI( buffer, &pdbl );

    fprintf( stdout
             , "PPFGTS Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            //DIMS* dims = NULL;
            *err = *(int*)(db->data);

            printf( "ERR:%d\n", *err );

            if( !*err )
            {
            }
        }
    }
}

void call_ppfmod( int shot
                  , int* date, int* time
                  , int* seq
                  , int* moddate, int* modtime, int* modseq
                  , int* err )

{

    char buffer[120];
    int h;

    *err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFMOD, shot=%d)", shot );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFMOD Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            DIMS* dims = db->dims;
            *err = *(int*)(db->data);

            printf( "ERR:%d\n", *err );

            if( !(*err) )
            {
                memcpy( date, dims->dim, sizeof(int) );
                memcpy( time, (dims+1)->dim, sizeof(int) );
                memcpy( seq, (dims+2)->dim, sizeof(int) );
                memcpy( moddate, (dims+3)->dim, sizeof(int) );
                memcpy( modtime, (dims+4)->dim, sizeof(int) );
                memcpy( modseq, (dims+5)->dim, sizeof(int) );
            }
        }
    }
}


void call_ppfondisk( int shot, char* pszDDA, int* isondisk, int* err )
{
    char buffer[120];
    int h;

    *err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFONDISK, shot=%d, dda=%s)", shot, pszDDA );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFONDISK Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            DIMS* dims = db->dims;
            *err = *(int*)(db->data);

            printf( "ERR:%d\n", *err );

            if( !(*err) )
            {
                memcpy( isondisk, dims->dim, dims->dim_n );
                printf( "ONDISK:%d\n", *isondisk );
            }
        }
    }
}


void call_ppfownerinfo( int shot, int seq
                        , int ownerflag
                        , char* pszOwner, char* pszPrevOwner, char* pszAuthor
                        , int* err )
{
    char buffer[120];
    int h;

    *err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFOWNERINFO, shot=%d, seq=%d, flag=%d)"
             , shot, seq, ownerflag );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFOWNERINFO Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            DIMS* dims = db->dims;
            *err = *(int*)(db->data);

            printf( "ERR:%d\n", *err );

            if( !(*err) )
            {
                memcpy( pszOwner, (dims+0)->dim, (dims+0)->dim_n );
                memcpy( pszPrevOwner, (dims+1)->dim, (dims+1)->dim_n );
                memcpy( pszAuthor, (dims+2)->dim, (dims+2)->dim_n );

                printf( "owner:%s\n", pszOwner );
                printf( "prev owner:%s\n", pszPrevOwner );
                printf( "author:%s\n", pszAuthor );
            }
        }
    }
}

void call_ppfsetdevice( const char* pszDevice )
{
    char buffer[120];
    int h;

    //*err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFSETDEVICE, device=%s)"
             , pszDevice );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFSETDEVICE Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            //DIMS* dims = db->dims;
            //*err = *(int*)(db->data);

            //printf( "ERR:%d\n", *err );
        }
    }
}

void call_ppfsiz(int shot, int seq
                 , const char* pszDDA
                 , const char* pszUser
                 , int* nppf, int* ndda, int* ndtype, int* err )
{
    char buffer[120];
    int h;

    *err = 0;

    sprintf( buffer
             , "PPF2::api(f=PPFSIZ, shot=%d, seq=%d, dda=%s, user=%s)"
             , shot, seq, pszDDA, pszUser );

    printf( "%s\n", buffer );

    h = idamGetAPI( buffer, "" );

    fprintf( stdout
             , "PPFSIZ Test #: %d  [%d] [%s]\n"
             , h, getIdamErrorCode(h), getIdamErrorMsg(h));

    DATA_BLOCK* db = getIdamDataBlock( h );

    if( db )
    {
        if( TYPE_INT == db->data_type )
        {
            DIMS* dims = db->dims;
            *err = *(int*)(db->data);

            if( !(*err) )
            {
                memcpy( nppf, dims->dim, dims->dim_n );
                dims = db->dims+1;
                memcpy( ndda, dims->dim, dims->dim_n );
                dims = db->dims+2;
                memcpy( ndtype, dims->dim, dims->dim_n );

                printf( "ERR:%d\n", *err );
                printf( "nppf:%d\n", *nppf );
                printf( "ndda:%d\n", *ndda );
                printf( "ndtype:%d\n", *ndtype );
            }
        }
    }
}

void call_readNothing()
{
    int r = idamGetAPI("", "NOTHING::12345");
    printf( "R=%d\n", r);
}

int main(int nargs, char *args[])
{
    setup();
    char szUserId[9];
    int seq = -1;
    int err = 0;
    int err2 = 0;
    int version = 1;
    char err_msg[81];
    char err_msg2[81];
    int shot = 8;
    int spqi = 1;
    int gpqi = -1;
    int ndda = 1;
    char ddas[5] = "MAGN";
    const int mseqnum = 10;
    int seqnum[ mseqnum ];

    int iwdat[12];
    const int mcomlen = 120;
    const int mddalen = 30;

    //int ndda = mddalen;
    int comlen = mcomlen;

    char szComment[ mcomlen ];
    char szDDAs[ (mddalen*4) + 1 ];

    /*


    call_ppfuid( "JETPPF", "R", &err );


    call_ppfgo(0,0, &err);

    call_pdmseq( 8, &seq, "JETPPF", "****", &err);
    call_ppfclo( 8, &seq, "dsand", version, &err );

    call_ppferr( "PPFCLO", err, err_msg, &err2 );

    call_ppfsqi( shot, spqi, &err );
    call_ppferr( "PPFSQI", err, err_msg, &err2 );

    call_ppfgqi( shot, &gpqi, &err );
    call_ppferr( "PPFGQI", err, err_msg2, &err2 );

    call_ppfgid( "R", szUserId, &err );

    call_ppfpok( &err );

    char szPrgNam[ 9 ];
    char szPPFNam[ 25 ];

    memset( szPrgNam, 0, 9 );
    memset( szPPFNam, 0, 25 );
    szPrgNam[8]='\0';
    szPPFNam[24]='\0';

    call_pdainf( szPrgNam, &version, szPPFNam, &err );
    call_ppferr( "PDAINF", err, err_msg2, &err2 );

    memset( seqnum, 0, mseqnum*sizeof(int) );
    call_ppfseq( &shot, &ndda, ddas, seqnum, &err );

    ndda = mddalen;
    call_ppfinf( &shot, &seq, comlen, iwdat, szComment, &err, &ndda, szDDAs );
    call_ppferr( "PPFINF", err, err_msg2, &err2 );

    int nwcom = 20;
    char szDDACom[ (nwcom*4) + 1 ];
    const int mndt = 300;
    int ndt = mndt;
    int lxtv[ 2 * mndt ];
    char szDTNAMS[ 5 * mndt ];

    call_ddainf( shot, "MAGN", &nwcom, szDDACom, &ndt, szDTNAMS, lxtv,  &err );
    call_ppferr( "DDAINF", err, err_msg2, &err2 );

    call_ppfdel( shot, seq, &err );


    int mxseq = 10;
    int mxdda = 10;
    int nseq = mxseq;
    int lseq[mxseq];
    int ldda[mxseq];
    ndda = mxdda;
    char cdda[ (ndda * nseq * 4)+1 ];
    //int uid = -1;

    call_pdlppf( shot, 1, 1, &nseq, lseq, ldda, &ndda, cdda
           , &err     );


    const int mxusers = 10;
    int nusers = mxusers;
    char szUsers[ (mxusers * 8) + 1];
    call_pdlusr( &shot, "NBPR", &nusers, szUsers, &err );

    call_pdmsdt( 300714, "00:00:00", &shot, &err );

    call_pdmsht( &shot );

    char szLoginName[9];
    char szStatus[61];
    int status = 0;

    call_pdstat( szLoginName, &status, szStatus, &err );

    int itime = 0;
    int idate = 0;

    call_pdstd( shot, &itime, &idate, &err );

    const int mxndda = 10;
    char szDDAList[(mxndda * 4)+1];
    call_pdsrch( shot
           , "NBPR"
           , &seq
           , &ndda, szDDAList
           , &err );



    const int mxndt = 512;
    ndt = mxndt;
    char dtnams[mxndt*4];
    int lxtv2[mxndt*2];
    char comments[mxndt*24+1];

    call_pdtinf( shot, "NBPR", &ndt, dtnams, lxtv2, comments, &err );


    const int mxX = 10;
    const int mxT = 10;

    int nx = mxX;
    int nt = mxT;

    float t[mxT];
    float x[mxX];
    float data[ mxT * mxX ];

    call_ppfdat( shot, 0, "NBPR", "AAAA"
           , &nx, x
           , &nt, t
           , data
           , &err );



    char szFormat[2];
    char szUnits[9];
    char szComm[25];
    int systat, ustat;
    call_ppfdti( shot,  seq
           , "NBPR", "AAAA"
           , &nx, &nt
           , szFormat, szUnits
           , szComm
           , &systat, &ustat
           , &err );

    const int mxseqs = 10;
    nseq = 10;
    int seqs[mxseqs];

    call_ppfdda( shot, "NBPR", &nseq, seqs, &err );


    int irdat2[13];
    int iwdat2[13];
    char ihdat2[61];

    memset( irdat2, 0, sizeof(int) * 13 );
    memset( iwdat2, 0, sizeof(int) * 13 );
    memset( ihdat2, 0, 61 );

    ihdat2[60]='\0';

    //irdat2[0] = -1;
    //irdat2[1] = mxX;
    //irdat2[2] = -1;
    //irdat2[3] = mxT * mxT;

    irdat2[4] = mxX * mxT;
    irdat2[5] = mxX;
    irdat2[6] = mxT;


    call_ppfget( shot
           , "NBPR", "AAAA"
    	  , irdat2, ihdat2, iwdat2
           , data, x, t
           , &err);

    int status_flags[200];
    call_ppfgsf( shot, seq
           , "****", "****"
           , status_flags
           , &err);

    float tgot = 0.0;
    cal_ppfgts( shot,  "NBPR",  "AAAA"
          , 1, irdat2, ihdat2,  iwdat2, data, x, &tgot, &err );

    int date, time, moddate, modtime, modseq;

    call_ppfmod( shot, &date, &time, &seq, &moddate, &modtime, &modseq, &err );

    int isondisk = -1;
    call_ppfondisk( shot, "NBIP", &isondisk, &err );

    char szOwner[9];
    char szPrevOwner[9];
    char szAuthor[9];
    call_ppfownerinfo( shot, seq, 1, szOwner, szPrevOwner, szAuthor, &err );

    call_ppfsetdevice( "bob" );

    int nppf = 0;
    int ndtype = 0;
    ndda = 0;

    call_ppfsiz( shot, seq
           ,  "****", "JETPPF"
           , &nppf, &ndda, &ndtype, &err );
    call_ppferr( "PPFSIZ", err, err_msg, &err2 );
    */
    call_help();
    /*
    call_readNothing();
    */
    tearDown();

    return 0;
}




