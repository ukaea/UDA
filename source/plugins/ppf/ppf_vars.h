#ifndef IDAM_PLUGINS_PPF_PPF_VARS_H
#define IDAM_PLUGINS_PPF_PPF_VARS_H

#include <clientserver/idamStructs.h>
#include <server/idamPluginStructs.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define STR_FUNCTION_TAG "f"
#define STR_SHOT "shot"
#define STR_PULSE "pulse"
#define STR_SEQ "seq"
#define STR_OWNER "owner"
#define STR_DDA "dda"
#define STR_PVERS "version"

#define STR_USERID "userid"
#define STR_RW_FLAG "rw"

#define STR_PRGNAME "prgnam"

#define STR_RNAME "rname"
#define STR_ERRCODE "error_code"

#define STR_PQI "pqi"

enum ERRORS
{
    ERR_NONE = 0,
    ERR_UNKNOWN_FUNCTION,
    ERR_FUNCTION_NOT_SUPPORTED,
    ERR_PDMSEQ_SETUP,
    ERR_PDMSEQ_EXECUTE,
    ERR_PPFUID_SETUP,
    ERR_PPFGO_SETUP,
    ERR_PPFCLO_SETUP,
    ERR_PPFERR_SETUP,
    ERR_PPFSQI_SETUP,
    ERR_PPFGQI_SETUP,
    ERR_PPFGID_SETUP,
    ERR_PPFPOK_SETUP,
    ERR_PDAINF_SETUP,
    ERR_PPFSEQ_SETUP,
    ERR_PPFINF_SETUP,
    ERR_DDAINF_SETUP,
    ERR_PPFDEL_SETUP,
    ERR_PPFWRI_SETUP,
    ERR_PDLPPF_SETUP,
    ERR_PDLPPPF_EXECUTE,
    ERR_PDLUSR_SETUP,
    ERR_PDMSDT_SETUP,
    ERR_PDSTAT_SETUP,
    ERR_PDSTD_SETUP,
    ERR_PDSRCH_SETUP,
    ERR_PDTINF_SETUP,
    ERR_PPFDAT_SETUP,
    ERR_PPFDTI_SETUP,
    ERR_PPFDDA_SETUP,
    ERR_PPFGET_SETUP,
    ERR_PPFGSF_SETUP,
    ERR_PPFGTS_SETUP,
    ERR_PPFMOD_SETUP,
    ERR_PPFONDISK_SETUP,
    ERR_PPFOWNERINFO_SETUP,
    ERR_PPFSETDEVICE_SETUP,
    ERR_PPFSIZ_SETUP,
    ERR_LAST
};

extern const char * pszErrorStrings[];

int nvl_count(const NAMEVALUELIST * nvl);

const NAMEVALUE * nvl_pair(const NAMEVALUELIST * nvl, int idx);

const char * nv_name(const NAMEVALUE * nv);

const char * nv_value(const NAMEVALUE * nv);

const NAMEVALUE * nvl_name_find(const NAMEVALUELIST * nvl, const char * name);

struct VAR_PPFGO
{
    int shot;
    int seq;
    int err;
};

typedef struct VAR_PPFGO VAR_PPFGO;

#define xstr(s) str(s)
#define str(s) #s

#define DECLARE_PPF_INIT(V) int V ## _init( VAR_ ## V* var)
#define DECLARE_PPF_SETUP(V) int V ## _setup( VAR_ ## V* var, IDAM_PLUGIN_INTERFACE* ipi )
#define DECLARE_PPF_EXECUTE(V) int V ## _execute( VAR_ ## V* var)
#define DECLARE_PPF_DATABLOCK(V) int V ## _DATABLOCK( const VAR_ ## V* var, DATA_BLOCK* db)
#define DECLARE_PPF_FREE(V) int V ## _free( VAR_ ## V* var)

DECLARE_PPF_INIT(PPFGO);

DECLARE_PPF_SETUP(PPFGO);

DECLARE_PPF_EXECUTE(PPFGO);

DECLARE_PPF_DATABLOCK(PPFGO);

struct VAR_PDMSEQ
{
    int pulse;
    int seq;
    int err;
    char szOwner[9];
    char szDDA[5];
};
typedef struct VAR_PDMSEQ VAR_PDMSEQ;

DECLARE_PPF_INIT(PDMSEQ);

DECLARE_PPF_SETUP(PDMSEQ);

DECLARE_PPF_EXECUTE(PDMSEQ);

DECLARE_PPF_DATABLOCK(PDMSEQ);

struct VAR_PPFUID
{
    int err;
    char szUID[9];
    char rw[2];
};
typedef struct VAR_PPFUID VAR_PPFUID;

DECLARE_PPF_INIT(PPFUID);

DECLARE_PPF_SETUP(PPFUID);

DECLARE_PPF_EXECUTE(PPFUID);

DECLARE_PPF_DATABLOCK(PPFUID);

struct VAR_PPFCLO
{
    int shot;
    int seq;
    int unit;
    int pvers;
    int err;
    char prgname[9];
};
typedef struct VAR_PPFCLO VAR_PPFCLO;


DECLARE_PPF_INIT(PPFCLO);

DECLARE_PPF_SETUP(PPFCLO);

DECLARE_PPF_EXECUTE(PPFCLO);

DECLARE_PPF_DATABLOCK(PPFCLO);

#define ERR_MSG_LEN 80

struct VAR_PPFERR
{
    char rname[7];
    int err_code;
    char msg[ERR_MSG_LEN + 1];
    int err;
};
typedef struct VAR_PPFERR VAR_PPFERR;

DECLARE_PPF_INIT(PPFERR);

DECLARE_PPF_SETUP(PPFERR);

DECLARE_PPF_EXECUTE(PPFERR);

DECLARE_PPF_DATABLOCK(PPFERR);

struct VAR_PPFSQI
{
    int shot;
    int pqi;
    int err;
};
typedef struct VAR_PPFSQI VAR_PPFSQI;

DECLARE_PPF_INIT(PPFSQI);

DECLARE_PPF_SETUP(PPFSQI);

DECLARE_PPF_EXECUTE(PPFSQI);

DECLARE_PPF_DATABLOCK(PPFSQI);

struct VAR_PPFGQI
{
    int shot;
    int pqi;
    int err;
};
typedef struct VAR_PPFGQI VAR_PPFGQI;

DECLARE_PPF_INIT(PPFGQI);

DECLARE_PPF_SETUP(PPFGQI);

DECLARE_PPF_EXECUTE(PPFGQI);

DECLARE_PPF_DATABLOCK(PPFGQI);

struct VAR_PPFGID
{
    char rw[1];
    char userid[9];
    int err;
};

typedef struct VAR_PPFGID VAR_PPFGID;

DECLARE_PPF_INIT(PPFGID);

DECLARE_PPF_SETUP(PPFGID);

DECLARE_PPF_EXECUTE(PPFGID);

DECLARE_PPF_DATABLOCK(PPFGID);

struct VAR_PPFPOK
{
    int err;
};

typedef struct VAR_PPFPOK VAR_PPFPOK;

DECLARE_PPF_INIT(PPFPOK);

DECLARE_PPF_SETUP(PPFPOK);

DECLARE_PPF_EXECUTE(PPFPOK);

DECLARE_PPF_DATABLOCK(PPFPOK);

#define PRGNAM_LEN 8
#define PPFNAM_LEN 24

struct VAR_PDAINF
{
    int version;
    char prgnam[PRGNAM_LEN + 1];
    char ppfnam[PPFNAM_LEN + 1];
    int err;
};

typedef struct VAR_PDAINF VAR_PDAINF;

DECLARE_PPF_INIT(PDAINF);

DECLARE_PPF_SETUP(PDAINF);

DECLARE_PPF_EXECUTE(PDAINF);

DECLARE_PPF_DATABLOCK(PDAINF);

struct VAR_PPFSEQ
{
    int err;
    int shot;
    int * pSeqNums;
    int * pModDates;
    int * pModTimes;
    char * pDDAs;
    int ndda;
};
typedef struct VAR_PPFSEQ VAR_PPFSEQ;

DECLARE_PPF_INIT(PPFSEQ);

DECLARE_PPF_SETUP(PPFSEQ);

DECLARE_PPF_EXECUTE(PPFSEQ);

DECLARE_PPF_DATABLOCK(PPFSEQ);

DECLARE_PPF_FREE(PPFSEQ);
//int PPFSEQ_free( VAR_PPFSEQ* var );


struct VAR_PPFINF
{
    int shot;
    int seq;
    int comlen;
    int err;
    int ndda;


    int iwdat[12];
    char * pszComment;
    char * pszDDAs;

};
typedef struct VAR_PPFINF VAR_PPFINF;

DECLARE_PPF_INIT(PPFINF);

DECLARE_PPF_SETUP(PPFINF);

DECLARE_PPF_EXECUTE(PPFINF);

DECLARE_PPF_DATABLOCK(PPFINF);

DECLARE_PPF_FREE(PPFINF);

struct VAR_DDAINF
{
    int shot;
    int err;
    char szDDA[5];
    int nwcom;
    char * pszDDACom;
    int ndt;
    char * pszDTNAMS;
    int * lxtv;
};
typedef struct VAR_DDAINF VAR_DDAINF;

DECLARE_PPF_INIT(DDAINF);

DECLARE_PPF_SETUP(DDAINF);

DECLARE_PPF_EXECUTE(DDAINF);

DECLARE_PPF_DATABLOCK(DDAINF);

DECLARE_PPF_FREE(DDAINF);

struct VAR_PPFDEL
{
    int shot;
    int seq;
    int err;
};
typedef struct VAR_PPFDEL VAR_PPFDEL;

DECLARE_PPF_INIT(PPFDEL);

DECLARE_PPF_SETUP(PPFDEL);

DECLARE_PPF_EXECUTE(PPFDEL);

DECLARE_PPF_DATABLOCK(PPFDEL);

/*
struct VAR_PPFWRI
{
  int shot;
  int err;
  int irdat[13];
  int iwdat[13];
  char szDDA[5];
  char szDTYPE[5];
  char ihdat[60];

  float* data;
  float* x;
  float* t;
};
typedef struct VAR_PPFWRI VAR_PPFWRI;
DECLARE_PPF_INIT( PPFWRI );
DECLARE_PPF_SETUP( PPFWRI );
DECLARE_PPF_EXECUTE( PPFWRI );
DECLARE_PPF_DATABLOCK( PPFWRI );
*/

struct VAR_PDLPPF
{
    int shot;
    int uid;
    int dup;
    int nseq;
    int * lseq;
    int ndda;
    int * ldda;
    char * cdda;
    int err;
};
typedef struct VAR_PDLPPF VAR_PDLPPF;

DECLARE_PPF_INIT(PDLPPF);

DECLARE_PPF_SETUP(PDLPPF);

DECLARE_PPF_EXECUTE(PDLPPF);

DECLARE_PPF_DATABLOCK(PDLPPF);

DECLARE_PPF_FREE(PDLPPF);

struct VAR_PDLUSR
{
    int shot;
    int nusers;
    char szDDA[5];
    char * pszUsers;
    int err;
};
typedef struct VAR_PDLUSR VAR_PDLUSR;

DECLARE_PPF_INIT(PDLUSR);

DECLARE_PPF_SETUP(PDLUSR);

DECLARE_PPF_EXECUTE(PDLUSR);

DECLARE_PPF_DATABLOCK(PDLUSR);

DECLARE_PPF_FREE(PDLUSR);

struct VAR_PDMSDT
{
    int idate;
    char szTime[9];
    int shot;
    int err;
};
typedef struct VAR_PDMSDT VAR_PDMSDT;

DECLARE_PPF_INIT(PDMSDT);

DECLARE_PPF_SETUP(PDMSDT);

DECLARE_PPF_EXECUTE(PDMSDT);

DECLARE_PPF_DATABLOCK(PDMSDT);
//DECLARE_PPF_FREE( PDLUSR );

struct VAR_PDMSHT
{
    int shot;
    int err;
};
typedef struct VAR_PDMSHT VAR_PDMSHT;

DECLARE_PPF_INIT(PDMSHT);
//DECLARE_PPF_SETUP( PDMSDT );
DECLARE_PPF_EXECUTE(PDMSHT);

DECLARE_PPF_DATABLOCK(PDMSHT);

struct VAR_PDSTAT
{
    int status;
    int err;
    char szLoginName[9];
    char szStatusString[61];
};

typedef struct VAR_PDSTAT VAR_PDSTAT;

DECLARE_PPF_INIT(PDSTAT);

DECLARE_PPF_SETUP(PDSTAT);

DECLARE_PPF_EXECUTE(PDSTAT);

DECLARE_PPF_DATABLOCK(PDSTAT);


struct VAR_PDSTD
{
    int err;
    int shot;
    int time;
    int date;
};

typedef struct VAR_PDSTD VAR_PDSTD;

DECLARE_PPF_INIT(PDSTD);

DECLARE_PPF_SETUP(PDSTD);

DECLARE_PPF_EXECUTE(PDSTD);

DECLARE_PPF_DATABLOCK(PDSTD);


struct VAR_PDSRCH
{
    int err;
    int shot;
    int seq;
    int ndda;
    char szDDA[5];
    char * pszDDAs;
};

typedef struct VAR_PDSRCH VAR_PDSRCH;

DECLARE_PPF_INIT(PDSRCH);

DECLARE_PPF_SETUP(PDSRCH);

DECLARE_PPF_EXECUTE(PDSRCH);

DECLARE_PPF_DATABLOCK(PDSRCH);

DECLARE_PPF_FREE(PDSRCH);


struct VAR_PDTINF
{
    int err;
    int shot;
    char szDDA[5];
    int ndt;
    char * pszDTNAMS;
    int * lxtv;
    char * pszDTComms;
};

typedef struct VAR_PDTINF VAR_PDTINF;

DECLARE_PPF_INIT(PDTINF);

DECLARE_PPF_SETUP(PDTINF);

DECLARE_PPF_EXECUTE(PDTINF);

DECLARE_PPF_DATABLOCK(PDTINF);

DECLARE_PPF_FREE(PDTINF);


struct VAR_PPFDAT
{
    int err;
    int shot;
    int seq;
    int nx;
    int nt;
    char szDDA[5];
    char szDTYPE[5];

    float * data;
    float * x;
    float * t;
};

typedef struct VAR_PPFDAT VAR_PPFDAT;

DECLARE_PPF_INIT(PPFDAT);

DECLARE_PPF_SETUP(PPFDAT);

DECLARE_PPF_EXECUTE(PPFDAT);

DECLARE_PPF_DATABLOCK(PPFDAT);

DECLARE_PPF_FREE(PPFDAT);

struct VAR_PPFDTI
{
    int err;
    int shot;
    int seq;
    int nx;
    int nt;

    int systat, ustat;

    char chFormat[2];
    char szDDA[5];
    char szDTYPE[5];
    char szUnits[9];
    char szComm[25];
};

typedef struct VAR_PPFDTI VAR_PPFDTI;

DECLARE_PPF_INIT(PPFDTI);

DECLARE_PPF_SETUP(PPFDTI);

DECLARE_PPF_EXECUTE(PPFDTI);

DECLARE_PPF_DATABLOCK(PPFDTI);
//DECLARE_PPF_FREE( PPFDTI );


struct VAR_PPFDDA
{
    int shot;
    int nseq;
    int * pseqs;
    int err;
    char szDDA[5];
};
typedef struct VAR_PPFDDA VAR_PPFDDA;

DECLARE_PPF_INIT(PPFDDA);

DECLARE_PPF_SETUP(PPFDDA);

DECLARE_PPF_DATABLOCK(PPFDDA);

DECLARE_PPF_EXECUTE(PPFDDA);

DECLARE_PPF_FREE(PPFDDA);


struct VAR_PPFGET
{
    int shot;
    int err;
    int irdat[13];
    int iwdat[13];
    char szDDA[5];
    char szDTYPE[5];
    char ihdat[61];

    float * data;
    float * x;
    float * t;
};

typedef struct VAR_PPFGET VAR_PPFGET;

DECLARE_PPF_INIT(PPFGET);

DECLARE_PPF_SETUP(PPFGET);

DECLARE_PPF_DATABLOCK(PPFGET);

DECLARE_PPF_EXECUTE(PPFGET);

DECLARE_PPF_FREE(PPFGET);


#define VAR_PPFGSF_MXSTATS 200
struct VAR_PPFGSF
{
    int shot;
    int seq;
    //const int mxstats = 200;
    int stats[VAR_PPFGSF_MXSTATS];
    int err;

    char szDDA[5];
    char szDTYPE[5];
};
typedef struct VAR_PPFGSF VAR_PPFGSF;

DECLARE_PPF_INIT(PPFGSF);

DECLARE_PPF_SETUP(PPFGSF);

DECLARE_PPF_DATABLOCK(PPFGSF);

DECLARE_PPF_EXECUTE(PPFGSF);
//DECLARE_PPF_FREE( PPFGSF );

struct VAR_PPFGTS
{
    int shot;
    int err;
    char szDDA[5];
    char szDTYPE[5];

    int irdat[13];

    int iwdat[13];
    char ihdat[61];

    float twant;
    float tgot;
    float * data;
    float * x;
};
typedef struct VAR_PPFGTS VAR_PPFGTS;

DECLARE_PPF_INIT(PPFGTS);

DECLARE_PPF_SETUP(PPFGTS);

DECLARE_PPF_DATABLOCK(PPFGTS);

DECLARE_PPF_EXECUTE(PPFGTS);

DECLARE_PPF_FREE(PPFGTS);

struct VAR_PPFMOD
{
    int shot;
    int time;
    int date;
    int seq;
    int moddate;
    int modtime;
    int modseq;
    int err;
};
typedef struct VAR_PPFMOD VAR_PPFMOD;

DECLARE_PPF_INIT(PPFMOD);

DECLARE_PPF_SETUP(PPFMOD);

DECLARE_PPF_DATABLOCK(PPFMOD);

DECLARE_PPF_EXECUTE(PPFMOD);
//DECLARE_PPF_FREE( PPFMOD );

struct VAR_PPFONDISK
{
    int shot;
    char szDDA[5];
    int ondisk;
    int err;
};

typedef struct VAR_PPFONDISK VAR_PPFONDISK;

DECLARE_PPF_INIT(PPFONDISK);

DECLARE_PPF_SETUP(PPFONDISK);

DECLARE_PPF_DATABLOCK(PPFONDISK);

DECLARE_PPF_EXECUTE(PPFONDISK);

struct VAR_PPFOWNERINFO
{
    int shot;
    int seq;
    int ownerflag;
    int err;
    char szOwner[9];
    char szPrevOwner[9];
    char szAuthor[9];
};
typedef struct VAR_PPFOWNERINFO VAR_PPFOWNERINFO;

DECLARE_PPF_INIT(PPFOWNERINFO);

DECLARE_PPF_SETUP(PPFOWNERINFO);

DECLARE_PPF_DATABLOCK(PPFOWNERINFO);

DECLARE_PPF_EXECUTE(PPFOWNERINFO);

struct VAR_PPFSETDEVICE
{
    char szDevice[9];
    int err;
};
typedef struct VAR_PPFSETDEVICE VAR_PPFSETDEVICE;

DECLARE_PPF_INIT(PPFSETDEVICE);

DECLARE_PPF_SETUP(PPFSETDEVICE);

DECLARE_PPF_DATABLOCK(PPFSETDEVICE);

DECLARE_PPF_EXECUTE(PPFSETDEVICE);

struct VAR_PPFSIZ
{
    int shot;
    int seq;
    int nppf;
    int ndda;
    int ndtype;
    int err;
    char szDDA[5];
    char szUser[9];
};
typedef struct VAR_PPFSIZ VAR_PPFSIZ;

DECLARE_PPF_INIT(PPFSIZ);

DECLARE_PPF_SETUP(PPFSIZ);

DECLARE_PPF_DATABLOCK(PPFSIZ);

DECLARE_PPF_EXECUTE(PPFSIZ);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_PPF_PPF_VARS_H
