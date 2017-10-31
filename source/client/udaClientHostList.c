/*---------------------------------------------------------------
* Identify the Server Host Attributes
*---------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>

#include <client/udaClientHostList.h>
#include <clientserver/stringUtils.h>
#include <logging/logging.h>

static HOSTLIST hostlist;
static int hostId = -1;

HOSTLIST* udaClientGetHostList()
{
    return &hostlist;
}

void udaClientPutHostNameId(int id)
{
    hostId = id;
}

int udaClientGetHostNameId()
{
    return hostId;
}

void udaClientAllocHostList(int count)
{
    HOSTLIST* list = udaClientGetHostList();
    if (count >= list->mcount) {
        list->mcount = list->mcount + HOST_MSTEP;
        list->hosts = (HOSTDATA*)realloc((void*)list->hosts, list->mcount * sizeof(HOSTDATA));
    }
}

void udaClientFreeHostList()
{
    HOSTLIST* list = udaClientGetHostList();
    free((void*)list->hosts);
    list->hosts = NULL;
    list->count = 0;
    list->mcount = 0;
}

void udaClientInitHostData(HOSTDATA* host)
{
    host->hostalias[0] = '\0';
    host->hostname[0] = '\0';
    host->port = 0;
    host->certificate[0] = '\0';
    host->key[0] = '\0';
    host->ca_certificate[0] = '\0';
}

int udaClientFindHostByAlias(const char* alias)
{
    udaClientInitHostList();

    int i;
    HOSTLIST* list = udaClientGetHostList();
    for (i = 0; i < list->count; i++) {
        if (STR_IEQUALS(list->hosts[i].hostalias, alias)) {
            return i;
        }
    }
    return -1;
}

int udaClientFindHostByName(const char* name)
{
    udaClientInitHostList();

    int i;
    HOSTLIST* list = udaClientGetHostList();
    for (i = 0; i < list->count; i++) {
        if (STR_IEQUALS(list->hosts[i].hostname, name)) {
            return i;
        }
    }
    return -1;
}

char* udaClientGetHostName(int id)
{
    HOSTLIST* list = udaClientGetHostList();
    if (id >= 0 && id < list->count) {
        return list->hosts[id].hostname;
    } else {
        return NULL;
    }
}

char* udaClientGetHostAlias(int id)
{
    HOSTLIST* list = udaClientGetHostList();
    if (id >= 0 && id < list->count) {
        return list->hosts[id].hostalias;
    } else {
        return NULL;
    }
}

int udaClientGetHostPort(int id)
{
    HOSTLIST* list = udaClientGetHostList();
    if (id >= 0 && id < list->count) {
        return list->hosts[id].port;
    } else {
        return -1;
    }
}

char* udaClientGetHostCertificatePath(int id)
{
    HOSTLIST* list = udaClientGetHostList();
    if (id >= 0 && id < list->count) {
        return list->hosts[id].certificate;
    } else {
        return NULL;
    }
}

char* udaClientGetHostKeyPath(int id)
{
    HOSTLIST* list = udaClientGetHostList();
    if (id >= 0 && id < list->count) {
        return list->hosts[id].key;
    } else {
        return NULL;
    }
}

char* udaClientGetHostCAPath(int id)
{
    HOSTLIST* list = udaClientGetHostList();
    if (id >= 0 && id < list->count) {
        return list->hosts[id].ca_certificate;
    } else {
        return NULL;
    }
}

void udaClientInitHostList()
{

    static int hostListInitialised = 0;

    if (hostListInitialised) return;
    hostListInitialised = 1;

    int i;
    HOSTLIST* list = udaClientGetHostList();

    // initialise the Plugin List and Allocate heap for the list

    list->count = 0;
    list->hosts = (HOSTDATA*)malloc(HOST_MCOUNT * sizeof(HOSTDATA));
    list->mcount = HOST_MCOUNT;

    for (i = 0; i < list->mcount; i++) {
        udaClientInitHostData(&list->hosts[i]);
    }

    //----------------------------------------------------------------------------------------------------------------------
    // Read the host configuration file: No error if the file does not exist

    int lstr;
    char buffer[HOST_STRING];
    char* config = getenv("UDA_CLIENT_HOSTS_CONFIG");    // Host configuration file
    FILE* conf = NULL;
    char* filename = "hosts.cfg";                // Default name
    char* work = NULL, * next, * split;

    // Locate the hosts registration file

    if (config == NULL) {
        lstr = (int)strlen(filename) + 9;
        work = (char*)malloc(lstr * sizeof(char));
#ifdef _WIN32
        sprintf(work, "%s", filename);			// Local directory
#else
        sprintf(work, "~/.uda/%s", filename);        // the UDA hidden directory in the user's home directory
#endif
    } else {
        lstr = (int)strlen(config) + 1;
        work = (char*)malloc(lstr * sizeof(char));            // Alternative File Name and Path
        strcpy(work, config);
    }

    // Read the hosts file

    errno = 0;
    if ((conf = fopen(work, "r")) == NULL || errno != 0) {
        if (conf != NULL) fclose(conf);
        free((void*)work);
        return;
    }

    if (work != NULL) free((void*)work);

    // organisation: sets of 1-6 records, empty records ignored, comment begins #
    // hostName must be the first record in a set
    // hostAlias and other attributes are not required
    // ordering is not important

    int newHost = 0;

    while (fgets(buffer, HOST_STRING, conf) != NULL) {
        convertNonPrintable2(buffer);                // convert non printable chars to spaces
        LeftTrimString(TrimString(buffer));                // remove leading and trailing spaces
        do {
            if (buffer[0] == '#') break;
            if (strlen(buffer) == 0) break;

            next = buffer;
            split = strchr(next, ' ');                // Split the string on the first space character
            if (split != NULL) split[0] = '\0';            // Extract the attribute name
            LeftTrimString(TrimString(next));

            if (!strcasecmp(next, "hostName")) {            // Trigger a new set of attributes
                newHost = 0;
                udaClientInitHostData(&list->hosts[list->count]);        // New block of attributes
                next = &split[1];
                LeftTrimString(TrimString(next));
                if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                    udaClientAllocHostList(list->count);
                    strcpy(list->hosts[list->count++].hostname, next);
                    newHost = 1;
                }
            } else if (newHost && !strcasecmp(next, "hostAlias")) {
                next = &split[1];
                LeftTrimString(TrimString(next));
                if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                    strcpy(list->hosts[list->count - 1].hostalias, next);
                }
            } else if (newHost && !strcasecmp(next, "port")) {
                next = &split[1];
                LeftTrimString(TrimString(next));
                if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                    list->hosts[list->count - 1].port = atoi(next);
                }
            } else if (newHost && !strcasecmp(next, "certificate")) {
                next = &split[1];
                LeftTrimString(TrimString(next));
                if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                    strcpy(list->hosts[list->count - 1].certificate, next);
                }
            } else if (newHost && !strcasecmp(next, "privateKey")) {
                next = &split[1];
                LeftTrimString(TrimString(next));
                if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                    strcpy(list->hosts[list->count - 1].key, next);
                }
            } else if (newHost && !strcasecmp(next, "CA-Certificate")) {
                next = &split[1];
                LeftTrimString(TrimString(next));
                if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                    strcpy(list->hosts[list->count - 1].ca_certificate, next);
                }
            }

        } while (0);
    }

    fclose(conf);

    UDA_LOG(UDA_LOG_DEBUG, "idamClientHostList: Number of named hosts %d\n", list->count);
    for (i = 0; i < list->count; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "idamClientHostList: [%d] Host Alias     : %s\n", i, list->hosts[i].hostalias);
        UDA_LOG(UDA_LOG_DEBUG, "idamClientHostList: [%d] Host Name      : %s\n", i, list->hosts[i].hostname);
        UDA_LOG(UDA_LOG_DEBUG, "idamClientHostList: [%d] Host Port      : %d\n", i, list->hosts[i].port);
        UDA_LOG(UDA_LOG_DEBUG, "idamClientHostList: [%d] Certificate    : %s\n", i, list->hosts[i].certificate);
        UDA_LOG(UDA_LOG_DEBUG, "idamClientHostList: [%d] Key            : %s\n", i, list->hosts[i].key);
        UDA_LOG(UDA_LOG_DEBUG, "idamClientHostList: [%d] CA Certificate : %s\n", i, list->hosts[i].ca_certificate);
    }
}
