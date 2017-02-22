/*---------------------------------------------------------------
* Form the Correct IDA Filename from the 3 character Alias form
*
* Input Arguments:	char   *alias
*			int	pulno
*	
* Returns:		char   *filename                        
*
*------------------------------------------------------------------*/

#include <stdio.h>

#include <clientserver/stringUtils.h>
#include <logging/logging.h>

#ifndef NOIDAPLUGIN

//-------------------------------------------------------------------
// Form the Filename of the IDA File

void nameIDA(char * alias, int pulno, char * filename)
{

    char strint[7];
    int pulno_lhs, pulno_rhs;

    sprintf(strint, "%d", pulno);

    strncpy(filename, alias, 3);
    filename[3] = '\0';
    TrimString(filename);

    pulno_lhs = pulno / 100;
    pulno_rhs = pulno - 100 * (pulno / 100);

    sprintf(strint, "%d", pulno_lhs);
    strcat(filename, "0000");

    if (pulno_lhs < 10 && pulno_lhs > 0) {
        filename[6] = strint[0];
    } else {
        if (pulno_lhs < 100) {
            filename[5] = strint[0];
            filename[6] = strint[1];
        } else {
            if (pulno_lhs < 1000) {
                filename[4] = strint[0];
                filename[5] = strint[1];
                filename[6] = strint[2];
            } else {
                filename[3] = strint[0];
                filename[4] = strint[1];
                filename[5] = strint[2];
                filename[6] = strint[3];
            }
        }
    }

    sprintf(strint, "%d", pulno_rhs);

    strcat(filename, ".");
    if (pulno_rhs < 10) {
        strcat(filename, "0");
    }
    strcat(filename, strint);

    IDAM_LOGF(LOG_DEBUG, "IDA_Filename: %s\n", filename);

    return;
}

#endif
