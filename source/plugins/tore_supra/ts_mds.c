#include "ts_mds.h"
#include <stdlib.h>
#include <mdslib.h>

#define status_ok(status) (((status) & 1) == 1)

int get_signal_length(const char *signal)
{
    /* local vars */
    int dtype_long = DTYPE_LONG;
    char buf[1024];
    int size;
    int null = 0;
    int idesc = descr(&dtype_long, &size, &null);
    int status;

    /* init buffer */
    memset(buf, 0, sizeof(buf));

    /* put SIZE() TDI function around signal name */
    snprintf(buf, sizeof(buf) - 1, "SIZE(%s)", signal);

    /* use MdsValue to get the signal length */
    status = MdsValue(buf, &idesc, &null, NULL);
    if (!((status & 1) == 1)) {
	fprintf(stderr, "Unable to get length of %s.\n", signal);
	return -1;
    }

    /* return signal length */
    return size;

}

int ts_mds_get(const char *signalName, int shot, float **time, float **data, int *len)
{

    int dtype_float = DTYPE_FLOAT;
    int null = 0;
    int status;
    /* Connect to MDSplus */
    int socket = MdsConnect("altair.partenaires.cea.fr:8000");
    if (socket == -1) {
	    fprintf(stderr, "Error connecting to altair.partenaires.cea.fr.\n");
    	return -1;
    }

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, "GetTsBaseITM(%d, '%s')", shot, signalName);

    *len = get_signal_length(buf);

    if (len < 0) {
	    fprintf(stderr, "Unable to get signal length.\n");
	    return -1;
    }

    *time = malloc(*len * sizeof(float));
    *data = malloc(*len * sizeof(float));

    int fdesc = descr(&dtype_float, *time, len, &null);
    int rlen = 0;

    //Get time data
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, "dim_of(GetTsBaseITM(%d, '%s'), 0)", shot, signalName);

    status = MdsValue(buf, &fdesc, &null, &rlen, NULL);
    if (!((status & 1) == 1)) {
	    fprintf(stderr, "Unable to get signal.\n");
    	return -1;
    }

    fdesc = descr(&dtype_float, *data, len, &null);

    //Get data
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, "GetTsBaseITM(%d, '%s')", shot, signalName);


    status = MdsValue(buf, &fdesc, &null, &rlen, NULL);
    if (!((status & 1) == 1)) {
	    fprintf(stderr, "Unable to get signal.\n");
	    return -1;
    }


    return 0;
}
