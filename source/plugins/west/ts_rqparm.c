#include "ts_rqparam.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include "tsdef.h"
#include "tsconf.h"
#include "tstype.h"
#include "def.h"
#include "tsbamat.h"
#include "tserreur.h"
#include <logging/logging.h>

struct sigaction newact, oldact;
sigset_t newmask, oldmask;
char tslib_reel = 'f';

int cr;
int extract = 1;
int l_rang = 1;      /* lecture rang par defaut */         /** +++  **/
int nb_extr2 = -1; /* nbre mesures quand extract = 2 */  /** +++  **/
int extraction, np_specif;
int num;
int nouttemp;
int type, absolute;

//float CONVERT_TIME_FACTOR = 1e-6;


/*extern int tsparse (char *string, char *nomsig, int indices[], int *extr, int *occur);

extern int TSExist (char *NomDon,
		 pS_Entree Entrees,
		 int *NbCoord,
		 int *NumVer,
		 int *Certif,
		 char *Date,
		 char *Heure);

extern int TSExist_gen (char * NomDon, pS_Entree Entrees, int NbMes [], int * NumVer, int Certif [] , char * Date, char * Heure,
		char * nom_gen, int *indg);

extern int TSDescDon_choc ( char * NomDon, int shot, int occur, ts_donnee **ptDescDon);

extern int TSGrpRg (char *NomDon,
		 pS_Entree Entrees,
		 int *Rangs,
		 int NbMax,
		 int Extract,
		 pS_Unite * Unite,
		 int *NumVer,
		 int *Certif,
		 char *Date,
		 char *Heure,
		 int *NbMes,
		 int *MaxReel,
		 char * X,
		 char * Y,
		 char *Coord);

extern int TSRSigRg (char *NomDon,
		  pS_Entree Entrees,
		  int *Rangs,
		  int NbMax,
		  int Extract,
		  pS_Unite * Unite,
		  int *NumVer,
		  int *Certif,
		  char *Date,
		  char *Heure,
		  int *NbMes,
		  int *MaxReel,
		  char * X,
		  char * Y);

extern int TSGrpX (char *NomDon,
		pS_Entree Entrees,
		float XI,
		float XF,
		int NbMax,
		int Extract,
		pS_Unite * Unite,
		int *NumVer,
		int *Certif,
		char *Date,
		char *Heure,
		int *NbMes,
		int *MaxReel,
		char * X,
		char * Y,
		char *Coord);

extern int TSRSigX (char *NomDon,
		 pS_Entree Entrees,
		 float XI,
		 float XF,
		 int NbMax,
		 int Extract,
		 pS_Unite * Unite,
		 int *NumVer,
		 int *Certif,
		 char *Date,
		 char *Heure,
		 int *NbMes,
		 int *MaxReel,
		 char * X,
		 char * Y);

extern int TSXtrRg (char *NomDon,
	 pS_Entree Entrees,
	 int *Indices,
	 int *Rangs,
	 int NbMax,
	 int Extract,
	 pS_Unite * Unite,
	 int *NumVer,
	 int *Certif,
	 char *Date,
	 char *Heure,
	 int *NbMes,
	 int *MaxReel,
	 char * X,
	 char * Y,
	 char *Coord);

extern int TSXtrX (char *NomDon,
	pS_Entree Entrees,
	int *Indices,
	float XI,
	float XF,
	int NbMax,
	int Extract,
	pS_Unite * Unite,
	int *NumVer,
	int *Certif,
	char *Date,
	char *Heure,
	int *NbMes,
	int *MaxReel,
	char * X,
	char * Y,
	char *Coord);
 */

//extern int TSRqParm(int num_choc, char* nom_prod, char* nom_objet, char* nom_param, int val_nb, char **pt_char, int *nb_val, int *format);

int lit_traite(char* nomsigp, int numchoc, int occ,
		int rang[], float** X, float** Y, int* maxreel);
void meser(char* mes1, char* mes2, int numer, int code);

int readStaticParameters(char** pt_char, int* nb_val, int num_choc, char* nom_prod, char* nom_objet, char* nom_param,
		int val_nb)
{
	int format;

	UDA_LOG(UDA_LOG_DEBUG, "Producer: %s\n", nom_prod);
	UDA_LOG(UDA_LOG_DEBUG, "Object: %s\n", nom_objet);
	UDA_LOG(UDA_LOG_DEBUG, "Parameter: %s\n", nom_param);

	cr = TSRqParm(num_choc, nom_prod, nom_objet, nom_param, val_nb, pt_char, nb_val, &format);
	return cr;
}

int readSignal(char* nomsigp, int numchoc, int occ,
		int rang[], float** X, float** Y, int* len)
{
	UDA_LOG(UDA_LOG_DEBUG, "Reading signal: %s\n", nomsigp);

	cr = lit_traite(nomsigp, numchoc, occ, rang, (float**)X, (float**)Y, (int*)len);

	//    UDA_LOG(LOG_DEBUG, "%s\n", "First time values...");
	//    int j;
	//    for (j=0; j <10; j++) {
	//    		UDA_LOG(LOG_DEBUG, "value : %f\n", *(*X + j));
	//    }
	if (cr == 0) {
		int i;
		for (i = 0; i < *len; i++)
			*(*X + i) = *(*X + i) / CTPS; //TSLib gives the time in µs, so we do this conversion to put the time in seconds
	}
	return cr;
}

int
lit_traite(char* nomsigp, int numchoc, int occ,
		int rang[], float** X, float** Y, int* maxreel)
{
	//TODO : mxArray * pout[];
	float xab[2] = { 0, 0 };
	int nout = 2;
	register int i, j;
	int nbmax, nbmes, nbmesgr[3], lind[5], cr, cr1, numv;
	int nbcoord[MAX_COORD], nb_certif, rgrp = 0;
	/** +++ **/
	/* int	rang[2], maxreel, cooked_d = 0, fcomment = 0;*/
	int cooked_d = 0, fcomment = 0;

	int indices[2], rscalaire = 0, scomment = 0;
	int certif[MAX_CERTIF];

	float Coord[10 * MAX_SIG];
	char* Xc, * Yc;
	char date[TAILLE_DATE], heure[TAILLE_HEURE], cmisc[TAILLE_MISC] = { '\0' };
	char* ptd, nomsig[TAILLE_NOM_DONNEE], nom_gen[TAILLE_NOM_DONNEE];
	char aff[64], nomsig_misc[TAILLE_NOM_DONNEE + 4];
	/********* New for ARCAD ************/
	/* pS_DescDon ptDescDon; */
	ts_donnee* ptDescDon;
	/********* New for ARCAD ************/
	pS_Entree ptEntree;
	pS_Unite ptUnite = NULL;
	//float *pr;
	int occur = 0, indg = 0;
	short pansement = 0, generique = 1;

	ptEntree = (pS_Entree)calloc(1, sizeof(struct S_Entree));
	ptEntree->union_var.lval = numchoc;

	/*****++++++++++++++++++++++++++++++++ MAJ Mars 2002 ****************/
	if (nomsigp[0] == '!') {
		strcpy(nomsigp, &nomsigp[1]);
		generique = 0;
	}
	/*****++++++++++++++++++++++++++++++++ MAJ Mars 2002 ****************/

	tsparse(nomsigp, nomsig, indices, &rgrp, &occur);
	//tsparse (char *string, char *nomsig, int indices[], int *extr, int *occur)

	if (occ) {
		occur = occ;
	}

	if (occur) {
		ptEntree->ps_suivant = (pS_Entree)calloc(1, sizeof(struct S_Entree));
		ptEntree->ps_suivant->union_var.lval = occur;
		ptEntree->ps_suivant->ps_suivant = NULL;
	} else {
		ptEntree->ps_suivant = NULL;
	}

	for (i = 0; i < MAX_CERTIF; i++)
		certif[i] = 0;

	for (i = 0; i < MAX_COORD; i++)
		nbcoord[i] = 0;
#ifdef DEBUG
printf ("avant Tsexist signal %s choc %d \n", nomsig, numchoc);
#endif
cr = TSExist_gen(nomsig, ptEntree, nbcoord, &numv, certif, date, heure,
		nom_gen, &indg);

if ((strncmp(nomsig, nom_gen, strlen(nomsig))) && (nom_gen[0] == 'G')) {
	pansement = 1;
}

/*****++++++++++++++++++++++++++++++++ MAJ Mars 2002 ****************/
if ((extract > 3) && ((nom_gen[0] == 'g') || (nom_gen[0] == 'G')) &&
		(rgrp == 0)) {
	extract = 1;
}

if (generique)
	strcpy(nomsig, nom_gen);
if (indg != 0) {
	rgrp = 1;
	indices[0] = indg;
	sprintf(aff, "lisant %s %d !", nomsig, indg);
} else
	sprintf(aff, "lisant %s !", nomsig);

#ifdef DEBUG
printf ("Tsexist cr = %d nbcoord[0] %d \n", cr, nbcoord[0]);
#endif
if (cr || (nbcoord[0] == 0)) {                /* ncoord[0]= 0 si certif <= -2 */
	if (cr) {            /* verification de l'existence du choc */
		cr1 = TSExist("num_choc", ptEntree, nbcoord, &numv, certif, date, heure);
		if (cr1) {            /* le choc n'existe pas */

			switch (cr1) {
			case 103:
				printf("Tslib Server down !!! \n");
				printf("N'en fau tasta per lou saupre \n");
				break;
			default:
				printf("TSExist - %s Choc absent cr1 = %d\n",
						nomsig, cr1);
			}
			free(ptEntree);
			UDA_LOG(UDA_LOG_DEBUG, "Returning from 1\n");
			return (cr);
		}
	}
#ifdef DEBUG
	printf ("Tsexist cr = %d nbcoord[0] %d \n", cr, nbcoord[0]);
#endif
	/* Unites , date , heure , certifs ...   quand certif <= -2 */
	if (nbcoord[0] == 0) {            /* certif =-2 */
		cr = TSDescDon_choc(nomsig, numchoc, 0, &ptDescDon);
		if (cr) {
			meser("TSDescDon_choc - ", nomsig, cr, FATAL);
		}
		if ((ptDescDon->type_donnee >> S_SIMPLE) & 1) { /* signal simple */
			type = sigType;
		} else if ((ptDescDon->type_donnee >> D_GHOMO) & 1) { /* groupe */
			type = homoType;
		} else {
			meser("Who are you ?", nomsig, 1002, FATAL);
		}

		if (ptDescDon->type_donnee & 1 << 1) {  /* donnee traitee */
			cooked_d = 1;
		}

		UDA_LOG(UDA_LOG_DEBUG, "%s %d\n", "Type of signal:", type);

		switch (type) {
		case homoType:
		case extgType:
			if (nout < 4) {
				printf("Il faut 4 arguments de sortie\n");
				UDA_LOG(UDA_LOG_DEBUG, "Returning from 2\n");
				return (cr);
			}
			break;
		default:
			if (nout < 3) {
				printf("Il faut 3 arguments de sortie\n");
				UDA_LOG(UDA_LOG_DEBUG, "Returning from 3\n");
				return (cr);
			}
			break;
		}

		j = num + 2;        /* on saute Y et T qui sont vides */
		if ((type == homoType) || (type == extgType)) {
			j++;
		}        /* certif = 4eme arg */

		ptd = cmisc;
#if (client == PC)
		Strcpy (ptd, " ");
		for (i = 1; i < 2 * LONG_UNITE; i++)
			strcat (ptd, " ");	/* Simule Unites X et Y */
#else
		for (i = 0; i < 2 * LONG_UNITE; i++)    /* Simule Unites X et Y */
			*ptd++ = ' ';
		*ptd = '\0';
#endif
		strcat (cmisc, date);
		strcat (cmisc, heure);
		/*****++++++++++++++++++++++++++++++++ MAJ Mars 2002 ****************/
		ptd = cmisc;
		while (*ptd)
			ptd++;

		if (indg > 0)
			sprintf(nomsig_misc, "%s%d", nomsig, indg);
		else
			strcpy(nomsig_misc, nomsig);

		for (i = 0; i < TAILLE_NOM_DONNEE + 4; i++) {
			if (i < strlen(nomsig_misc)) {
				*ptd++ = nomsig_misc[i];
			} else {
				*ptd++ = ' ';
			}
		}
		*ptd = '\0';
		if (cooked_d)
			strcat(cmisc, "T");
		else
			strcat(cmisc, "B");
		/*****++++++++++++++++++++++++++++++++ MAJ Mars 2002 ****************/
		nb_certif = 1;

		if (type == homoType) {
			nb_certif += nbcoord[1] * (nbcoord[2] > 0 ? nbcoord[2] : 1);
		}
#ifdef DEBUG
		printf ("Tsexist cr = %d misc num = %d \n", cr, j);
#endif
		/*pout[j] = mxCreatefloatMatrix (strlen (cmisc) + 1 + nb_certif + 1, 1, mxREAL);
            pr = mxGetPr (pout[j]);
            pr[0] = (float) nb_certif;
            for (i = 0; i < nb_certif; i++)
                pr[1 + i] = (float) certif[i];
            pr[1 + nb_certif] = (float) numv;
            for (i = 0; i < strlen (cmisc); i++)
                pr[2 + nb_certif + i] = cmisc[i];*/
		/* Fin  Unites , date , heure , certifs ...   */
	}
	UDA_LOG(UDA_LOG_DEBUG, "Signal name at line 393 : %s\n", nomsig);

	meser("TSExist - ", nomsig, ((cr != 0) ? cr : 9), NONFATAL);
	if (cr != 0) {
		UDA_LOG(UDA_LOG_DEBUG, "TSExist return false \n");
	} else {
		UDA_LOG(UDA_LOG_DEBUG, "no comprendo ! \n");
	}
	if (occur) {
		printf
		("En fait c'est l'occurrence %d du choc %d qui n'exite pas :-)\n",
				occur, numchoc);
		UDA_LOG(UDA_LOG_DEBUG, "Occurrence : %d\n", occur);

		UDA_LOG(UDA_LOG_DEBUG, "En fait c'est l'occurrence du choc qui n'exite pas\n");
	}
	free(ptEntree);
	UDA_LOG(UDA_LOG_DEBUG, "Returning from 4\n");
	return (((cr != 0) ? cr : 199));
}


/********* New for ARCAD ************/
cr = TSDescDon_choc(nomsig, (int)numchoc, (int)0, &ptDescDon);
/********* New for ARCAD ************/
if (cr) {
	meser("TSDescDon_choc - ", nomsig, cr, FATAL);
}

if ((ptDescDon->type_donnee >> D_TRAITEE) & 1) {
	cooked_d = 1;        /* donnee traitee */
}
if ((ptDescDon->type_donnee >> S_SIMPLE) & 1) {    /* signal simple */
	type = sigType;
} else if ((ptDescDon->type_donnee >> D_GHOMO) & 1) {    /* groupe homogene */
	type = homoType;
} else {
	meser("Who are you ?", nomsig, 1002, FATAL);
}

UDA_LOG(UDA_LOG_DEBUG, "%s %d\n", "Type of signal at line 433:", type);

if (rgrp) {
	type = extgType;
}        /* extraction d'un signal demandee */

if (!(ptDescDon->type_donnee >> SG_FTMPS & 1)) {                /* non fonction du temps */
	if (ptDescDon->nb_coordonnees == 1) {
		rscalaire = 1;
	} else if (ptDescDon->nb_coordonnees > 1) {
		rscalaire = 2;
	}

	/*      if (ptDescDon->ps_coord != NULL)
    {*/            /* format caractere ? */
#if (client == PC || client == MAC)
	if ((!Strncmp (ptDescDon->formaty, "c", 1)) ||
			(!Strncmp (ptDescDon->formaty, "C", 1)))
#else
		if (!strncasecmp(ptDescDon->formaty, "c", 1))
#endif
		{
			ptd = ptDescDon->formaty;
			ptd++;
			scomment = 1;
			fcomment = atoi(ptd);    /* taille format */
		}
	/* } */
}
/* YBYBYBYBYBYBYB  FreeMemM (ptDescDon); */

#ifdef DEBUG
printf ("type %d scomment %d rgrp %d rscalaire %d absolu %d\n",
		type, scomment, rgrp, rscalaire, absolute);
#endif
/* Read the data */

nbmax = nbcoord[0];
if (rang[0] == 0) {
	rang[0] = 1;
}        /** +++  **/
#if (client == PC)
if (nbcoord[0] > MAXDATAPC)
{
	if (rang[1] == 0)
		rang[1] = MAXDATAPC;	/** +++ **/
	printf ("==> Attention plus de 100000 mesures on ne peut  <==\n");
	printf ("==> lire que 100000 mesures maximum sur les PCs  <==\n");
	printf ("==> pour plus de renseignements Y.Buravand 23.42 <==\n");
}
else if ( extraction || rang[1] == 0 || np_specif == 0)
	rang[1] = nbcoord[0];	/** +++ **/
#else
if (extraction || rang[1] == 0 || np_specif == 0)
	rang[1] = nbcoord[0];    /** +++ **/
#endif

#ifdef DEBUG
printf ("extraction %d extr2 %d l_rang %d\n", extraction,nb_extr2,l_rang);
#endif
printf("%s \n", aff);
if (type == homoType) {
	int nbmax_sv;
	/*extract = 1;*/  /** ++++ **/
	if (nb_extr2 < 0) {
		nbmax = nbcoord[0] * nbcoord[1] * (nbcoord[2] > 0 ? nbcoord[2] : 1);
		nbmax_sv = nbmax;
	} else {
		nbmax = nb_extr2 * nbcoord[1] * (nbcoord[2] > 0 ? nbcoord[2] : 1);
		nbmax_sv = nb_extr2;
	}

	if (nbmax > 100000)
		printf("What a big signal !! be quiet please data are coming soon ...\n");
	if ((*X = (float*)calloc(nbmax, sizeof(float))) == NULL)
		meser("Alloc7 -", " ", 1001, FATAL);
	if ((*Y = (float*)calloc(nbmax, sizeof(float))) == NULL)
		meser("Alloc8 -", " ", 1001, FATAL);
	/** ++++   **/
	if (l_rang == 1) {
#ifdef DEBUG
		printf ("avant TSGrpRg rangs %d %d nbmax %d\n", rang[0], rang[1], nbmax);
		printf ("avant TSGrpRg nbcoord %d %d %d extract=%d \n", nbcoord[0], nbcoord[1], nbcoord[2],extract);
#endif
		UDA_LOG(UDA_LOG_DEBUG, "%s\n", "Calling TSGrpRg...");
		cr = TSGrpRg(nomsig, ptEntree, rang, nbmax_sv, extract, &ptUnite,
				&numv, certif, date, heure, nbmesgr, maxreel,
				(char*)*X, (char*)*Y, (char*)Coord);
	} else {
#ifdef DEBUG
		printf ("avant TSGrpX abs %g %g nbmax %d\n", xab[0], xab[1], nbmax);
#endif
		UDA_LOG(UDA_LOG_DEBUG, "%s\n", "Calling TSGrpX...");
		cr = TSGrpX(nomsig, ptEntree, xab[0], xab[1], nbmax_sv, extract, &ptUnite,
				&numv, certif, date, heure, nbmesgr, maxreel, (char*)*X,
				(char*)*Y, (char*)Coord);
	}

	if (cr != 0) {
		if (l_rang == 1)
			meser("TSGrpRg - ", nomsig, cr, NONFATAL);
		else
			meser("TSGrpX - ", nomsig, cr, NONFATAL);
		free(ptEntree);
		//free (X);
		//free (Y);
		UDA_LOG(UDA_LOG_DEBUG, "Returning from 5\n");
		return (cr);
	}
	lind[0] = nbmesgr[0];
	lind[1] = nbcoord[1] * (nbcoord[2] > 0 ? nbcoord[2] : 1);
	if (cooked_d == 0)
		lind[2] = lind[1];
	else
		lind[2] = 1;
#ifdef DEBUG
printf ("apres lecture homotype lind[0] %d, lind[1] %d, lind[2] %d\n",
		lind[0], lind[1], lind[2]);
#endif
} else if (type == sigType) {
	/*extract = 1; */   /*   ++++ **/
	/*     if (nb_extr2 < 0 && !scomment) */
	if (nb_extr2 < 0)
		nbmax = nbcoord[0];
	else
		nbmax = nb_extr2;

	if (scomment) {            /* signal commentaire  */
		nbmax = nbmax * fcomment;
		if ((Xc = (char*)calloc(nbmax, sizeof(char))) == NULL)
			meser("Alloc9 - ", " ", 1001, FATAL);
		if ((Yc = (char*)calloc(nbmax, sizeof(char))) == NULL)
			meser("Alloc10 - ", " ", 1001, FATAL);
#ifdef DEBUG
		printf ("avant TSRSIGRg rangs %d %d nbmax %d\n", rang[0], rang[1], nbmax);
#endif
		UDA_LOG(UDA_LOG_DEBUG, "%s\n", "Calling TSRSigRg...");
		cr = TSRSigRg(nomsig, ptEntree, rang, nbmax, extract, &ptUnite,
				&numv, certif, date, heure, &nbmes, maxreel, Xc, Yc);
#ifdef DEBUG
		printf ("apres TSRSigRg cr %d nbmes %d maxreel %d fcomment %d \n",
				cr,nbmes,*maxreel,fcomment);
		printf("Xc %s strlen(Xc) %zu\n",Xc,strlen(Xc));
#endif
		if (cr) {
			meser("TSRSigRg - ", nomsig, cr, NONFATAL);
			free(ptEntree);
			free(Xc);
			free(Yc);
			UDA_LOG(UDA_LOG_DEBUG, "Returning from 6\n");
			return (cr);
		}
		lind[0] = 1;
		lind[1] = nbmes * fcomment;
	} else
		/* signal "normal"  */
	{
		if (nbmax > 100000)
			printf("What a big signal !! be quiet please data are coming soon ...\n");
		if ((*X = (float*)calloc(nbmax, sizeof(float))) == NULL)
			meser("Alloc9 - ", " ", 1001, FATAL);
		if ((*Y = (float*)calloc(nbmax, sizeof(float))) == NULL)
			meser("Alloc10 - ", " ", 1001, FATAL);

		if (l_rang == 1) {
#ifdef DEBUG
			printf ("avant TSRSIGRg rangs %d %d nbmax %d\n", rang[0], rang[1], nbmax);
#endif
			UDA_LOG(UDA_LOG_DEBUG, "%s\n", "Calling TSRSigRg...");
			UDA_LOG(UDA_LOG_DEBUG, "%s %d\n", "rang[0]: ", rang[0]);
			UDA_LOG(UDA_LOG_DEBUG, "%s %d\n", "rang[1]: ", rang[1]);
			UDA_LOG(UDA_LOG_DEBUG, "%s %d\n", "nbmax: ", nbmax);
			UDA_LOG(UDA_LOG_DEBUG, "%s %d\n", "extract: ", extract);


			cr = TSRSigRg(nomsig, ptEntree, rang, nbmax, extract, &ptUnite,
					&numv, certif, date, heure, &nbmes, maxreel, (char*)*X,
					(char*)*Y);
#ifdef DEBUG
printf ("apres TSRSigRg cr %d nbmes %d maxreel %d\n",
		cr,nbmes,*maxreel);
#endif
		} else {
#ifdef DEBUG
			printf ("avant TSRSigX xab %g %g nbmax %d\n", xab[0], xab[1], nbmax);
#endif
			UDA_LOG(UDA_LOG_DEBUG, "%s\n", "Calling TSRSigX...");
			cr = TSRSigX(nomsig, ptEntree, xab[0], xab[1], nbmax, extract,
					&ptUnite, &numv, certif, date, heure, &nbmes, maxreel,
					(char*)*X, (char*)*Y);
		}
		if (cr) {
			if (l_rang == 1)
				meser("TSRSigRg - ", nomsig, cr, NONFATAL);
			else
				meser("TSRSigX - ", nomsig, cr, NONFATAL);
			free(ptEntree);
			//free (X);
			//free (Y);
			UDA_LOG(UDA_LOG_DEBUG, "Returning from 7\n");
			return (cr);
		}
		lind[0] = nbmes;
		lind[1] = 1L;
		lind[2] = lind[1];
	}
	switch (rscalaire) {
	case 0:
	case 2:
		break;
	case 1:
		if (scomment == 1)
			for (i = 0; i < nbmes * fcomment; i++)
				Yc[i] = Xc[i];
		else
			for (i = 0; i < nbmes; i++)
				Y[i] = X[i];
		break;
	default:
		break;
	}

} else if (type == extgType) {
	/* extract = 1; */    /**  ++++  **/
	if (nb_extr2 < 0)
		nbmax = nbcoord[0];
	else
		nbmax = nb_extr2;

	if ((*X = (float*)calloc(nbmax, sizeof(float))) == NULL)
		meser("Alloc9.1 - ", " ", 1001, FATAL);
	if ((*Y = (float*)calloc(nbmax, sizeof(float))) == NULL)
		meser("Alloc10.1 - ", " ", 1001, FATAL);
	if (l_rang == 1) {
#ifdef DEBUG
		printf ("aavnt TSRXtrRg ind0 %d ind1 %d \n", indices[0], indices[1]);
#endif
		UDA_LOG(UDA_LOG_DEBUG, "%s\n", "Calling TSXtrRg...");
		cr = TSXtrRg(nomsig, ptEntree, indices, rang, nbmax, extract, &ptUnite,
				&numv, certif, date, heure, &nbmes, maxreel, (char*)*X,
				(char*)*Y, (char*)Coord);
	} else {
#ifdef DEBUG
		printf ("avant TSRXtrX ind0 %d ind1 %d \n", indices[0], indices[1]);
#endif
		UDA_LOG(UDA_LOG_DEBUG, "%s\n", "Calling TSXtrX...");
		cr = TSXtrX(nomsig, ptEntree, indices, xab[0], xab[1], nbmax, extract,
				&ptUnite, &numv, certif, date, heure, &nbmes, maxreel,
				(char*)*X, (char*)*Y, (char*)Coord);
	}
	if (cr) {
		if (l_rang == 1)
			meser("TSXtrRg - ", nomsig, cr, NONFATAL);
		else
			meser("TSXtrX - ", nomsig, cr, NONFATAL);
		free(ptEntree);
		//free (X);
		//free (Y);
		UDA_LOG(UDA_LOG_DEBUG, "Returning from 8\n");
		return (cr);
	}
	lind[0] = nbmes;
	lind[1] = 1L;
	lind[2] = 1L;
}
if ((ptUnite) && (rscalaire != 1)) {
	//rscaltime = 1;
#if (client == PC)        /* scalaire = valeur temporelle ? */
	if ((!Strncmp (ptUnite->nom, "mcs", 3)) ||
			(!Strncmp (ptUnite->nom, "MCS", 3)))
		coeff_temps = 1.0E6;

	if ((!Strncmp (ptUnite->nom, "ms", 2)) ||
			(!Strncmp (ptUnite->nom, "MS", 2)))
		coeff_temps = 1.0E3;

	if ((!Strncmp (ptUnite->nom, "s", 1)) ||
			(!Strncmp (ptUnite->nom, "S", 1)))
		coeff_temps = 1.0;

#else
	/*if (!strncasecmp(ptUnite->nom, "mcs", 3))
            coeff_temps = 1.0E6;
        if (!strncasecmp(ptUnite->nom, "ms", 2))
            coeff_temps = 1.0E3;
        if (!strncasecmp(ptUnite->nom, "s", 1))
            coeff_temps = 1.0;*/
#endif
}
/* les valeurs  */
if (scomment) {
	/*if (nouttemp > 0)
        {
            pout[num] = charMat (lind[0], lind[1] , Yc);
            num += 1;
            nouttemp--;
        }*/
	free(Yc);
} else {
	if (nouttemp > 0) {
#ifdef DEBUG
		printf ("Trying to create val %d %d matrix \n", lind[0], lind[1]);
		printf ("rscaltime %d\n", rscaltime);
#endif
		/*pout[num] = mxCreatefloatMatrix (lind[0], lind[1], mxREAL);
            pr = mxGetPr (pout[num]);
            for (j = 0; j < lind[1]; j++)
            {
                if (rscaltime && rscalaire)
                {
                    for (i = 0; i < lind[0]; i++)
                        pr[i + j * lind[0]] = Y[i * lind[1] + j] / coeff_temps;
                }
                else
                {
                    for (i = 0; i < lind[0]; i++)
                        pr[i + j * lind[0]] = Y[i * lind[1] + j];
                }
            }*/

#ifdef DEBUG
		printf ("fin Trying to create val %d %d matrix \n", lind[0], lind[1]);
#endif
		num += 1;
		nouttemp--;
	}
	//free (Y);
}
/* les temps  */
if (scomment) {
	if (nouttemp > 0) {
		//pout[num] = emptyMat ();
		num += 1;
		nouttemp--;
	}
	//free (Xc);
} else {
#ifdef DEBUG
	printf ("Trying to create temps %d %d matrix\n", lind[0], lind[2]);
#endif
	/*		if (nouttemp > 0)
        {
            pout[num] = mxCreatefloatMatrix (lind[0], lind[2], mxREAL);
            pr = mxGetPr (pout[num]);
            if (!absolute) {
                if ( (it0 = tsignitron (&numchoc)) < 0) {
                    printf ("Ignitron non disponible => temps en absolu");
                    printf ("  Ignitron es beleu escondu\n");
                    it0 = 0;
                }
            }

            for (j = 0; j < lind[2]; j++)
            {
                if (absolute)
                {
                    if (rscalaire == 0)
                    {
                        for (i = 0; i < lind[0]; i++)
                            pr[i + j * lind[0]] = X[i * lind[1] + j] / CTPS;
                    }
                    else

	 * signal scalaire : rendu tel quel si valeur != temps

                    {
                        if (rscaltime)
                        {
                            for (i = 0; i < lind[0]; i++)
                                pr[i + j * lind[0]] = X[i * lind[1] + j] / coeff_temps;
                        }
                        else
                        {
                            for (i = 0; i < lind[0]; i++)
                                pr[i + j * lind[0]] = X[i * lind[1] + j];
                        }
                    }
                }
                else
                {
                        it0 = tsignitron(&numchoc);
                    if (rscalaire == 0)
                    {
                        for (i = 0; i < lind[0]; i++)
                            pr[i + j * lind[0]] = (X[i * lind[1] + j] - (float) it0) / CTPS;
                    }
                    else
                    {
                        if (rscaltime)
                        {
                            for (i = 0; i < lind[0]; i++)
                                pr[i + j * lind[0]] = X[i * lind[1] + j] / coeff_temps;
                        }
                        else
                        {
                            for (i = 0; i < lind[0]; i++)
                                pr[i + j * lind[0]] = X[i * lind[1] + j];
                        }
                    }
                }
            }
            num += 1;
            nouttemp--;
        }
        //free (X);*/
}
#ifdef DEBUG
printf ("Fin Trying to create temps %d %d matrix\n", lind[0], lind[2]);
#endif

/* les coordonnees si groupe  */
if (type == homoType) {
	if (nouttemp > 0) {
#ifdef DEBUG
		printf (" Trying to create Coord %d %d matrix\n", nbcoord[1], nbcoord[2]);
#endif
		/*pout[num] = mxCreatefloatMatrix (1, (nbcoord[1]  + *
                    (nbcoord[2] > 0 ? nbcoord[2] : 1)), mxREAL);
            pr = mxGetPr (pout[num]);
            for (j = 0; j < lind[1]; j++)
            {
                for (i = 0; i < 1; i++)
                    pr[i + j * 1] = Coord[i * (nbcoord[1] +
                            (nbcoord[2] > 0 ? nbcoord[2] : 0)) + j];
            }
               free((char *) Coord);
            num += 1;
            nouttemp--;*/
#ifdef DEBUG
		printf ("Fin Trying to create Coord %d %d matrix\n", nbcoord[1], nbcoord[2]);
#endif
	}

}
if ((type == extgType) && (!pansement)) {
	if (nouttemp > 0) {
		/*pout[num] = mxCreatefloatMatrix (1, 1 + (indices[1] != 0 ? 1 : 0), mxREAL);
            pr = mxGetPr (pout[num]);
            pr[0] = Coord[(int) 0];
            if (indices[1] != 0)
                pr[1] = Coord[1];*/
		/*    free((char *) Coord); */
		num += 1;
		nouttemp--;
	}
}
/* Unites , date , heure , certifs ...   */
if (nouttemp > 0) {
#ifdef DEBUG
	printf ("Trying create misc  num %d nouttemp = %d\n", num, nouttemp);
#endif
	ptd = cmisc;
	for (i = 0; i < LONG_UNITE; i++) {
		if (ptUnite) {
			if (i < strlen(ptUnite->nom))
				*ptd++ = ptUnite->nom[i];
			else
				*ptd++ = ' ';
		} else
			*ptd++ = ' ';
	}
	ptUnite = ptUnite->ps_suivant;

	if (ptUnite) {
		while (ptUnite != NULL) {
			for (i = 0; i < LONG_UNITE; i++) {
				if (i < strlen(ptUnite->nom))
					*ptd++ = ptUnite->nom[(int)i];
				else
					*ptd++ = ' ';
			}
			ptUnite = ptUnite->ps_suivant;
		}
	} else {
		for (i = 0; i < LONG_UNITE; i++)
			*ptd++ = ' ';
	}
	*ptd = '\0';

	strcat (cmisc, date);
	strcat (cmisc, heure);
	/****++++++++++++++++++++++++++++++++ MAJ Mars 2002 ****************/
	ptd = cmisc;
	while (*ptd)
		ptd++;

	if (indg > 0)
		sprintf(nomsig_misc, "%s%d", nomsig, indg);
	else
		strcpy(nomsig_misc, nomsig);

	for (i = 0; i < TAILLE_NOM_DONNEE + 4; i++) {
		if (i < strlen(nomsig_misc))
			*ptd++ = nomsig_misc[i];
		else
			*ptd++ = ' ';
	}
	*ptd = '\0';
	if (cooked_d)
		strcat(cmisc, "T");
	else
		strcat(cmisc, "B");
	/****++++++++++++++++++++++++++++++++ MAJ Mars 2002 ****************/
	nb_certif = 1;
	if (type == homoType)
		nb_certif = nbcoord[1] * ((nbcoord[2] > 0) ? nbcoord[2] : 1);
#ifdef DEBUG
	printf ("Fin ptunite misc %d nb_certif\n", nb_certif);
#endif

	/*pout[num] = mxCreatefloatMatrix (strlen (cmisc) + 1 + nb_certif + 1, 1, mxREAL);
        pr = mxGetPr (pout[num]);
        pr[0] = (float) nb_certif;
        for (i = 0; i < nb_certif; i++)
            pr[1 + i] = (float) certif[i];
        pr[1 + nb_certif] = (float) numv;
        for (i = 0; i < strlen (cmisc); i++)
            pr[2 + nb_certif + i] = cmisc[(int) i];*/

	num += 1;
	nouttemp--;
#ifdef DEBUG
	printf ("Fin misc nouttemp %d\n", nouttemp);
#endif
}
free(ptEntree);
UDA_LOG(UDA_LOG_DEBUG, "Returning from 9\n");
return (cr);

}


void
meser(char* mes1, char* mes2, int numer, int code)
{
	printf("tsbase : %s%s :", mes1, mes2);

	numer--;

	if ((numer >= 0) && (numer < 1000)) {
		if (numer > 98) {
			numer = numer - 49;
		}
		if (numer > 65) {
			numer = numer - 85;
		}
		printf(" %s\n", err_tslib[numer]);
	} else {
		if ((numer > ERR_MAX) || (numer < 0)) {
			printf(" Erreur TSLIB (bug?) %d\n", (int)numer);
			sigaction(SIGINT, &oldact, NULL);     /* reset previous action */
			printf("Bad news : Contact your System Manager ");
		}
		printf(" %s\n", err_tsmat[(int)(numer - 1000)]);
	}
	if (code == FATAL) {
		sigaction(SIGINT, &oldact, NULL);     /* reset previous action */
		printf("Oops Pecaire ... a ben-leu");
	}
}

void getSignalType(char* nomsig, int numchoc, int* signalType)
{
	ts_donnee* ptDescDon;
	cr = TSDescDon_choc(nomsig, numchoc, 0, &ptDescDon);
	if (cr) {
		meser("TSDescDon_choc - ", nomsig, cr, FATAL);
	}
	if ((ptDescDon->type_donnee >> S_SIMPLE) & 1) {/* signal simple */
		UDA_LOG(UDA_LOG_DEBUG, "--> single signal\n");
		*signalType = 1;
	} else if ((ptDescDon->type_donnee >> D_GHOMO) & 1) { /* groupe */
		UDA_LOG(UDA_LOG_DEBUG, "--> group of  signals\n");
		*signalType = 2;
	} else {
		UDA_LOG(UDA_LOG_DEBUG, "--> Who are you ?\n");
		meser("Who are you ?", nomsig, 1002, FATAL);
	}
}

void getExtractionsCount(char* nomsigp, int numchoc, int occ, int* extractionCount)
{

	int certif[MAX_CERTIF];
	int indices[2];
	int nbcoord[MAX_COORD];
	pS_Entree ptEntree;
	int occur = 0, indg = 0;
	char nomsig[TAILLE_NOM_DONNEE];
	char nom_gen[TAILLE_NOM_DONNEE];
	char date[TAILLE_DATE], heure[TAILLE_HEURE];
	int rgrp = 0;
	int numv;

	ptEntree = (pS_Entree)calloc(1, sizeof(struct S_Entree));
	ptEntree->union_var.lval = numchoc;

	/*****++++++++++++++++++++++++++++++++ MAJ Mars 2002 ****************/
	if (nomsigp[0] == '!') {
		strcpy(nomsigp, &nomsigp[1]);
	}
	/*****++++++++++++++++++++++++++++++++ MAJ Mars 2002 ****************/

	tsparse(nomsigp, nomsig, indices, &rgrp, &occur);

	if (occ) {
		occur = occ;
	}

	if (occur) {
		ptEntree->ps_suivant = (pS_Entree)calloc(1, sizeof(struct S_Entree));
		ptEntree->ps_suivant->union_var.lval = occur;
		ptEntree->ps_suivant->ps_suivant = NULL;
	} else {
		ptEntree->ps_suivant = NULL;
	}

	int i;
	for (i = 0; i < MAX_CERTIF; i++)
		certif[i] = 0;

	for (i = 0; i < MAX_COORD; i++)
		nbcoord[i] = 0;
	cr = TSExist_gen(nomsig, ptEntree, nbcoord, &numv, certif, date, heure,
			nom_gen, &indg);

	*extractionCount = nbcoord[1];

}


