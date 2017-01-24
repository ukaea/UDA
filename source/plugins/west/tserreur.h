/*		.... erreur.h .....		*/
/*		....    YB    .....		*/
/*						*/

/*  1		NomTrait 	: nom d'un traitement
  2 		Nomdiag 	: nom d'un diagnostic
  3		NomDon 	: nom d'une donn�e
  4		NomDec 	: nom d'une d�clenche
  5		Date 
  6		Heure
  7		Entrees 	: liste cha�n�e des entr�es
  8		DescTrait 	: structure de description d'un traitement
  9		DescDiag 	: structure de description d'un diagnostic
 10		DescDon 	: structure de description d'une donn�e
 11 		Unite 		: liste cha�n�e des unit�s
 12 		NbCoord 	: tableau des longueurs des coordonn�es
 13		TabVal 	: tableau des mesures
 14		NbCoord 	: tableau de valeurs des coordonn�es
 15		Rangs 	: tableau contenant une fourchette de rangs
 16		Incides 	: tableau d'indices d'un signal dans un groupe
 17		X 		: tableau des abscisses
 18		T 		: tableau des temps
 19		Y 		: tableau des mesures
 20		Coord 	: tableau des coordonn�es
 21		Certif 		: tableau des certifications
 22		NbMax 	: nombre de mesures demand�
 23		Extract 	: type d'extraction demand�
 24		Numchoc 	: num�ro de choc
 25		Occur 	: occurrence d'une d�clenche
 26		XI 		: valeur initiale d'abscisse 		
 27		XF 		: valeur finale d'abscisse 
 28		LongNb 	: taille en octets du tableau Longueurs
 29		CertifNb 	: taille en octets du tableau Certif	
 30		ValNb 	: taille en octets du tableau TabVal
 31		NumVer 	: num�ro de version
 32		NbMes 	: nombre de mesures rendues
 33		MaxReel 	: nombre de mesures trouv�es
 34		CR 		: compte-rendu d'ex�cution
 35		CodeClient 	: code associ� au client
 36		CodeFonc 	: code associ� � la fonction

*/

/* messages erreur tslib [0..38]	*/
static char *err_tslib[]={
	"Traitement inconnu",
	"Diagnostic inconnu",
	"Donnee inconnue",
	"Traitement existant mais version inexistante",
	"Diagnostic existant mais version inexistante",
	"Numero de choc incorrect",
	"Traitement absent pour ce choc",
	"Diagnostic absent pour ce choc",
	"Donnee non produite pour ce choc",
	"Nom de donnee incorrect",
	"Fourchette de rang incorrecte",
	"Fonction indisponible pour ce type de donnees (calculee)",
	"Type d'extraction incorrect",
	"Nombre de mesures demande incorrect",
	"Lecture incoherente pour ce type de donnees",
	"Fonction non autorisees pour un groupe heterogene",
	"Abscisses incorrectes",
	"Valeurs des indices incorrectes",
	"Nom de declenche ou occurence non trouve",
	"Valeurs des certifications inferieure ou egale -2",
	"Modifications des certifications en negatif",
	"Valeurs des certifications incorrectes",
	"Nombre de certifications incorrect (Groupe Homogene)",
	"Ecriture en cours pour ce traitement et ce choc",
	"Ecriture en cours delai attente trop long",
	"Lecture en cours delai attente trop long",
	"Disque sature",
	"Probleme a l'ouverture du fichier",
	"Taille des tableaux erronnee",
	"Erreur BD rencontree",
	"Incoherence BD",
	"Donnee non acquise",
	"Erreur lors de la lecture des donnees brutes",
	"Structure de fichier incorrecte en BD",
	"Valeurs de x decroissantes",
	"Pas de mesures dans l'intervalle demande",
	"Puissance de 10 incorrecte",
	"Probleme de verrou",
        "Incoherence dans la definition du type de la donnee",
	" ",
	" ",
	" ",
	" ",
	" ",
	" ",
	" ",
	" ",
	" ",
	" ",
	" ",
"Machine hote absente du fichier /etc/hosts",
"Service absent du fichier /etc/services",
"Creation de socket impossible",
"Probleme de connexion avec le serveur",
"Probleme d'attribution de port de communication",
"Probleme d'emission",
"Probleme de fin d'emission",
"Probleme de reception",
"Chaine non initialisee",
"Tableau non initialise",
"Probleme avec l'outil de connexion TCPack",
"Probleme a la creation de la structure de donnees relative a la connexion",
"Probleme de configuration de la connexion",
"",
"",
"Unexepected error",
"Allocation memoire impossible",
"Probleme a l'ouverture du fichier",
"Repertoire non trouve",
"Producteur non trouve",
"Diagnostic non trouve",
"Traitement non trouve",
"Objet non trouve",
"Parametre non trouve",
"Pas d'objets ....",
"Mauvais numero de choc",
"Mauvais type d'objet",
"Impossible d'ouvrir la BD",
"Unrecoverable error",
"Erreur SQL",
"Mauvais nom de producteur",
"Mauvais nom d'objet",
"Mauvais nom de parametre",
"Mauvais nombre de valeurs",
"Mauvaise longueur",
"Erreur donnee ",
"Mauvais format de parametre",
"numero_choc_invalide",
"nom_declenche_invalide",
"nom_diagnostic_invalide",
"nom_objet_invalide",
"nom_parametre_invalide",
" ",
" ",
};
/* messages erreur tsmat [1001..1014]	*/
static char *err_tsmat[]={
"Allocation memoire impossible",
"La donnee demandee n'est ni un signal simple ni un groupe ",
"Format inconnu ou non supporte actuellement",
" ",
" ",
" ",
" ",
" ",
" ",
"Numero de choc invalide ( <0 )",
"Nom de declenche invalide",
"Nom de diagnostic invalide",
"Nnom d'objet invalide",
"Nom de parametre invalide",
};

/*static char *err_rqlib[]={
"Ouverture fichier choc impossible (absent ?) ",
"Diagnostic inconnu ou absent ",
"Objet inconnu", 
"Parametre inconnu",
"Nom ou occurrence declenche inconnu",
"Acces au diagnostic pour la declenche demandee impossible",
};*/
#define ERR_MAX	1014

/*static char *err_RDC[]={
        "  ",
        "Format heure debut incorrect (-1)",
        "Format heure fin incorrect (-2)",
        "Format heure debut incorrecte (-3)",
        "Format heure fin incorrecte (-4)",
        "Pb GregorianDayDiff (-5)",
        "Heure fin < heure debut (-6)",
        "Donnee inconnue (-7)",
        "Pb type producteur (-8)",
        "Pb description donne (producteur = diagnostic) (-9)",
        "Donnee non continue !!! (-10)",
        "Allocation memoire calibration impossible (-11)",
        "Allocation memoire offset impossible (-12)",
        "Allocation memoire calibration impossible (1.0) (-13)",
        "Allocation memoire offset impossible (0.0) (-14)",
        "Impossible d'ouvrir le fichier en memoire (-15)",
        "Impossible de mapper le cache en memoire (-16)",
        "Pas de donnees pour ce signal (-17)",
        "Impossible de mapper les donnees en memoire (-18)",
        "Pas de donnees dans l'intervalle de temps demande (-19)",
        "Pas d'occurrence jour (-20)",
        "Allocation memoire valeurs impossible (-21)",
        "Allocation memoire temps impossible (-22)",
        "Erreur get_array (-23)",
        "Pb description donne (producteur = diagnostic) (-24)",
        "Erreur get_array_cc (-25)",
        "Producteur n'est ni un diagnostic ni un traitement (-26)",
        "Erreur SQL ....(-27)",
        "Pas de choc en cours. Utilisez les fonctions normales !!! (-28)",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "Nombre de signaux incorrect",
        "Pas de trame de rang zero",
        "Longueur de la trame de rang 0 incorrecte",
        "Format des donnees incorrect",
};*/


