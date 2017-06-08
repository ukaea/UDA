#include "putUnits.h"

#include <locale.h>
#include <stdio.h>
#include <udunits2.h>

#include <logging/logging.h>

static ut_system* unitSystem = NULL;

int testUnitsCompliance(const char* units)
{

  IDAM_LOG(LOG_DEBUG, "Testing unit compliance\n");
    // Return TRUE (1) or FALSE (0)

    ut_unit* encoded = NULL;
    ut_encoding encoding = UT_ASCII;    // UT_UTF8

    //--------------------------------------------------------------------------
    // Check Units Compliance

    if (unitSystem == NULL) {
      IDAM_LOG(LOG_DEBUG, "Unit system is NULL: Try to retrieve it\n");
        unitSystem = ut_read_xml(NULL);
        if (!setlocale(LC_CTYPE, "")) {
            IDAM_LOG(LOG_DEBUG, "Can't set the specified locale! Check LANG, LC_CTYPE, LC_ALL\n");
            return 0;
        }
    }

    IDAM_LOGF(LOG_DEBUG, "Parse units %s\n", units);

    if (unitSystem == NULL){
      IDAM_LOG(LOG_DEBUG, "unitSystem is NULL\n");
    }

    if ((encoded = ut_parse(unitSystem, units, encoding)) == NULL) {
        int code = (int) ut_get_status();
        IDAM_LOG(LOG_ERROR, "Units [%s] are Not SI Compliant.\n");
        if (code == UT_SYNTAX) {
            fprintf(stderr, "Units contain a Syntax Error\n");
        } else {
            if (code == UT_UNKNOWN) {
                fprintf(stderr, "Units contain an Unknown Identifier\n");
            } else { fprintf(stderr, "Error Code: %d\n", code); }
        }
        return 0;
    }

    IDAM_LOG(LOG_DEBUG, "Free encoded\n");
    ut_free(encoded);

    return 1;
} 
