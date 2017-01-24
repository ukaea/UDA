;###############################################################################
;
; Name: sign_dt.pro
;
; Version: v2003_1.3
;
; Description: Returns the sign (+,-) of the input:- -1, 0 or +1
;
;     Authors: <c> 2003
;              DMAT (dtaylor@ukaea.org.uk)
;
;     Remarks: - 	
;
;     History: - v1.1 - DMAT 2001/12/13
;		 v1.2 - Remove unnecessary '@ida3' - DMAT 2002/12/18
;		 v1.3 - Add /POSZERO and /NEGZERO keywords - DMAT 2003/1/21
;			
;     Calls   -  e.g. var=SIGN_DT(any dimensioned numerical array [,/POSZERO])
;
;		 var is an array of the same size as [x] containing the unit signs
;		 of [x] :- -1, 0 or +1.	
;		
;     Keywords - /POSZERO - set to return zeroes as positives i.e. +1.
;		 /NEGZERO - set to return zeroes as negatives i.e. -1.
;
;###############################################################################

FUNCTION SIGN_DT,x,POSZERO=poszero,NEGZERO=negzero

IF KEYWORD_SET(poszero) AND KEYWORD_SET(negzero) THEN $
  PRINT,'** Contradictory keywords /POSZERO and /NEGZERO set in SIGN_DT - ignoring both'	;existing algorithm will cope

  ;Returns the sign (+,-) of the input:- -1, 0 or +1

IF KEYWORD_SET(poszero) THEN $
		pos=x GE 0 $
	ELSE $
		pos=x GT 0	;values greater than 0
IF KEYWORD_SET(negzero) THEN $
		neg=x LE 0 $
	ELSE $
		neg=x LT 0	;values less than 0

neg=FIX(neg)	
neg=-neg        ;convert 1s to -1s

ans=pos+neg

RETURN,ans
END