/*		.....	tsbamat.h  .....		*/
/*		.....	  YB	   .....		*/
/*							*/
#define sigType 	1
#define homoType 	2
#define extgType 	3

#define CTPS	(double) 1e6	/* pour avoir des secondes */
#define CTPSN	(double) 1e3	/* pour avoir des secondes */
#define SEP1		'!'	/* separateur indice 1	   */
#define SEP2		'.'	/* separateur indice 2	   */
#define SEP3		'#'	/* separateur occurrence   */

#define NONFATAL	0	/* erreur non fatale	*/
#define FATAL		1	/* erreur fatale	*/

#define RAW		1L	/* pas de calibration, pretraitement   */
#define COOKED		0L	/* calibrees, pretraitees..            */
#define MAX_OPTIONS	1	/* Nbre max d'options en dernier arg.  */
#define MAX_COORD	4	/* Nbre max de coordonnees d'un groupe */
#define MAX_CERTIF	128	/* Nbre max de certifications          */
#define MAX_SIG  	128	/* Nbre max de signaux d'un groupe     */
#define MAX_DIAG	50	/* Nbre max de diagnostics	       */
#define MAX_TRAIT 	80	/* Nbre max de traitements decrits     */
#define MAX_PARAM       16384	/* Nbre max de parametres demandes     */
#define TAILLE_COMMENT	80	/* Taille des commentaires rendus      */
#define TAILLE_MISC	256	/* Taille des certifs,date,heure ..    */
#define LONG_UNITE	8

	/*decalage a appliquer sur le type de la donnee du bit 0 a 10*/

#define D_BRUTE		0	/* donnee brute			*/
#define D_TRAITEE	1	/* donnee traitee		*/
#define S_SIMPLE	2	/* signal simple		*/
#define D_AGHHETERO	3	/* donnee d'un groupe hetero	*/
#define D_GHOMO		4	/* groupe homogene		*/
#define D_GHETERO	5	/* groupe heterogene		*/
#define SG_FTMPS	6	/* signal fonction du temps	*/
#define D_REDUITE	7	/* donnee reduite		*/
#define D_PRETRAITE	8	/* donnee pretraitee		*/
#define D_CALCULEE	9	/* donnee calculee		*/
#define D_RTEMPO	10	/* donnee reduite temporelle	*/

#define NUM_CHOC_BASE 20101
#define NUM_CHOC_TEST 200
#define CHOC_NEW_CHRONO 26822  /* 1er choc avec nouvelle chrono */      
