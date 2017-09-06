
#include "putdata.h"		// IDL API Header


int putDevice(int nparams, IDL_VPTR argv[], KW_RESULT *kw) {

    int err, lerr;
    int ncgrpid, status;
    char *type, *id, *serial, *device;

//---------------------------------------------------------------------------      
// Extract Keywords 

    type = NULL;
    id = NULL;
    serial = NULL;

    if (kw->is_type) type = IDL_STRING_STR(&kw->type);
    if (kw->is_id) id = IDL_STRING_STR(&kw->id);
    if (kw->is_serial) serial = IDL_STRING_STR(&kw->serial);

//--------------------------------------------------------------------------      
// Create an Error Trap

    err = NC_NOERR;
    lerr = NC_NOERR;

    do {

//---------------------------------------------------------------------------      
// Device Name Parameter

        if (nparams == 1 && argv[0]->type == IDL_TYP_STRING) {
            device = IDL_STRING_STR(&(argv[0]->value.str));        // The Device Name - defines the group
        } else {
            if (kw->verbose) fprintf(stderr, "The Parameter must be a String naming the Device\n");
            lerr = -1;
            break;
        }

        if (kw->debug) fprintf(stdout, "The Device is named %s\n", device);


//--------------------------------------------------------------------------         
// Check the Top Level Group Named 'Devices' exists - Create if not

        if ((lerr = testgroup(ncfileid, "devices", &status, &ncgrpid, kw->debug, kw->verbose)) != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to Find or Define the Top Level 'devices' Group\n");
            break;
        }

//--------------------------------------------------------------------------         
// Create a Child Group named after the device

        if ((lerr = testgroup(ncgrpid, device, &status, &ncgrpid, kw->debug, kw->verbose)) != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to Create a Devices Child Group named %s\n", device);
            break;
        }

//--------------------------------------------------------------------------         
// Attributes

        if (kw->is_type && (err = nc_put_att_text(ncgrpid, NC_GLOBAL, "type", strlen(type), type)) != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to Write the %s Device Type Attribute: %s\n", device, type);
            break;
        }

        if (kw->is_id && (err = nc_put_att_text(ncgrpid, NC_GLOBAL, "id", strlen(id), id)) != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to Write the %s Device ID Attribute: %s\n", device, id);
            break;
        }

        if (kw->is_serial &&
            (err = nc_put_att_text(ncgrpid, NC_GLOBAL, "serial", strlen(serial), serial)) != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to Write the %s Device Serial Attribute: %s\n", device, serial);
            break;
        }

        if (kw->is_resolution) {
            short resolution = (short) kw->resolution;
            if ((err = nc_put_att_short(ncgrpid, NC_GLOBAL, "resolution", NC_SHORT, 1, &resolution)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Write the %s Device Resolution: %d\n", device, (int) kw->resolution);
                break;
            }
        }

        if (kw->is_range) {
            float range[2];
            range[0] = (float) kw->range[0];
            range[1] = (float) kw->range[1];
            if ((err = nc_put_att_float(ncgrpid, NC_GLOBAL, "range", NC_FLOAT, 2, range)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Write the %s Device Range: %e : %e \n", device, kw->range[0],
                            kw->range[1]);
                break;
            }
        }

        if (kw->is_channels) {
            short channels = (short) kw->channels;
            if ((err = nc_put_att_short(ncgrpid, NC_GLOBAL, "channels", NC_SHORT, 1, &channels)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Write the %s Device Channel Count Attribute: %d\n", device,
                            (int) kw->channels);
                break;
            }
        }


//--------------------------------------------------------------------------      
// End of Error Trap  

    } while (0);

    if (err != NC_NOERR) {
        if (kw->verbose) fprintf(stderr, "Error Report: %s\n", nc_strerror(err));
    } else {
        err = lerr;
    }

    return err;
} 

