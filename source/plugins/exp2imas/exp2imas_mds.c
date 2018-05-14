#include "exp2imas_mds.h"

#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <mdslib.h>
#include <mdsshr.h>

#include <logging/logging.h>
#include <clientserver/stringUtils.h>
#include <plugins/udaPlugin.h>

#include "exp2imas_ramCache.h"
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
    const char* experiment;
    const char* ssh_host;
    const char* mds_host;
} SERVER_THREAD_DATA;

static void* server_task(void* ptr)
{
    SERVER_THREAD_DATA* data = (SERVER_THREAD_DATA*)ptr;
    ssh_run_server(data->experiment, data->ssh_host, data->mds_host);
    return NULL;
}

int mds_get(const char* experiment, const char* signalName, int shot, float** time, float** data, int* len, int time_dim)
{
    static int socket = -1;

    if (socket == -1) {

        char host[100];

        if (StringIEquals(experiment, "TCV") || StringIEquals(experiment, "AUG")) {
            g_server_port = 0;
            g_initialised = false;

            pthread_cond_init(&g_initialised_cond, NULL);
            pthread_mutex_init(&g_initialised_mutex, NULL);

            pthread_t server_thread;
            SERVER_THREAD_DATA thread_data = {};
            thread_data.experiment = experiment;

            if (StringIEquals(experiment, "TCV")) {
                thread_data.ssh_host = "lac911.epfl.ch";
                thread_data.mds_host = "tcvdata.epfl.ch";
            } else if (StringIEquals(experiment, "AUG")) {
                thread_data.ssh_host = "gate2.aug.ipp.mpg.de";
                thread_data.mds_host = "mdsplus.aug.ipp.mpg.de";
            }

            pthread_create(&server_thread, NULL, server_task, &thread_data);

            pthread_mutex_lock(&g_initialised_mutex);
            while (!g_initialised) {
                pthread_cond_wait(&g_initialised_cond, &g_initialised_mutex);
            }
            pthread_mutex_unlock(&g_initialised_mutex);

            pthread_mutex_destroy(&g_initialised_mutex);
            pthread_cond_destroy(&g_initialised_cond);

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

    char work[2048];

    regex_t re;
    int r = regcomp(&re, "([A-Z]+)\\|([A-Za-z0-9]+)", REG_EXTENDED);
    if (r) {
        fprintf(stderr, "regex failed to compile\n");
        RAISE_PLUGIN_ERROR("regex failed to compile");
    }

    regmatch_t matches[3];
    r = regexec(&re, signalName, 3, matches, 0);

    bool is_tdi = false;

    if (r == 0) {
        char* tmp = strdup(signalName);
        tmp[matches[1].rm_eo] = '\0';
        sprintf(work, "_sig=augdiag(%%SHOT%%,\"%s\",\"%s\")", tmp, &tmp[matches[2].rm_so]);
        free(tmp);
    } else if (STR_STARTSWITH(signalName, "%TDI%")) {
        char* shot_pos = strstr(signalName, "%SHOT%");
        if (shot_pos != NULL) {
            char shot_str[20];
            sprintf(shot_str, "%d", shot);
            char* tmp = StringReplaceAll(signalName, "%SHOT%", shot_str);
            sprintf(work, "%s", &tmp[5]);
        } else {
            sprintf(work, "%s", &signalName[5]);
        }
        is_tdi = true;
    } else {
        sprintf(work, "%s", signalName);
    }

    char signal[2048];

    char* shot_pos = strstr(work, "%SHOT%");
    if (shot_pos != NULL) {
        char shot_str[10];
        sprintf(shot_str, "%d", shot);
        char* tmp = StringReplaceAll(work, "%SHOT%", shot_str);
        sprintf(signal, "%s", tmp);
        free(tmp);
    } else if (is_tdi) {
        sprintf(signal, "%s", work);
    } else if (StringIEquals(experiment, "JET")) {
        sprintf(signal, "_sig=jet(\"%s\",%d)", work, shot);
    } else {
        sprintf(signal, "_sig=%s", work);
    }

    fprintf(stderr, "fetching signal %s", signal);

    static RAM_CACHE* cache = NULL;
    if (cache == NULL) {
        cache = ram_cache_new(100);
    }

    char len_key[2048];
    char time_key[2048];
    char data_key[2048];

    sprintf(len_key, "%s/length", signal);
    sprintf(time_key, "%s/time", signal);
    sprintf(data_key, "%s/data", signal);

    int* cache_len = (int*)ram_cache_get(cache, len_key);
    if (cache_len != NULL) {
        *len = *cache_len;
        *time = (float*)malloc(*len * sizeof(float));
        *data = (float*)malloc(*len * sizeof(float));
        memcpy(*time, (int*)ram_cache_get(cache, time_key), *len * sizeof(float));
        memcpy(*data, (int*)ram_cache_get(cache, data_key), *len * sizeof(float));
        fprintf(stderr, " -> from cache\n");
        return 0;
    }

    if (StringIEquals(experiment, "TCV")) {
        const char* tree = "tcv_shot";

        int status = MdsOpen((char*)tree, &shot);
        if (((status & 1) != 1)) {
            UDA_LOG(UDA_LOG_ERROR, "Error opening tree for shot %d: %s.\n", shot, MdsGetMsg(status));
            return -1;
        }
    }

    *len = get_signal_length(signal);

    if (*len < 0) {
        fprintf(stderr, " -> unable to get signal length.\n");
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
        fprintf(stderr, " -> unable to get signal\n");
        UDA_LOG(UDA_LOG_ERROR, "Unable to get signal.\n");
        return -1;
    }

    fdesc = descr(&dtype_float, *data, len, &null);

    //Get data
    sprintf(buf, "%s; _sig;", signal);

    status = MdsValue(buf, &fdesc, &null, &rlen, NULL);
    if (((status & 1) != 1)) {
        fprintf(stderr, " -> unable to get signal\n");
        UDA_LOG(UDA_LOG_ERROR, "Unable to get signal.\n");
        return -1;
    }

    fprintf(stderr, " -> from mdsplus\n");

    ram_cache_add(cache, len_key, len, sizeof(int));
    ram_cache_add(cache, time_key, *time, *len * sizeof(float));
    ram_cache_add(cache, data_key, *data, *len * sizeof(float));

//    MdsDisconnect();

    return 0;
}


