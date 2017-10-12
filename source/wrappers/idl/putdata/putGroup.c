
#include "putdata.h"		// IDL API Header

#include <stdlib.h>

int putGroup(KW_RESULT* kw, int* ncgrpid) {

    int err, lerr, status;
    char* group;
    char* defgrp = "/";
    char* work = NULL, * token = NULL;

//---------------------------------------------------------------------------      
// Group Name

    if (kw->is_group)
        group = IDL_STRING_STR(&kw->group);
    else
        group = defgrp;

//--------------------------------------------------------------------------      
// Create an Error Trap

    lerr = 0;        // Local Error
    err = 0;        // NC Error

    do {

//--------------------------------------------------------------------------      
// Locate or Define the Group (Can use / for Top Level - Ignored!)

        if (kw->debug) fprintf(stdout, "#Testing whole group Hierarchy [%s]\n", group);

        if (strcmp(group, "/") != 0) {                // Not required if Top Level Group

            work = (char*) malloc((strlen(group) + 1) * sizeof(char));
            strcpy(work, group);

            if ((token = strtok(work, "/")) != NULL) {        // Tokenise for 1 or more grouping levels

                if (kw->debug) fprintf(stdout, "#Testing group [%s]\n", token);

                if ((err = testgroup(ncfileid, token, &status, ncgrpid, kw->debug, kw->verbose)) != NC_NOERR) {
                    if (kw->verbose) fprintf(stderr, "Unable to Find or Define a Top Level Group\n");
                    break;
                }

                if (kw->debug) {
                    fprintf(stdout, "Status   %d\n", status);
                    fprintf(stdout, "Group ID %d\n", *ncgrpid);
                }

                if (status == 1) {        // Create all Child Nodes

                    if (kw->debug) fprintf(stdout, "Group %s did not exist - Created with all child groups\n", token);

                    while ((token = strtok(NULL, "/")) != NULL) {
                        if (kw->debug) fprintf(stdout, "Creating Group [%s]\n", token);
                        if ((err = nc_def_grp(*ncgrpid, token, ncgrpid)) != NC_NOERR) {
                            if (kw->verbose) fprintf(stderr, "Unable to Create a Child Group named [%s]\n", token);
                            break;
                        }
                    }
                } else {

// Find or Define Child Nodes

                    if (kw->debug) fprintf(stdout, "Group [%s] Found - Locating all child groups\n", token);

                    while ((token = strtok(NULL, "/")) != NULL) {        // Loop over all Intermediate Groups

                        if (kw->debug) fprintf(stdout, "Testing Group [%s]\n", token);

                        if ((err = testgroup(*ncgrpid, token, &status, ncgrpid, kw->debug, kw->verbose)) != NC_NOERR) {
                            if (kw->verbose)
                                fprintf(stderr, "Unable to Find or Define an Intermediate Level Group [%s]\n", token);
                            break;
                        }

                        if (kw->debug) {
                            fprintf(stdout, "Status   %d\n", status);
                            fprintf(stdout, "Group ID %d\n", *ncgrpid);
                        }

                        if (status == 1) {                    // Create all Child Nodes
                            if (kw->debug)
                                fprintf(stdout, "Group [%s] did not exist - Created with all child groups\n", token);
                            while ((token = strtok(NULL, "/")) != NULL) {
                                if (kw->debug) fprintf(stdout, "Creating Group [%s]\n", token);
                                if ((err = nc_def_grp(*ncgrpid, token, ncgrpid)) != NC_NOERR) {
                                    if (kw->verbose)
                                        fprintf(stderr, "Unable to Create a Child Group named [%s]\n", token);
                                    break;
                                }
                            }
                            break;
                        } else if (kw->debug) fprintf(stdout, "Group [%s] Found\n", token);

                    }

                    if (err != NC_NOERR) break;

                }
            }

            free((void*) work);
            work = NULL;

        } else {

            if (kw->debug) fprintf(stdout, "Top Level Group Selected\n");

            *ncgrpid = ncfileid;        // Top Level Group
        }

//--------------------------------------------------------------------------      
// End of Error Trap  

    } while (0);

    if (err != NC_NOERR) {
        if (kw->verbose) fprintf(stderr, "Error Report: %s\n", nc_strerror(err));
    } else {
        err = lerr;
    }


//--------------------------------------------------------------------------      
// Cleanup Keywords 

    if (work != NULL)free((void*) work);

    return err;
}


int testgroup(int ncgrpid, char* target, int* status, int* targetid, int debug, int verbose) {

    int i, err = 0, numgrps = 0, namelength = 0;
    int* ncids = NULL;
    char* grpname = NULL;

    *status = 0;        // 0 => Exists; 1 => Created
    *targetid = -1;        // Target Group ID

// Does this group exist?
// Obtain List from the level

    if (debug)fprintf(stdout, "How many Child Groups (ID %d) ?\n", ncgrpid);

    if ((err = nc_inq_grps(ncgrpid, &numgrps, NULL)) != NC_NOERR) {
        if (verbose) fprintf(stderr, "Unable to Count the Child Groups\n");
        return err;
    }

    if (debug)fprintf(stdout, "Child Groups Count = %d\n", numgrps);

    if (numgrps > 0) {

        ncids = (int*) malloc(sizeof(int) * numgrps);

        if (debug)fprintf(stdout, "Listing Child Groups\n");

        if ((err = nc_inq_grps(ncgrpid, &numgrps, ncids)) != NC_NOERR) {
            if (verbose) fprintf(stderr, "Unable to List the Group IDs\n");
            free((void*) ncids);
            return err;
        }

// Test existing Groups

	int ngrps = numgrps;
        for (i = 0; i < ngrps; i++) {

            if (debug)fprintf(stdout, "Testing Child Group %d\n", ncids[i]);

            if ((err = nc_inq_grpname_len(ncids[i], (size_t*) &namelength)) != NC_NOERR) {
                if (verbose) fprintf(stderr, "Unable to Obtain Group Name Length\n");
                if (grpname != NULL) free((void*) grpname);
                free((void*) ncids);
                return err;
            }

            grpname = (char*) realloc((void*) grpname, sizeof(char) * (namelength + 1));

            if ((err = nc_inq_grpname(ncids[i], grpname)) != NC_NOERR) {
                if (verbose) fprintf(stderr, "Unable to Name an existing Group\n");
                free((void*) grpname);
                free((void*) ncids);
                return err;
            }

            if (debug)fprintf(stdout, "Comparing Group Name [%s] with Target Group [%s]\n", grpname, target);

            if (STR_EQUALS(grpname, target)) {
                if (debug)fprintf(stdout, "Group Found %d\n", ncids[i]);

                *targetid = ncids[i];            // Found - it exists!
                *status = 0;
                free((void*) grpname);
                free((void*) ncids);
                return 0;
            }
        }

        free((void*) grpname);
        free((void*) ncids);
    }

    if (*targetid == -1) {                    // Doesn't exist so create it
        if (debug)fprintf(stdout, "Creating the Group [%s] \n", target);

        if ((err = nc_def_grp(ncgrpid, target, targetid)) != NC_NOERR) {
            if (verbose) fprintf(stderr, "Unable to Create a Named Group [%s]\n", target);
            return err;
        }
        *status = 1;
    }

    return 0;
}

