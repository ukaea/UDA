#include <libssh/libssh.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <clientserver/stringUtils.h>

#include "exp2imas_ssh_gui.h"
#include "exp2imas_ssh.h"

#ifndef HAVE_GTK3
#  include <clientserver/stringUtils.h>
#endif

static int verify_knownhost(ssh_session session)
{
    int state = ssh_is_server_known(session);

    ssh_key pubkey;
    int rc = ssh_get_publickey(session, &pubkey);
    if (rc < 0) {
        return -1;
    }

    unsigned char* hash = NULL;
    size_t hlen;
    rc = ssh_get_publickey_hash(pubkey, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);
    if (rc < 0) {
        return -1;
    }

    switch (state) {
        case SSH_SERVER_KNOWN_OK:
            break; /* ok */
        case SSH_SERVER_KNOWN_CHANGED:
            fprintf(stderr, "Host key for server changed: it is now:\n");
            ssh_print_hexa("Public key hash", hash, (size_t)hlen);
            fprintf(stderr, "For security reasons, connection will be stopped\n");
            free(hash);
            return -1;
        case SSH_SERVER_FOUND_OTHER:
            fprintf(stderr, "The host key for this server was not found but an other"
                    "type of key exists.\n");
            fprintf(stderr, "An attacker might change the default server key to"
                    "confuse your client into thinking the key does not exist\n");
            free(hash);
            return -1;
        case SSH_SERVER_FILE_NOT_FOUND:
            fprintf(stderr, "Could not find known host file.\n");
            fprintf(stderr, "If you accept the host key here, the file will be"
                    "automatically created.\n");
            /* fallback to SSH_SERVER_NOT_KNOWN behavior */
        case SSH_SERVER_NOT_KNOWN: {
            char* hexa = ssh_get_hexa(hash, (size_t)hlen);
            fprintf(stderr, "The server is unknown. Do you trust the host key?\n");
            fprintf(stderr, "Public key hash: %s\n", hexa);
            free(hexa);
            char buf[10];
            if (fgets(buf, sizeof(buf), stdin) == NULL) {
                free(hash);
                return -1;
            }
            if (strncasecmp(buf, "yes", 3) != 0) {
                free(hash);
                return -1;
            }
            if (ssh_write_knownhost(session) < 0) {
                fprintf(stderr, "Error %s\n", strerror(errno));
                free(hash);
                return -1;
            }
            break;
        }
        case SSH_SERVER_ERROR:
            fprintf(stderr, "Error %s", ssh_get_error(session));
            free(hash);
            return -1;
        default:
            fprintf(stderr, "Unknown SSH state %d", state);
    }

    free(hash);
    return 0;
}

static ssh_session create_session(const char* experiment, const char* ssh_host)
{
    ssh_session session = ssh_new();
    if (session == NULL) {
        fprintf(stderr, "failed to create SSH session\n");
        return NULL;
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, ssh_host);

    int rc = ssh_connect(session);
    if (rc != SSH_OK) {
        fprintf(stderr, "failed to connect to %s\n", ssh_host);
        ssh_free(session);
        return NULL;
    }

    fprintf(stdout, "connected to %s\n", ssh_host);

    // Verify the server's identity
    // For the source code of verify_knowhost(), check previous example
    if (verify_knownhost(session) < 0) {
        ssh_disconnect(session);
        ssh_free(session);
        return NULL;
    }

    char* username = NULL; // itmmds
    char* password = NULL;

    char* home = getenv("HOME");
    if (home != NULL) {
        char fname[1024];
        sprintf(fname, "%s/.exp2imas", home);
        if (access(fname, F_OK) != -1) {
            FILE* fid = fopen(fname, "r");
            char exp[1024];
            char user[1024];
            char pass[1024];
            while (fscanf(fid, "%s %s %s", exp, user, pass) == 3) {
                if (StringEquals(exp, experiment)) {
                    username = strdup(user);
                    password = strdup(pass);
                    break;
                }
            }
            fclose(fid);
        }
    }

#ifdef HAVE_GTK3
    if (username == NULL || password == NULL) {
        rc = ssh_open_dialog(&username, &password);
        if (rc < 0) {
            fprintf(stderr, "Failed to get authentication details");
            return NULL;
        }
    }
#else
    fprintf(stdout, "username: ");
    size_t len = 0;
    getline(&username, &len, stdin);
    username = TrimString(username);

    password = getpass("password: ");
#endif

    // Authenticate ourselves
    rc = ssh_userauth_password(session, username, password);

    free(username);
#ifdef HAVE_GTK3
    free(password);
#else
    memset(password, '\0', strlen(password));
#endif

    if (rc != SSH_AUTH_SUCCESS) {
        fprintf(stderr, "Error authenticating with password: %s\n", ssh_get_error(session));
        ssh_disconnect(session);
        ssh_free(session);
        return NULL;
    }

    return session;
}

static ssh_channel create_forwarding_channel(ssh_session session, const char* remote_host, int32_t client_port)
{
    ssh_channel forwarding_channel = ssh_channel_new(session);
    if (forwarding_channel == NULL) {
        fprintf(stderr, "failed to create SSH channel\n");
        return NULL;
    }

    int rc = ssh_channel_open_forward(forwarding_channel, remote_host, 8000, "localhost", client_port);
    if (rc != SSH_OK) {
        fprintf(stderr, "failed to open SSH channel\n");
        ssh_channel_free(forwarding_channel);
        ssh_free(session);
        return NULL;
    }

    fprintf(stdout, "forwarding %s:%d to %s:%d\n", "localhost", client_port, remote_host, 8000);

    return forwarding_channel;
}

static socket_t listen_for_client(int32_t* client_port)
{
    // create socket
    socket_t sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "failed to create socket\n");
        return -1;
    }

    struct sockaddr_in serv_addr;
    int port = 5000;

    while (true) {
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);

        if (bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
            break;
        }

        ++port;

        if (port > 6000) {
            fprintf(stderr, "too many ports tried\n");
            return -1;
        }
    }

    int rc = listen(sock, 10);
    if (rc != 0) {
        fprintf(stderr, "failed to listen\n");
        return rc;
    }

    struct sockaddr_in client_addr;
    socklen_t client_name_len = sizeof(client_addr);

    fprintf(stdout, "listening on port %d\n", port);

    pthread_mutex_lock(&g_initialised_mutex);

    g_server_port = port;
    g_initialised = true;
    pthread_cond_signal(&g_initialised_cond);

    pthread_mutex_unlock(&g_initialised_mutex);

    socket_t client_sock = accept(sock, (struct sockaddr*)&client_addr, &client_name_len);
    if (client_sock == -1) {
        perror("Error on accept");
        return SSH_ERROR;
    }

    char buf[100];
    inet_net_ntop(AF_INET, &client_addr.sin_addr, 32, buf, sizeof(buf));
    *client_port = ntohs(client_addr.sin_port);

    return client_sock;
}

int ssh_run_server(const char* experiment, const char* ssh_host, const char* remote_host)
{
    int32_t client_port;
    socket_t client_sock = listen_for_client(&client_port);
    if (client_sock < 0) {
        return -1;
    }

    fcntl(client_sock, F_SETFL, O_NONBLOCK);

    ssh_session session = create_session(experiment, ssh_host);
    if (session == NULL) {
        return -1;
    }

    ssh_channel forwarding_channel = create_forwarding_channel(session, remote_host, client_port);
    if (forwarding_channel == NULL) {
        return -1;
    }

    fprintf(stderr, "channel created, listening\n");
    fflush(stderr);

    while (!ssh_channel_is_eof(forwarding_channel)) {
        ssize_t size_recv;
        uint8_t data[4096];

        if ((size_recv = recv(client_sock, data, sizeof(data), MSG_DONTWAIT)) < 0) {
            int nread;
            if ((nread = ssh_channel_read_nonblocking(forwarding_channel, data, sizeof data, 0)) > 0) {
                if (write(client_sock, data, (size_t)nread) < 0) {
                    perror("Error writing to socket");
                    close(client_sock);
                    ssh_channel_free(forwarding_channel);
                    return SSH_ERROR;
                }
            }
        } else if (!size_recv) {
            fprintf(stderr, "Local client disconnected, SSH forwarding server exiting\n");
            break;
        }

        int nwritten = ssh_channel_write(forwarding_channel, data, (uint32_t)size_recv);
        if (size_recv != nwritten) {
            ssh_channel_free(forwarding_channel);
            return SSH_ERROR;
        }
    }

    return 0;
}

