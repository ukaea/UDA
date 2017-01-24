
#ifndef ULONG
#ifdef __alpha
#define ULONG unsigned int
#else
#define ULONG unsigned long
#endif
#endif

#define VRAI                1
#define FAUX                0

/* Ne pas supprimer la ligne qui suit, elle sert a generer DicoTop_ESQLC.h*/
/* Description of table carte_camac_2162 from database basetop */

#define L_NOM_TABLE		26
#define L_NOM_PARAM		13
#define L_NOM_ATTRIB		21
#define L_NOM_VUE		21
#define L_NOM_TABLEAU		21
#define L_LIBELLE_VUE		21
#define L_NOM_CARTE		13
#define L_NOM_OBJET		13
#define L_NOM_PRODUCTEUR	13
#define L_NOM_ENSEMBLE		13
#define L_NOM_DECLENCHE		13
#define L_NOM_STRATEGIE		13
#define L_NOM_DONNEE		13
#define L_NOM_EVENEMENT		13
#define L_UNITE			13
#define L_TYPE_DIAGNOSTIC	7
#define L_NOM_AUTEUR		65
#define L_FORMAT		5
#define L_REPERTOIRE		65	/* JS was here on Feb 04 2000 */
#define L_NOM_UTIL_UNIX		17
#define L_NOM_UTIL		33
#define L_COMMENTAIRE_OBJET	301
#define L_COMMENTAIRE_PARAM	241
#define L_COMMENTAIRE		33
#define	L_CTYPE			6
#define L_NOM_COLONNE		13
#define L_NOM_REF		9
#define L_NOM_REPERE		9
#define L_TITRE                 33  
#define L_TEXTE                 201
#define L_COULEUR_SIGNAL        13

 typedef struct zs_carte_camac_2162_ {
	int	id_objet;
	short	numero;
	int	adresse;
	short	chassis;
	short	nutiroir;
	short	modef;
	float	tpsun;
	float	tpsdeux;
	float	tpsint;
	int	perhorl;
	short	nbcycle;
	short	nbspec;
	short	nbcanaux;
  } ts_carte_camac_2162;

/* Description of table carte_camac_6103 from database basetop */
 typedef struct zs_carte_camac_6103_ {
	int	id_objet;
	short	numero;
	int	adresse;
	short	chassis;
	short	nutiroir;
  } ts_carte_camac_6103;

/* Description of table carte_camac_6810 from database basetop */
 typedef struct zs_carte_camac_6810_ {
	int	id_objet;
	short	numero;
	int	adresse;
	short	chassis;
	short	nutiroir;
	char	seqnum[L_NOM_OBJET];
	short	funf17a14;
	short	ncaf17a0;
	short	horf17a13;
	short	fdef17a15;
	short	tsef17a10;
	short	ns1f17a11;
	short	ns2f17a12;
	short	pt1f16a14;
	short	pt2f16a15;
	short	rtef16a0;
	short	tgcf16a10;
	short	tgpf16a9;
	short	tgsf16a13;
	short	tghf16a11;
	short	tgbf16a12;
	short	tgdf17a9;
	short	tgsoft;
	short	datecam;
	int	tgoffset;
	short	preoffset;
  } ts_carte_camac_6810;

/* Description of table carte_camac_8818 from database basetop */
 typedef struct zs_carte_camac_8818_ {
	int	id_objet;
	short	numero;
	int	adresse;
	short	chassis;
	short	nutiroir;
	short	freq;
	short	offset;
	short	taillemem;
	short	pretrig;
	char	nomamp[L_NOM_OBJET];
	short	nbmaxtg;
	short	nbtgpaq;
  } ts_carte_camac_8818;

/* Description of table carte_chrono from database basetop */
 typedef struct zs_carte_chrono_ {
	int	id_objet;
	short	numero;
	int	adresse;
	short	numicm100;
	short	generit;
  } ts_carte_chrono;

/* Description of table carte_date1 from database basetop */
 typedef struct zs_carte_date1_ {
	int	id_objet;
	short	numero;
	int	adresse;
	short	generit;
	short	numicm100;
  } ts_carte_date1;

/* Description of table carte_date2 from database basetop */
 typedef struct zs_carte_date2_ {
	int	id_objet;
	short	numero;
	short	numicm100;
  } ts_carte_date2;

/* Description of table carte_icm100 from database basetop */
 typedef struct zs_carte_icm100_ {
	int	id_objet;
	short	numero;
	short	type;
	short	stype;
	int	adresse;
	short	typedate;
	char	nomdate[L_NOM_CARTE];
	short	adrstor;
	short	adretor;
	short	mode;
	short	demar;
	short	datation;
	short	horloge;
	short	ram_max;
	short	burst;
	short	nbvoie;
	short	repsd;
	short	nbmotifs;
	short	nvcont;
	short	expostps;
	int	cadence;
	int	cadsd;
	short	npretrig;
  } ts_carte_icm100;

/* Description of table carte_icm101 from database basetop */
 typedef struct zs_carte_icm101_ {
	int	id_objet;
	short	numero;
	short	stype;
	int	adresse;
	short	typedate;
	char	nomdate[L_NOM_CARTE];
	short	adressio;
	short	adrstor;
	short	adretor;
	short	mode;
	short	demar;
	short	datation;
	short	horloge;
	short	ram_max;
	short	burst;
	short	nbvoie;
	short	softtrig;
	short	repsd;
	short	nbmotifs;
	short	nvcont;
	short	expostps;
	int	cadence;
	int	cadsd;
	int	cadrap;
	short	countrap;
	short	countpre;
  } ts_carte_icm101;

/* Description of table choc_reduit from database basetop */
 typedef struct zs_choc_reduit_ {
	int	num_choc;
	int	date;
	int	heure;
  } ts_choc_reduit;

/* Description of table chrono from database basetop */
 typedef struct zs_chrono_ {
	int	id_objet;
	char	stratpilf[L_NOM_STRATEGIE];
  } ts_chrono;

/* Description of table comment_param from database basetop */
 typedef struct zs_comment_param_ {
	int	id_param;
	char	commentaire[L_COMMENTAIRE_PARAM];
  } ts_comment_param;

/* Description of table commentaire from database basetop */
 typedef struct zs_commentaire_ {
	int	id_objet;
	char	commentaire[L_COMMENTAIRE_OBJET];
  } ts_commentaire;

/* Description of table common from database basetop */
 typedef struct zs_common_ {
	int	id_objet;
	short	numero;
  } ts_common;

/* Description of table consigne from database basetop */
 typedef struct zs_consigne_ {
	int	id_objet;
	short	bloc;
	char	unitey[9];
	char	repet[2];
	char	declenche[L_NOM_DECLENCHE];
	int	pasmini;
	char	reelle[L_NOM_OBJET];
	short	indice;
	float	tmax;
  } ts_consigne;

/* Description of table asservissement from database basetop */
 typedef struct zs_asservissement_ {
	int	id_objet;
        char    diagnostic[L_NOM_PRODUCTEUR];
        int creation;
        char    auteurs[L_NOM_AUTEUR];
  } ts_asservissement;

/* Description of table decodeur_chrono from database basetop */
 typedef struct zs_decodeur_chrono_ {
        int     id_objet;
  } ts_decodeur_chrono;

/* Description of table encodeur_chrono from database basetop */
 typedef struct zs_encodeur_chrono_ {
        int     id_objet;
  } ts_encodeur_chrono;

/* Description of table pilotage from database basetop */
 typedef struct zs_pilotage_ {
        int     id_objet;
  } ts_pilotage;

/* Description of table v_listdiag from database basetop */
 typedef struct zs_v_listdiag_ {
        int     id_param;
        short   rang;
        char    nomdiag[L_NOM_OBJET];
  } ts_v_listdiag;


/* Description of table diagnostic from database basetop */
 typedef struct zs_diagnostic_ {
	int	id_objet;
        short   numero;
	short	version;
	int	id_diagnostic;
	short	asservissement;
	char	type_diagnostic[L_TYPE_DIAGNOSTIC];
	short	type;
	int	creation;
	short	nb_donnees;
	char	auteurs[L_NOM_AUTEUR];
	char	stratbase[L_NOM_STRATEGIE];
	short	enregistrable;
	short	changement_version; 
  } ts_diagnostic;

/* Description of table donnee from database basetop */
 typedef struct zs_donnee_ {
	int	id_objet;
        short   numero;
	int	id_donnee;
	short	confidentialite;
	int	type_donnee;
	short	nb_composantes;
	short	nb_coordonnees;
	char	formatx[L_FORMAT];
	char	formaty[L_FORMAT];
	char	formati[L_FORMAT];
	char	formatj[L_FORMAT];
	char	unitex[L_UNITE];
	char	unitey[L_UNITE];
	char	unitei[L_UNITE];
	char	unitej[L_UNITE];
	short	structx;
	short	structy;
	short	structi;
	short	structj;
	short	longueuri;
	short	longueurj;
	short	maxtrame;
	short	remdif;
  } ts_donnee;

/* Description of table fonction from database basetop */
 typedef struct zs_fonction_ {
	int	id_param;
        short   rang;
	char	type[3];
	float	amplitude;
	float	periode;
	float	ti;
	float	tf;
	float	yi;
	float	yf;
	short	offset;
  } ts_fonction;

/* Description of table generateur_de_fonctions from database basetop */
 typedef struct zs_generateur_de_fonctions_ {
	int	id_objet;
	short	type;
	int	horloge;
	char	unitet[9];
	char	unitey[9];
	float	caliby;
	float	offsety;
	float	maxy;
	float	miny;
  } ts_generateur_de_fonctions;

/* Description of table groupe_heterogene from database basetop */
 typedef struct zs_groupe_heterogene_ {
	int	id_groupe;
	int	id_donnee;
	short	rang_donnee;
  } ts_groupe_heterogene;

/* Description of table id_objet from database basetop */
 typedef struct zs_ident {
	int	id_objet;
	int	id_param;
	int	id_vue;
	int	id_to_vue;
	int	id_tableau;
  } ts_ident;

  
/* Description of table donnee_generique from database arcad */
  typedef struct zs_donnee_generique_ {
        int     id_objet;
  } ts_donnee_generique;

/* Description of table m_liens_to from database basetop */
 typedef struct zs_m_liens_to_ {
	short	type_objet;
	char	nom_param[L_NOM_ATTRIB];
	short	type_objet_lie;
	char	nom_param_lie[L_NOM_ATTRIB];
  } ts_m_liens_to;

/* Description of table m_param from database basetop */
 typedef struct zs_m_param_ {
	short	type_objet;
	char	nom_param[L_NOM_ATTRIB];
	short	categ_param;
	short	type_param;
	short	automatique;
	short	increment;
	short	liste;
	short	pointeur;
	char	table_pointee[L_NOM_TABLE];
	char	attribut_pointe[L_NOM_ATTRIB];
  } ts_m_param;

typedef struct zs_cwm_valeur_ {
	short	type_param;
	char	nom[L_NOM_ATTRIB];
	short	type_val;
	int	taille;
	float	valmin;
	float	valmax;
	short	affichage;
	short 	interne;
	short	automatique;
	short	increment;
	short	liste;
	short	pointeur;
	char	table_pointee[L_NOM_TABLE];
	char	attribut_pointe[L_NOM_ATTRIB];
  } ts_m_valeur;

/* Description of table obj_vue from database basetop */
 typedef struct zs_obj_vue_ {
	int	id_to_vue;
	char	nom_objet[L_NOM_OBJET];
  } ts_obj_vue;

/* Description of table objet from database basetop */
 typedef struct zs_objet_ {
	int	id_objet;
	short	type_objet;
	char	nom_objet[L_NOM_OBJET];
	char	producteur[L_NOM_PRODUCTEUR];
	char	ensemble[L_NOM_ENSEMBLE];
	char	origine[L_NOM_ENSEMBLE];
	int	date_creation;
	int	date_modif;
	int	heure_modif;
	short	on_off;
	short	barre;
	short	double_grise;
        short   tele;
	short	nb_param;
  } ts_objet;

/* Description of table occurence_diagnostic from database basetop */
 typedef struct zs_occurence_diagnostic_ {
	int	num_choc;
	int	id_diagnostic;
	char	repertoire[L_REPERTOIRE];
	int	id_occurence;
  } ts_occurence_diagnostic;

/* Description of table occurence_traitement from database basetop */
 typedef struct zs_occurence_traitement_ {
	int	num_choc;
	int entree2;
	int	id_traitement;
	int	date;
	int	heure;
	char	repertoire[L_REPERTOIRE];
	short   etat;
        float   duree;
        char    localisation[60];
        short   nbexec;
	int	id_occurence;
  } ts_occurence_traitement;

/*** JS - 05.2002 - Ajout de l'objet carte_chrono_vme ***/
  typedef struct ts_carte_chrono_vme_ {
        int     id_objet;
        short   cpt_util_chgt;
        short   cpt_util_horl;
        short   cpt_util_mode;
        short   cpt_util_interrup;
        short   cpt_date_horl;
        short   evtex_interrup;
        char    razdate_decl[13];
        short   razdate_sens;
        short   razdate_duree;
        short   razdate_auto;
        char    decl_mouchard[13];
  } ts_carte_chrono_vme;

  typedef struct ts_icv101_ {
        int     id_objet;        /* Assume 32-bit int */
        short   numero;
        short   noboard;
        short   type;
        short   stype;
        short   maitre;
        short   mode;
        short   softtrig;
        short   dema;
        short   maxram;
        short   datation;
        short   horloge;
        int     burst;          /* Assume 32-bit int */
        int     cadence; /* Assume 32-bit int */
        int     cadsd;   /* Assume 32-bit int */
        short   expostps;
        short   repsd;
        short   nvcont;
        short   nbmotifs;
        short   countpre;
        short   nbvoies;
  } ts_icv101;

  typedef struct ts_v_topmsdiv_ {
        int     id_param;
        short   rang;
        short   type;
        short   autorisation;
        char    topms[13];
        short   duree;
        short   sens;
  } ts_v_topmsdiv;

  typedef struct ts_v_graphe_ {
        int     id_param;       /* Assume 32-bit int */
        short   rang;
        short   numero_page;
        short   numero_graphe;
        char    titre[L_TITRE];
        float   xmin;
        float   xmax;
        float   ymin;
        float   ymax;
  } ts_v_graphe;

  typedef struct ts_v_signal_ {
        int     id_param;       /* Assume 32-bit int */
        short   rang;
        short   numero_page;
        short   numero_graphe;
        char    signal[L_NOM_OBJET];
        short   indice;
        char    couleur[L_COULEUR_SIGNAL];
        short   tri;
        char    valid[2];
  } ts_v_signal;

/* Description of table p_vue from database basetop */
 typedef struct zs_p_vue_ {
	int	id_to_vue;
	char	nom_param[L_NOM_ATTRIB];
	short	categ_param;
  } ts_p_vue;

/* Description of table parametre from database basetop */
 typedef struct zs_parametre_ {
	int	id_param;
	int	id_objet;
	char	nom_param[L_NOM_PARAM];
	short	rang;
	int	date_creation;
	int	date_modif;
	int	heure_modif;
	short	format;
	int	nb_val;
	short	categ_param;
  } ts_parametre;

/* Description of table responsable from database basetop */
 typedef struct zs_responsable_ {
	int	id_utilisateur;
	char	nom_programme[17];
	char	code_objet[2];
  } ts_responsable;

/* Description of table sequenceur_numerique from database basetop */

 typedef struct zs_sequenceur_numerique_ {
	int	id_objet;
	short	type;
	int	horloge;
	int	dureemax;
	short	repet;
  } ts_sequenceur_numerique;

/* Description of table strategie_diagnostic from database basetop */
 typedef struct zs_strategie_diagnostic_ {
	int	id_objet;
	short	numero;
	short	pretrig;
	int	flux;
  } ts_strategie_diagnostic;

/* Description of table strategie_pilote from database basetop */
 typedef struct zs_strategie_pilote_ {
	int	id_objet;
	short	numero;
	int	debut;
	int	fin;
	short	priorite;
  } ts_strategie_pilote;

/* Description of table to_vue from database basetop */
 typedef struct zs_to_vue_ {
	int	id_to_vue;
	int	id_vue;
	short	type_objet;
	char	libelle[L_LIBELLE_VUE];
  } ts_to_vue;

/* Description of table top from database basetop */
 typedef struct zs_top_ {
	char	producteur[L_NOM_PRODUCTEUR];
	char	ensemble[L_NOM_ENSEMBLE];
	char	nom_util[33];
	char	nom_machine[17];
  } ts_top;

/* Description of table tore_supra from database basetop */
 typedef struct zs_tore_supra_ {
	int	id_objet;
	char	phase[2];
  } ts_tore_supra;

/* Description of table traitement from database basetop */
 typedef struct zs_traitement_ {
	int	id_objet;
	short	version;
	int	id_traitement;
	int	creation;
	char	machine[17];
	char	type_machine[4];
	char	localisation[65];
	short	nb_entrees;
	short	nb_donnees;
	char	auteurs[65];
        short changement_version;
  } ts_traitement;

/* Description of table utilisateur from database basetop */
 typedef struct zs_utilisateur_ {
	char	nom_util[L_NOM_UTIL];
	char	nom_utilisateur[L_NOM_UTIL_UNIX];
	char	prenom_utilisateur[17];
	int	id_utilisateur;
	char	telephone[33];
	char	site[33];
	short	confidentialite;
	short	profil;
  } ts_utilisateur;

/* Description of table v_cadran from database basetop */
 typedef struct zs_v_cadran_ {
	int	id_param;
	short	rang;
	short	code;
	char	nom[L_NOM_DECLENCHE];
	char	affich[2];
	char	stratpil[L_NOM_STRATEGIE];
	char	comment[L_COMMENTAIRE];
  } ts_v_cadran;

/* Description of table v_char12 from database basetop */
 typedef struct zs_v_char12_ {
	int	id_param;
	short	rang;
	char	char12[13];
  } ts_v_char12;

  typedef struct ts_v_declenche_ {
        int     id_param;       /* Assume 32-bit int */
        short   rang;
        char    declenche[L_NOM_DECLENCHE];
  } ts_v_declenche;
  
/* Description of table v_texte from database mizar::arcad */
  typedef struct ts_v_texte_ {
        int     id_param;       /* Assume 32-bit int */
        short   rang;
        char    texte[L_TEXTE];
  } ts_v_texte;

/* Description of table v_chrono from database basetop */
 typedef struct zs_v_chrono_ {
	int	id_param;
	short	rang;
	char	valid[2];
	char	typedecl[2];
	char	repere[L_NOM_REPERE];
	char	declenche[L_NOM_DECLENCHE];
	float	decalage;
	char	ref[9];
	float	duree;
	char	comment[L_COMMENTAIRE];
  } ts_v_chrono;

/* Description of table v_common from database basetop */
 typedef struct zs_v_common_ {
	int	id_param;
	short	rang;
	char	nom_diag[L_NOM_PRODUCTEUR];
	char	nom_objet[L_NOM_OBJET];
	char	nom_param[L_NOM_PARAM];
	char	colonne[L_NOM_COLONNE];
	short	indice1;
	short	indice2;
	char	format[L_FORMAT];
  } ts_v_common;

/* Description of table v_courbe from database basetop */
 typedef struct zs_v_courbe_ {
	short	numero_pg;
	short	numero_gr;
	char	nomdonnee[19];
	short	tri;
	short	lissage;
	short	offset;
  } ts_v_courbe;

/* Description of table v_decl_cond from database basetop */
 typedef struct zs_v_decl_cond_ {
	int	id_param;
	short	rang;
	char	declenche[L_NOM_DECLENCHE];
	char	condition[33];
  } ts_v_decl_cond;

/* Description of table v_decl_seq from database basetop */
 typedef struct zs_v_decl_seq_ {
	int	id_param;
	short	rang;
	char	ctype[L_CTYPE];
	char	declenche[L_NOM_DECLENCHE];
	char	nom_carte[L_NOM_CARTE];
  } ts_v_decl_seq;

/* Description of table v_declenc from database basetop */
 typedef struct zs_v_declenc_ {
	int	id_param;
	short	rang;
	char	nom[L_NOM_DECLENCHE];
	float	temps;
  } ts_v_declenc;
  
/* 06.2001 - ajout du parametre descripteur */
/* Description of table v_descripteur from database arcad */
  typedef struct zs_v_descripteur_ {
        int     id_param;
        short   rang;
        int     num_choc;
        char    nom_diag[13];
        char    donnee_brute[13];
	short   indiceb;
        char    nom_trait[13];
        char    donnee_traitee[13];
	short   indicet;
  } ts_v_descripteur;


/* Description of table v_evtext from database basetop */
 typedef struct zs_v_evtext_ {
	int	id_param;
	short	rang;
	char	valid[2];
	char	nomref[L_NOM_REF];
	char	nomevt[L_NOM_EVENEMENT];
	float	debvalid;
	float	finvalid;
	char	comment[L_COMMENTAIRE];
  } ts_v_evtext;

/* Description of table v_float4 from database basetop */
 typedef struct zs_v_float4_ {
	int	id_param;
	short	rang;
	float	float4;
  } ts_v_float4;

/* Description of table v_float4_char64 from database arcad */
  typedef struct ts_v_float4_char64_ {
        int     id_param;       /* Assume 32-bit int */
        short   rang;
        float   float4;
        char    comment[65];
  } ts_v_float4_char64;

/* Description of table v_float4_float4 from database basetop */
 typedef struct zs_v_float4_float4_ {
	int	id_param;
	short	rang;
	float	float4_1;
	float	float4_2;
  } ts_v_float4_float4;

/* Description of table v_float8 from database basetop */
 typedef struct zs_v_float8_ {
	int	id_param;
	short	rang;
	double	float8;
  } ts_v_float8;

/* Description of table v_int1 from database basetop */
 typedef struct zs_v_int1_ {
	int	id_param;
	short	rang;
	short	integer1;
  } ts_v_int1;

/* Description of table v_int2 from database basetop */
 typedef struct zs_v_int2_ {
	int	id_param;
	short	rang;
	short	integer2;
  } ts_v_int2;

/* Description of table v_int2_char64 from database arcad */
  typedef struct ts_v_int2_char64_ {
        int     id_param;       /* Assume 32-bit int */
        short   rang;
        short   int2;
        char    comment[65];
  } ts_v_int2_char64;

/* 12.98 - ajout du parametre achrono */
/* Description of table v_achrono from database arcad */
  typedef struct ts_v_achrono_ {
        int     id_param;
        short   rang;
        char    code[2];
        char    declenche[13];
        float   decalage;
        float   duree;
        char    comment[33];
  } ts_v_achrono;


/* Description of table v_int2_int2 from database basetop */
 typedef struct zs_v_int2_int2_ {
	int	id_param;
	short	rang;
	short	int2_1;
	short	int2_2;
  } ts_v_int2_int2;

/* Description of table v_int4 from database basetop */
 typedef struct zs_v_int4_ {
	int	id_param;
	short	rang;
	int	integer4;
  } ts_v_int4;

/* Description of table v_int4_float4 from database basetop */
 typedef struct zs_v_int4_float4_ {
	int	id_param;
	short	rang;
	int	integer4;
	float	float4;
  } ts_v_int4_float4;

/* Description of table v_lien from database basetop */
 typedef struct zs_v_lien_ {
	int	id_param;
	short	rang;
	char	declench1[L_NOM_DECLENCHE];
	char	declench2[L_NOM_DECLENCHE];
  } ts_v_lien;

/* Description of table v_registres from database basetop */
 typedef struct zs_v_registres_ {
	int	id_param;
	short	rang;
	short	numreg;
	char	declenche[L_NOM_DECLENCHE];
	char	comment[L_COMMENTAIRE];
  } ts_v_registres;

/* Description of table v_sernam from database basetop */
 typedef struct zs_v_sernam_ {
	int	id_param;
	short	rang;
	short	code;
	char	nom[L_NOM_DECLENCHE];
	char	affich[2];
	char	comment[L_COMMENTAIRE];
  } ts_v_sernam;

/* Description of table v_stratdiag from database basetop */
 typedef struct zs_v_stratdiag_ {
	int	id_param;
	short	rang;
	char	nomdonnee[L_NOM_DONNEE];
	short	algo;
	short	a;
	short	b;
  } ts_v_stratdiag;

/* Description of table v_stratpil from database basetop */
 typedef struct zs_v_stratpil_ {
	int	id_param;
	short	rang;
	char	nom_diag[L_NOM_PRODUCTEUR];
	char	nom_strat[L_NOM_STRATEGIE];
  } ts_v_stratpil;

/* Description of table v_t_decode from database basetop */
 typedef struct zs_v_t_decode_ {
	int	id_param;
	short	rang;
	char	type[2];
	short	numero;
	short	polar;
	char	declench1[L_NOM_DECLENCHE];
	char	declench2[L_NOM_DECLENCHE];
	int	duree;
	char	comment[L_COMMENTAIRE];
  } ts_v_t_decode;

/* Description of table v_t_encode from database basetop */
 typedef struct zs_v_t_encode_ {
	int	id_param;
	short	rang;
	char	type[2];
	short	polar;
	char	declenche[L_NOM_DECLENCHE];
	short	nbsaut;
	short	nbmax;
	char	comment[L_COMMENTAIRE];
  } ts_v_t_encode;

/* Description of table v_t_it from database basetop */
 typedef struct zs_v_t_it_ {
	int	id_param;
	short	rang;
	short	polar;
	char	declenche[L_NOM_DECLENCHE];
  } ts_v_t_it;

/* Description of table v_tab_seq from database basetop */
 typedef struct zs_v_tab_seq_ {
	int	id_param;
	short	rang;
	short	voie;
	int	temps;
	int	cadence;
	short	nbimp;
  } ts_v_tab_seq;

/* Description of table v_voie from database basetop */
 typedef struct zs_v_voie_ {
	int	id_param;
	short	rang;
	short	voie;
	char	nom_carte[L_NOM_CARTE];
  } ts_v_voie;

/* Description of table v_voie_a from database basetop */
 typedef struct zs_v_voie_a_ {
	int	id_param;
	short	rang;
	short	voie;
	char	nom_carte[L_NOM_CARTE];
	float	a;
  } ts_v_voie_a;

/* Description of table v_voie_ab from database basetop */
 typedef struct zs_v_voie_ab_ {
	int	id_param;
	short	rang;
	short	voie;
	char	nom_carte[L_NOM_CARTE];
	float	a;
	float	b;
  } ts_v_voie_ab;

/* Description of table v_voie_gain from database basetop */
 typedef struct zs_v_voie_gain_ {
	int	id_param;
	short	rang;
	short	voie;
	short	gainpro;
	float	gainfix;
  } ts_v_voie_gain;

/* Description of table v_voie_seq from database basetop */
 typedef struct zs_v_voie_seq_ {
	int	id_param;
	short	rang;
	short	voie;
	short	polar;
	int	dureeimp;
	char	comment[L_COMMENTAIRE];
  } ts_v_voie_seq;

/* Description of table version from database basetop */
 typedef struct zs_version_ {
	int	id_objet;
	char	code_objet[2];
	int	id_donnee;
	short	rang_donnee;
  } ts_version;

/* Description of table vue from database basetop */
 typedef struct zs_vue_ {
	int	id_vue;
	char	nom_vue[L_NOM_VUE];
	char	proprietaire[L_NOM_UTIL_UNIX];
  } ts_vue;

/* Description of table tableau_de_bord from database basetop */
 typedef struct zs_tableau_de_bord {
	int	id_tableau;
	char	nom_tableau[L_NOM_TABLEAU];
	int	proprietaire;
  } ts_tableau_de_bord;

/* 12.98 - ajout du type d'objet achrono */
/* Description of table achrono from database arcad */
  typedef struct ts_achrono_ {
        int     id_objet;
        char    type[2];
        char    declenche[13];
        float   retard;
        float   maille;
  } ts_achrono;
