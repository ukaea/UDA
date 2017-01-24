//! $LastChangedRevision: 265 $
//! $LastChangedDate: 2011-06-02 15:28:56 +0100 (Thu, 02 Jun 2011) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/plugins/ida/nameIda.c $

/*---------------------------------------------------------------
* Form the Correct IDA Filename from the 3 character Alias form
*
* Input Arguments:	char   *alias
*			int	pulno
*	
* Returns:		char   *filename                        
*
* Change History

* 0.0  05-Aug-2004	D.G.Muir
* 0.1  09Jul2007 D.G.Muir	debugon, verbose enabled
* 02Jun2011 dgm	Include source only if NOIDAPLUGIN is not defined
*------------------------------------------------------------------*/

#include <stdio.h>
#include <TrimString.h>
#include <idamLog.h>

#ifndef NOIDAPLUGIN

//#include <ida3.h>

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

    idamLog(LOG_DEBUG, "IDA_Filename: %s\n", filename);

    return;
}

#endif
