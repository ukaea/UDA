/*---------------------------------*/
/* tsparse.c                     */
/*---------------------------------*/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include "tsdef.h"
#include "tsconf.h"
#include "tstype.h"
#include "tsbamat.h"

int
tsparse(char* string, char* nomsig, int indices[], int* extr, int* occur)
{
    char* pt1, * pt2;
    indices[0] = indices[1] = 0;
    *occur = 0;
    *extr = 0;

    pt1 = string;
    pt2 = nomsig;

    while (*pt1 && (*pt1 != SEP1) && (*pt1 != SEP2) && (*pt1 != SEP3))
        *pt2++ = *pt1++;
    *pt2 = '\0';
    switch (*pt1) {
        case SEP3:
            pt1++;  /* Separateur occurrence # */
            if (*pt1 != '\0' && isdigit(*pt1)) {
                *occur = atol(pt1);
                pt1++;
            }
            break;

        case SEP1:
            pt1++; /* Separateur index groupe % */
            if (*pt1 != '\0' && isdigit(*pt1) && nomsig[0] == 'G') {
                indices[0] = atol(pt1);
                *extr = 1;
                pt1++;
            }
            if (*pt1 == SEP2) { /* 2eme Separateur index groupe . */
                pt1++;
                if (*pt1 != '\0' && isdigit(*pt1)) {
                    indices[1] = atol(pt1);
                }
                pt1++;
            }
            break;

        default :
            break;
    }

    if (*pt1 != '\0') {
        switch (*pt1) {
            case SEP1:
                pt1++;
                if (*pt1 != '\0' && isdigit(*pt1) && nomsig[0] == 'G') {
                    indices[0] = atol(pt1);
                    *extr = 1;
                    pt1++;
                }
                if (*pt1 == SEP2) {
                    pt1++;
                    if (*pt1 != '\0' && isdigit(*pt1)) {
                        indices[1] = atol(pt1);
                    }
                    pt1++;
                }
                break;

            case SEP3:
                pt1++;
                if (*pt1 != '\0' && isdigit(*pt1)) {
                    *occur = atol(pt1);
                    pt1++;
                }
                break;

            default :
                break;
        }
    }
    return 0;
}

