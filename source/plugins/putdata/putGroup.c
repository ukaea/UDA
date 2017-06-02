#include "putGroup.h"

#include <netcdf.h>
#include <stdlib.h>

#include <clientserver/stringUtils.h>

#include "putOpenClose.h"

int do_group(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    //---------------------------------------------------------------------------
    // Group Name

    const char* name = "/";
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, name);

    int fileid = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, fileid);

    int ncfileid = get_file_id(fileid);
    if (ncfileid < 0) {
        RAISE_PLUGIN_ERROR("Invalid fileid given");
    }

    //--------------------------------------------------------------------------
    // Locate or Define the Group (Can use / for Top Level - Ignored!)

    IDAM_LOGF(LOG_DEBUG, "Testing whole group Hierarchy [%s]\n", name);

    if (!STR_EQUALS(name, "/")) {
        // Not required if Top Level Group

        char* work = (char*) malloc((strlen(name) + 1) * sizeof(char));
        strcpy(work, name);

        char* token;
        if ((token = strtok(work, "/")) != NULL) {
            // Tokenise for 1 or more grouping levels

            IDAM_LOGF(LOG_DEBUG, "Testing group [%s]\n", token);

            int status;
            int grpid;

            if (testgroup(ncfileid, token, &status, &grpid) != NC_NOERR) {
                RAISE_PLUGIN_ERROR("Unable to Find or Define a Top Level Group");
            }

            IDAM_LOGF(LOG_DEBUG, "Status   %d", status);
            IDAM_LOGF(LOG_DEBUG, "Group ID %d", grpid);

            if (status == 1) {
                // Create all Child Nodes

                IDAM_LOGF(LOG_DEBUG, "Group %s did not exist - Created with all child groups", token);

                while ((token = strtok(NULL, "/")) != NULL) {
                    IDAM_LOGF(LOG_DEBUG, "Creating Group [%s]", token);
                    if (nc_def_grp(grpid, token, &grpid) != NC_NOERR) {
                        RAISE_PLUGIN_ERROR("Unable to Create a Child Group");
                    }
                }
            } else {

                // Find or Define Child Nodes

                IDAM_LOGF(LOG_DEBUG, "Group [%s] Found - Locating all child groups", token);

                while ((token = strtok(NULL, "/")) != NULL) {        // Loop over all Intermediate Groups

                    IDAM_LOGF(LOG_DEBUG, "Testing Group [%s]", token);

                    if (testgroup(grpid, token, &status, &grpid) != NC_NOERR) {
                        RAISE_PLUGIN_ERROR("Unable to Find or Define an Intermediate Level Group");
                    }

                    IDAM_LOGF(LOG_DEBUG, "Status   %d", status);
                    IDAM_LOGF(LOG_DEBUG, "Group ID %d", grpid);

                    if (status == 1) {                    // Create all Child Nodes
                        IDAM_LOGF(LOG_DEBUG, "Group [%s] did not exist - Created with all child groups", token);
                        while ((token = strtok(NULL, "/")) != NULL) {
                            IDAM_LOGF(LOG_DEBUG, "Creating Group [%s]", token);
                            if (nc_def_grp(grpid, token, &grpid) != NC_NOERR) {
                                RAISE_PLUGIN_ERROR("Unable to Create a Child Group");
                            }
                        }
                        break;
                    } else {
                        IDAM_LOGF(LOG_DEBUG, "Group [%s] Found", token);
                    }

                }
            }
        }

        free((void*) work);

    } else {
        IDAM_LOG(LOG_DEBUG, "Top Level Group Selected");
    }

    return 0;
}

int testgroup(int locid, const char* target, int* status, int* targetid)
{
    *status = 0;        // 0 => Exists; 1 => Created
    *targetid = -1;     // Target Group ID

    // Does this group exist?
    // Obtain List from the level

    IDAM_LOGF(LOG_DEBUG, "How many Child Groups (ID %d) ?\n", locid);

    int numgrps;

    if (nc_inq_grps(locid, &numgrps, NULL) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Count the Child Groups");
    }

    IDAM_LOGF(LOG_DEBUG, "Child Groups Count = %d\n", numgrps);

    if (numgrps > 0) {

        int* ncids = (int*) malloc(sizeof(int) * numgrps);

        IDAM_LOG(LOG_DEBUG, "Listing Child Groups");

        if (nc_inq_grps(locid, &numgrps, ncids) != NC_NOERR) {
            free((void*) ncids);
            RAISE_PLUGIN_ERROR("Unable to List the Group IDs\n");
        }

        // Test existing Groups

        int i;
        for (i = 0; i < numgrps; i++) {
            IDAM_LOGF(LOG_DEBUG, "Testing Child Group %d\n", ncids[i]);

            char* grpname = NULL;
            int namelength;

            if (nc_inq_grpname_len(ncids[i], (size_t*) &namelength) != NC_NOERR) {
                free((void*) grpname);
                free((void*) ncids);
                RAISE_PLUGIN_ERROR("Unable to Obtain Group Name Length");
            }

            grpname = (char*) realloc((void*) grpname, sizeof(char) * (namelength + 1));

            if (nc_inq_grpname(ncids[i], grpname) != NC_NOERR) {
                free((void*) grpname);
                free((void*) ncids);
                RAISE_PLUGIN_ERROR("Unable to Name an existing Group");
            }

            IDAM_LOGF(LOG_DEBUG, "Comparing Group Name [%s] with Target Group [%s]", grpname, target);

            if (STR_EQUALS(grpname, target)) {
                IDAM_LOGF(LOG_DEBUG, "Group Found %d\n", ncids[i]);
                *targetid = ncids[i]; // Found - it exists!
                *status = 0;
                free((void*) grpname);
                free((void*) ncids);
                return 0;
            }

            free((void*) grpname);
        }

        free((void*) ncids);
    }

    if (*targetid == -1) {
        // Doesn't exist so create it
        IDAM_LOGF(LOG_DEBUG, "Creating the Group [%s]\n", target);

        if (nc_def_grp(locid, target, targetid) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Create a Named Group");
        }
        *status = 1;
    }

    return NC_NOERR;
}

