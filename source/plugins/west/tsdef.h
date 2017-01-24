/*****************************************************************************

   NOM  DU MODULE        :  tsdef.h

   DESCRIPTION DU MODULE :  Fichier Include contenant toutes les constantes 
                                utilises par le client TSLib       

   Auteur       :       Vita-Maria GUZZI
   Societe      :       CR2A
   Projet       :       TSLib

   Date de Creation :   15/09/92
   Modifications    :

 *****************************************************************************/

#define	TAILLE_NOM_DONNEE	(int)17	/* taille maxi avec '\0' */
#define	TAILLE_NOM_COORDONNEE	(int)17
#define	TAILLE_NOM_DIAGNOSTIC	(int)17
#define	TAILLE_NOM_TRAITEMENT	(int)17
#define	TAILLE_NOM_PRODUCTEUR	(int)17
#define	TAILLE_NOM_UNITE	(int)17
#define	TAILLE_NOM_ENTREE	(int)17
#define	TAILLE_NOM_DECLENCHE	(int)17
#define	TAILLE_NOM_AUTEUR	(int)17
#define	TAILLE_PRETRAITEMENT	(int)17
#define	TAILLE_COMMENTAIRE	(int)1991
#define	TAILLE_NOM_AUTEURS	(int)65
#define	TAILLE_NOM_MACHINE	(int)65
#define	TAILLE_NOM_LOCALISATION	(int)65
#define	TAILLE_NOM_REPERTOIRE	(int)65	/* JS was here on Feb 04 2000 */
#define	TAILLE_FORMAT		(int)6
#define	TAILLE_FORMAT_SERVEUR	(int)2
#define	TAILLE_DATE		11
#define	TAILLE_HEURE		9
#define TAILLE_VALEUR_ENTREE    (int)8
#define MAX_INDICE		(int)2
#define MAX_RANG                (int)2
#define MAX_MESURE              (int)3
#define MAX_CODE_PARAM          (int)100


#define SUITE                   (int)1       /* indiquent la suite ou la */
#define FIN                     (int)0        /* fin d'une liste chainee */


#define DEC                     '1'             /* types de machines clientes */
#define PC                      '2'
#define MAC                     '3'
#define SUN                     '4'
#define INTEL                   '5'

#define entier_court            's'           /* types de donnees         */
#define entier_long             'l' 
#define non_signe		'u' 
#define reel_simple             'f'
#define reel_double             'd'
#define caractere               'c'

#define UNIQ			0      /* indiquent si les coordonnees*/
#define MULT			1      /* sont uniques ou multiples*/

#define MAX_FUNCT_NUMBER	1000

#define NUM_CHOC_BASE		20101  /* choc charniere */	
#define NUM_CHOC_TEST		200    /* choc charniere test*/	
#define NUM_CHOC_DEC		20800  /* choc charniere serveur DEC*/	
