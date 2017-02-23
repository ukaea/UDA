/*---------------------------------------------------------------
// Help: A Description of library functionality
// Information is returned as a single string containing all required format control characters.
*---------------------------------------------------------------------------------------------------------------*/
#include "help.h"

#include <stdlib.h>

#include <structures/struct.h>
#include <structures/accessors.h>

#include "provenance.h"

int help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    int err = 0;
    int offset = 0;

    char work[MAXSQL];
    int stringLength;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    if (idam_plugin_interface->interfaceVersion == 1) {

        idam_plugin_interface->pluginVersion = 1;

        data_block = idam_plugin_interface->data_block;

    } else {
        err = 999;
        IDAM_LOG(LOG_ERROR, "ERROR Provenance: Plugin Interface Version Unknown\n");

        addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err,
                     "Plugin Interface Version is Not Known: Unable to execute the request!");
        return err;
    }

    IDAM_LOG(LOG_DEBUG, "Provenance: Plugin Interface transferred\n");

//----------------------------------------------------------------------------------------
// Common Name Value pairs

// Keywords have higher priority

//----------------------------------------------------------------------------------------
// Error trap

    err = 0;

    do {

        IDAM_LOG(LOG_DEBUG, "Provenance: entering function help\n");

        strcpy(work, "\nProvenance: Issue and register a new UUID for a specific scientific study.\n\n"

                "Summary of functions:\n\n"
                "get\t\tRegister and return a new UUID with an 'OPEN' status\n"
                "status\t\tEnquire about or change a registered provenance UUID's status\n"
                "put\t\tRecord metadata in the Provenance database and copy files to the Provenance data archive\n"
                "listSignals\tList all signals accessed with a specific client's UUID\n"
                "putSignal\tRecord a signal's use by an application with a specific UUID\n\n\n"

                "get(owner=owner, status=status, class=class, title=title, description=description, "
                "icatRef=icatRef, /returnUUID)\n\n"

                "status(uuid=uuid, status=[Open | Closed | Delete | Firm | Pending], /returnStatus)\n\n"

                "put(uuid=uuid, fileLocation=fileLocation, format=format)\n\n"

                "listSignals(uuid=uuid)\n\n"

                "putSignal(user=user, uuid=uuid, requestedSignal=requestedSignal, requestedSource=requestedSource, \n"
                "\ttrueSignal=trueSignal, trueSource=trueSource, trueSourceUUID=trueSourceUUID, \n"
                "\tlogRecord=logRecord, created=created, status=[New|Update|Close|Delete])\n\n"
        );

        IDAM_LOGF(LOG_DEBUG, "Provenance:\n%s\n", work);

// Create the Returned Structure Definition

        initUserDefinedType(&usertype);            // New structure definition

        strcpy(usertype.name, "PROVENANCEHELP");
        strcpy(usertype.source, "PROVENANCE:admin");
        usertype.ref_id = 0;
        usertype.imagecount = 0;                // No Structure Image data
        usertype.image = NULL;
        usertype.size = sizeof(PROVENANCEHELP);        // Structure size
        usertype.idamclass = TYPE_COMPOUND;

        offset = 0;

        defineField(&field, "help", "Information about the Provenance plugin functions", &offset,
                    SCALARSTRING);    // Single string, arbitrary length
        addCompoundField(&usertype, field);
        addUserDefinedType(userdefinedtypelist, usertype);

// Create Data	 

        PROVENANCEHELP* data;
        stringLength = strlen(work) + 1;
        data = (PROVENANCEHELP*)malloc(sizeof(PROVENANCEHELP));
        data->value = (char*)malloc(stringLength * sizeof(char));
        strcpy(data->value, work);
        addMalloc((void*)data, 1, sizeof(PROVENANCEHELP), "PROVENANCEHELP");
        addMalloc((void*)data->value, 1, stringLength * sizeof(char), "char");

// Pass Data	 

        data_block->data_type = TYPE_COMPOUND;
        data_block->rank = 0;
        data_block->data_n = 1;
        data_block->data = (char*)data;

        strcpy(data_block->data_desc, "Provenance Plugin help");
        strcpy(data_block->data_label, "");
        strcpy(data_block->data_units, "");

        data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
        data_block->opaque_count = 1;
        data_block->opaque_block = (void*)findUserDefinedType("PROVENANCEHELP", 0);

        IDAM_LOG(LOG_DEBUG, "Provenance: exiting function help\n");
        if (data_block->opaque_block == NULL) IDAM_LOG(LOG_DEBUG, "Provenance: PROVENANCEHELP type not found\n");

        break;

    } while (0);

    return err;
}
