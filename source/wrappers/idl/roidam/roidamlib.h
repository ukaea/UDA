#ifndef UDA_WRAPPERS_IDL_ROIDAMLIB_H
#define UDA_WRAPPERS_IDL_ROIDAMLIB_H

#include <stdio.h>

#include <clientserver/parseXML.h>

int queryIdamTable(char* sql, int verbose, FILE* fh, char* out);

int countIdamTable(char* sql, int verbose, FILE* fh);

int executeIdamSQL(char* sql, int verbose, FILE* fh);

void sqlIdamPatternMatch(char* in, char* out);

int checkSignalAuthorisation(int isKey, char* whr, char* user, int verbose, FILE* fh);

int checkSourceAuthorisation(char* source, char* user, int verbose, FILE* fh);

int checkAdminAuthorisation(char* user, int verbose, FILE* fh);

void copyXMLRanges(ACTION in, ACTION* out);

int combineIdamActions(int replace, ACTIONS newactions, ACTIONS* actions);

int createIdamActionXML(ACTIONS actions, char* xml);

#endif // UDA_WRAPPERS_IDL_ROIDAMLIB_H
