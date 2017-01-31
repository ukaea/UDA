#include "parseOperation.h"

#include <errno.h>
#include <stdlib.h>
#include <include/idamclientserverprivate.h>
#include "stringUtils.h"
#include "idamErrorLog.h"

int idamParseOperation(SUBSET* sub)
{

    char* p, * t1, * t2;
    char* endp = NULL;
    char opcopy[SXMLMAXSTRING];
    int i, ierr = 0;

//-------------------------------------------------------------------------------------------------------------
// Extract the Value Component from each separate Operation
// =0.15,!=0.15,<=0.05,>=0.05,!<=0.05,!>=0.05,<0.05,>0.05,0:25,25:0,25,*,25:,:25, 25:#
//
// Identify Three Types of Operations:
//	A) Contains the characters: =,>, <, !, ~
//	B) : or Integer Value
//	C) * or #  (* => ignore this dimension; # => Last Value in array)
//
// If the operation string is enclosed in [ ] then ignore these


    for (i = 0; i < sub->nbound; i++) {

        strcpy(opcopy, sub->operation[i]);

        if ((p = strchr(opcopy, '[')) != NULL) p[0] = ' ';
        if ((p = strchr(opcopy, ']')) != NULL) p[0] = ' ';
        LeftTrimString(opcopy);
        TrimString(opcopy);

        if ((p = strchr(opcopy, ':')) != NULL) {        // Integer Type Array Index Bounds
            t2 = &p[1];
            opcopy[p - opcopy] = '\0';            // Split the Operation String into two components
            t1 = opcopy;

            sub->isindex[i] = 1;
            sub->ubindex[i] = -1;
            sub->lbindex[i] = -1;

            if (t1[0] == '#') sub->lbindex[i] = -2;        // Reverse the data as # => Final array value

            if (strlen(t1) > 0 && t1[0] != '*' && t1[0] != '#') {
                if (IsNumber(t1)) {
                    sub->lbindex[i] = strtol(t1, &endp, 0);        // the Lower Index Value of the Bound
                    if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
                        ierr = 9999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverParseServerSide", ierr,
                                     "Server Side Operation Syntax Error: Lower Index Bound ");
                        return ierr;
                    }
                } else {
                    ierr = 9999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverParseServerSide", ierr,
                                 "Server Side Operation Syntax Error: Lower Index Bound ");
                    return ierr;
                }
            }

            if (strlen(t2) > 0 && t2[0] != '*' && t2[0] != '#') {
                if (IsNumber(t2)) {
                    sub->ubindex[i] = strtol(t2, &endp, 0);        // the Upper Index Value of the Bound
                    if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
                        ierr = 9999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverParseServerSide", ierr,
                                     "Server Side Operation Syntax Error: Upper Index Bound ");
                        return ierr;
                    }
                } else {
                    ierr = 9999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverParseServerSide", ierr,
                                 "Server Side Operation Syntax Error: Upper Index Bound ");
                    return ierr;
                }
            }

            strcpy(sub->operation[i], ":");            // Define Simple Operation
            continue;

        }

        if ((p = strstr(opcopy, "*")) != NULL) {        // Ignore this Dimension
            sub->isindex[i] = 1;
            sub->ubindex[i] = -1;
            sub->lbindex[i] = -1;
            strcpy(sub->operation[i], "*");            // Define Simple Operation
            continue;
        }

        if ((p = strstr(opcopy, "#")) != NULL) {        // Last Value in Dimension
            sub->isindex[i] = 1;
            sub->ubindex[i] = -1;
            sub->lbindex[i] = -1;
            strcpy(sub->operation[i], "#");            // Define Simple Operation
            continue;
        }

        if (IsNumber(opcopy)) {                    // Single Index value
            sub->isindex[i] = 1;
            sub->ubindex[i] = strtol(opcopy, &endp, 0);        // the Index Value of the Bound
            if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
                ierr = 9999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverParseServerSide", ierr,
                             "Server Side Operation Syntax Error: Single Index Bound ");
                return ierr;
            }
            sub->lbindex[i] = sub->ubindex[i];
            strcpy(sub->operation[i], ":");        // Define Simple Operation
            continue;
        }

    }

    return 0;
}
