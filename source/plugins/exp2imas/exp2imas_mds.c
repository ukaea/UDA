#include "exp2imas_mds.h"

#include <stdlib.h>
#include <mdslib.h>
#include <mdsshr.h>

#include <logging/logging.h>
#include <clientserver/stringUtils.h>

#include "exp2imas_ssh.h"
#include "exp2imas_ssh_server.h"

#define status_ok(status) (((status) & 1) == 1)

static int get_signal_length(const char* signal)
{
    /* local vars */
    int dtype_long = DTYPE_LONG;
    char buf[2048];
    int size;
    int null = 0;
    int idesc = descr(&dtype_long, &size, &null);
    int status;

    /* put SIZE() TDI function around signal name */
    sprintf(buf, "%s; SIZE(_sig);", signal);

    /* use MdsValue to get the signal length */
    status = MdsValue(buf, &idesc, &null, NULL);
    if (((status & 1) != 1)) {
        UDA_LOG(UDA_LOG_ERROR, "Unable to get length of %s.\n", signal);
        return -1;
    }

    /* return signal length */
    return size;

}

typedef struct ServerThreadData {
    const char* ssh_host;
    const char* mds_host;
} SERVER_THREAD_DATA;

static void* server_task(void* ptr)
{
    SERVER_THREAD_DATA* data = (SERVER_THREAD_DATA*)ptr;
    ssh_run_server(data->ssh_host, data->mds_host);
    return NULL;
}

int mds_get(const char* experiment, const char* signalName, int shot, float** time, float** data, int* len, int time_dim)
{
    static int socket = -1;

    if (socket == -1) {

        char host[100];

        if (StringIEquals(experiment, "TCV") || StringIEquals(experiment, "ASDEX")) {
            g_server_port = 0;
            g_initialised = false;

            pthread_cond_init(&g_initialised_cond, NULL);
            pthread_mutex_init(&g_initialised_mutex, NULL);

            pthread_t server_thread;
            SERVER_THREAD_DATA thread_data = {};

            if (StringIEquals(experiment, "TCV")) {
                thread_data.ssh_host = "lac911.epfl.ch";
                thread_data.mds_host = "tcvdata.epfl.ch";
            } else if (StringIEquals(experiment, "ASDEX")) {
                thread_data.ssh_host = "gate1.aug.ipp.mpg.de";
                thread_data.mds_host = "mdsplus.aug.ipp.mpg.de";
            }

            pthread_create(&server_thread, NULL, server_task, &thread_data);

            pthread_mutex_lock(&g_initialised_mutex);
            while (!g_initialised) {
                pthread_cond_wait(&g_initialised_cond, &g_initialised_mutex);
            }
            pthread_mutex_unlock(&g_initialised_mutex);

            struct timespec sleep_for;
            sleep_for.tv_sec = 0;
            sleep_for.tv_nsec = 100000000;
            nanosleep(&sleep_for, NULL);

            sprintf(host, "localhost:%d", g_server_port);
        } else {
            strcpy(host, "mdsplus.jet.efda.org:8000");
        }

//        const char* host = getenv("UDA_EXP2IMAS_MDSPLUS_FORWARD_PORT");

//        if (host == NULL || host[0] == '\0') {
//
//        }

        /* Connect to MDSplus */
        socket = MdsConnect((char*)host);
        if (socket == -1) {
            UDA_LOG(UDA_LOG_ERROR, "Error connecting to %s.\n", host);
            return -1;
        }
    }

    if (StringIEquals(experiment, "TCV")) {
        const char* tree = "tcv_shot";

        int status = MdsOpen((char*)tree, &shot);
        if (((status & 1) != 1)) {
            UDA_LOG(UDA_LOG_ERROR, "Error opening tree for shot %d: %s.\n", shot, MdsGetMsg(status));
            return -1;
        }
    }

    bool is_tdi = false;
    if (STR_STARTSWITH(signalName, "%TDI%")) {
        signalName = &signalName[5];
        is_tdi = true;
    }

    char signal[2048];

    char* shot_pos = strstr(signalName, "%SHOT%");
    if (shot_pos != NULL) {
        size_t offset = shot_pos - signalName;
        strncpy(signal, signalName, offset);
        size_t l = sprintf(signal + offset, "%d", shot);
        strcpy(signal + offset + l, shot_pos + 6);
    } else if (is_tdi) {
        sprintf(signal, "%s", signalName);
    } else if (StringIEquals(experiment, "JET")) {
        sprintf(signal, "_sig=jet(\"%s\",%d)", signalName, shot);
    } else {
        sprintf(signal, "_sig=%s", signalName);
    }

    *len = get_signal_length(signal);

    if (*len < 0) {
        UDA_LOG(UDA_LOG_ERROR, "Unable to get signal length.\n");
        return -1;
    }

    *time = malloc(*len * sizeof(float));
    *data = malloc(*len * sizeof(float));

    int null = 0;
    int dtype_float = DTYPE_FLOAT;

    int fdesc = descr(&dtype_float, *time, len, &null);
    int rlen = 0;

    char buf[2048];

    //Get time data
    sprintf(buf, "%s; dim_of(_sig, %d);", signal, time_dim - 1);

    int status = MdsValue(buf, &fdesc, &null, &rlen, NULL);
    if (((status & 1) != 1)) {
        UDA_LOG(UDA_LOG_ERROR, "Unable to get signal.\n");
        return -1;
    }

    fdesc = descr(&dtype_float, *data, len, &null);

    //Get data
    sprintf(buf, "%s; _sig;", signal);

    status = MdsValue(buf, &fdesc, &null, &rlen, NULL);
    if (((status & 1) != 1)) {
        UDA_LOG(UDA_LOG_ERROR, "Unable to get signal.\n");
        return -1;
    }

//    MdsDisconnect();

    return 0;
}

