/*
YB 
*/

#define THE_SIZE	13
#define NB_OBJETS	64   /* general purpose memory allocator */

/*   Decodage des formats   */

#define OCTET		1
#define STRING12	2
#define INT16		3
#define INT32		4
#define REEL16		5
#define REEL32		6
#define TWIN_INT16	7
#define TWIN_REEL32	8
#define INT_REEL32	9
#define VOIE		10
#define FONCTION	11
#define VOIE_GAIN	12
#define REGISTRE	13
#define F_STRAT_DIAG	14
#define STRAT_PILOTE	15
#define DECL_COND	16
#define EVT_EXTERNE	17
#define SERNAM		18
#define CADRAN		19
#define LIEN		20
#define DECL_SEQ	21
#define VOIE_SEQ	22
#define TAB_SEQ		23
#define T_DECODE	24
#define T_IT		25
#define T_ENCODE	26
#define F_COMMON	27
#define F_CHRONO	28
#define T_DECLENC	29
#define L_DIAG		30
#define T_DECLENCHE	31
#define TEXTE		32
#define F_TOPMSDIV	33
#define GRAPHE		34
#define SIGNAL		35
#define FLOAT_CHAR64	36
#define INT2_CHAR64	37
#define F_ACHRONO  	38
#define F_DESCRIPTEUR	39

/*  Decodage des types d'objets  */

#define DIAGNOSTIC	1
#define DONNEE		2
#define ICM100		3
#define ICM101		4
#define CAMAC2162	5
#define CAMAC6103	6
#define CAMAC6810	7
#define CAMAC8818	8
#define CHRONO		9
#define DATE1		10
#define DATE2		11
#define SEQ_NUM		12
#define GENE_FONCTION	13
#define DECOD_CHRONO	14
#define ENCOD_CHRONO	15
#define STRAT_DIAG	16
#define PILOTAGE	17
#define CHRONOVME	18
#define TRAITEMENT	19
#define ASSERVISSEMENT	20
#define CONSIGNE	21
#define DONGEN		22
#define STRAT_PIL	23
#define OBJET_CHRONO	24
#define TORE_SUPRA	25
#define COMMON		26
#define ICV101          27
#define ACHRONO         28
#define TOUS		100

/*  Les miens  */
#define OBJ_CADRAN	500
#define OBJ_D_CHRONO	501
#define OBJ_EVT_EXT	502
#define OBJ_SERNAM	503

/* For the silly Ingres DB  */

#define ING_NOT_FOUND 100    /* le tuple recherche n'existe pas dans la base */
#define ING_OK     0      /* code retour 'OK' de Ingres */

/*
 * Debug macro, based on the traceflag.
 * Note that a daemon typically freopen()s stderr to another file
 * for debugging purposes.
 */

/*#define DEBUG(fmt)              if (traceflag) { \
                                        fprintf(stderr, fmt); \
                                        fputc('\n', stderr); \
                                        fflush(stderr); \
                                } else ;

#define DEBUG1(fmt, arg1)       if (traceflag) { \
                                        fprintf(stderr, fmt, arg1); \
                                        fputc('\n', stderr); \
                                        fflush(stderr); \
                                } else ;

#define DEBUG2(fmt, arg1, arg2) if (traceflag) { \
                                        fprintf(stderr, fmt, arg1, arg2); \
                                        fputc('\n', stderr); \
                                        fflush(stderr); \
                                } else ;

#define DEBUG3(fmt, arg1, arg2, arg3) if (traceflag) { \
                                       fprintf(stderr, fmt, arg1, arg2, arg3); \
                                       fputc('\n', stderr); \
                                       fflush(stderr); \
                                } else ;

#define DEBUG4(fmt, arg1, arg2, arg3,arg4) if (traceflag) { \
                                fprintf(stderr, fmt, arg1, arg2, arg3, arg4); \
                                fputc('\n', stderr); \
                                fflush(stderr); \
                                } else ;*/


