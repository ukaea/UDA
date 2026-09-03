---
layout: default
title: Authentication
nav_order: 4
---

# Authenticated and encrypted connections in UDA
{:.no_toc}

UDA supports three transport security modes:

| Mode | Purpose | Client certificate required |
| --- | --- | --- |
| `off` | Plain TCP. No TLS encryption or certificate authentication. | No |
| `server` | Server-only TLS. The server presents a certificate and the client verifies it against a CA bundle. | No |
| `mutual` | Mutual TLS. The server and client both present certificates. This is the legacy X.509 authentication behaviour. | Yes |

Server-only TLS is the recommended mode when using OAuth/JWT authentication because it protects the token in transit without requiring every client to have an X.509 client certificate. Mutual TLS remains available for deployments that use certificate-based client identity.

## Contents
{:.no_toc}
1. TOC
{:toc}

## TLS mode selection

Set the TLS mode independently on the server and client:

```sh
export UDA_SERVER_TLS_MODE=off      # off | server | mutual
export UDA_CLIENT_TLS_MODE=off      # off | server | mutual
```

The legacy SSL environment variables are still supported:

```sh
export UDA_SERVER_SSL_AUTHENTICATE=1
export UDA_CLIENT_SSL_AUTHENTICATE=1
```

Legacy `UDA_*_SSL_AUTHENTICATE=1` maps to `mutual`. The new `UDA_*_TLS_MODE` variables take precedence when both are set.

For client compatibility, a host configured with the `SSL://` prefix or marked as SSL in the client host configuration still enables the legacy mutual TLS behaviour unless `UDA_CLIENT_TLS_MODE` is explicitly set.

## Certificate environment variables

The preferred variable names use `TLS`. Existing `SSL` names remain valid fallbacks.

| Purpose | Preferred variable | Legacy fallback |
| --- | --- | --- |
| Server certificate | `UDA_SERVER_TLS_CERT` | `UDA_SERVER_SSL_CERT` |
| Server private key | `UDA_SERVER_TLS_KEY` | `UDA_SERVER_SSL_KEY` |
| Server CA for client cert verification | `UDA_SERVER_CA_TLS_CERT` | `UDA_SERVER_CA_SSL_CERT` |
| Server CRL for client cert verification | `UDA_SERVER_CA_TLS_CRL` | `UDA_SERVER_CA_SSL_CRL` |
| Client certificate | `UDA_CLIENT_TLS_CERT` | `UDA_CLIENT_SSL_CERT` |
| Client private key | `UDA_CLIENT_TLS_KEY` | `UDA_CLIENT_SSL_KEY` |
| Client CA for server cert verification | `UDA_CLIENT_CA_TLS_CERT` | `UDA_CLIENT_CA_SSL_CERT` |

`UDA_SERVER_TLS_CA_CERT`, `UDA_SERVER_TLS_CA_CRL`, and `UDA_CLIENT_TLS_CA_CERT` are also accepted aliases.

## Plain TCP

Plain TCP is the default when no TLS mode or legacy SSL authentication variable is set.

```sh
export UDA_SERVER_TLS_MODE=off
export UDA_CLIENT_TLS_MODE=off
```

Use this only on trusted local networks or for testing. OAuth tokens must not be sent over plain TCP.

## Server-only TLS

Server-only TLS encrypts the UDA transport. The server presents a certificate and the client validates it using a CA certificate. The client does not present a certificate.

Server setup:

```sh
export UDA_SERVER_TLS_MODE=server
export UDA_SERVER_TLS_CERT="${UDA_ROOT}/etc/.uda/certs/<server_address>.cert.pem"
export UDA_SERVER_TLS_KEY="${UDA_ROOT}/etc/.uda/keys/<server_address>.key.pem"
```

Client setup:

```sh
export UDA_CLIENT_TLS_MODE=server
export UDA_CLIENT_CA_TLS_CERT="${SSL_HOME}/certs/uda-ca.cert.pem"
```

This is the usual setup for OAuth/JWT because the JWT is protected by TLS and the client identity comes from the OAuth token rather than an X.509 certificate.

## Mutual TLS

Mutual TLS is the existing X.509 authentication mode. It requires a server certificate and key, a client certificate and key, and CA certificates on both sides.

Server setup:

```sh
export UDA_SERVER_TLS_MODE=mutual
export UDA_SERVER_TLS_CERT="${UDA_ROOT}/etc/.uda/certs/<server_address>.cert.pem"
export UDA_SERVER_TLS_KEY="${UDA_ROOT}/etc/.uda/keys/<server_address>.key.pem"
export UDA_SERVER_CA_TLS_CERT="${UDA_ROOT}/etc/.uda/certs/client-ca.cert.pem"
export UDA_SERVER_CA_TLS_CRL="${UDA_ROOT}/etc/.uda/crl/client-ca.crl.pem"
```

Client setup:

```sh
export UDA_CLIENT_TLS_MODE=mutual
export UDA_CLIENT_TLS_CERT="${SSL_HOME}/certs/<username>.cert.pem"
export UDA_CLIENT_TLS_KEY="${SSL_HOME}/keys/<username>.key.pem"
export UDA_CLIENT_CA_TLS_CERT="${SSL_HOME}/certs/server-ca.cert.pem"
```

Equivalent legacy setup:

```sh
export UDA_SERVER_SSL_AUTHENTICATE=1
export UDA_SERVER_SSL_CERT="${UDA_ROOT}/etc/.uda/certs/<server_address>.cert.pem"
export UDA_SERVER_SSL_KEY="${UDA_ROOT}/etc/.uda/keys/<server_address>.key.pem"
export UDA_SERVER_CA_SSL_CERT="${UDA_ROOT}/etc/.uda/certs/client-ca.cert.pem"

export UDA_CLIENT_SSL_AUTHENTICATE=1
export UDA_CLIENT_SSL_CERT="${SSL_HOME}/certs/<username>.cert.pem"
export UDA_CLIENT_SSL_KEY="${SSL_HOME}/keys/<username>.key.pem"
export UDA_CLIENT_CA_SSL_CERT="${SSL_HOME}/certs/server-ca.cert.pem"
```

## OAuth/JWT authentication

OAuth authentication is enabled separately from TLS. TLS protects the connection; OAuth authenticates the request using a JWT supplied by the client.

Server setup:

```sh
export UDA_SERVER_AUTHENTICATION=OAUTH
export UDA_SERVER_KEYCLOAK_REALM="https://<keycloak-host>/realms/<realm>"
export UDA_SERVER_KEYCLOAK_CLIENT_ID="<client-id>"
```

Client setup:

```sh
export UDA_AUTH_TOKEN="<access-token>"
```

Recommended combined setup:

```sh
# Server
export UDA_SERVER_TLS_MODE=server
export UDA_SERVER_TLS_CERT="${UDA_ROOT}/etc/.uda/certs/<server_address>.cert.pem"
export UDA_SERVER_TLS_KEY="${UDA_ROOT}/etc/.uda/keys/<server_address>.key.pem"
export UDA_SERVER_AUTHENTICATION=OAUTH
export UDA_SERVER_KEYCLOAK_REALM="https://<keycloak-host>/realms/<realm>"
export UDA_SERVER_KEYCLOAK_CLIENT_ID="<client-id>"

# Client
export UDA_CLIENT_TLS_MODE=server
export UDA_CLIENT_CA_TLS_CERT="${SSL_HOME}/certs/uda-ca.cert.pem"
export UDA_AUTH_TOKEN="<access-token>"
```

The server verifies the JWT signature using the realm JWKS, checks the issuer, and checks the `azp` claim against `UDA_SERVER_KEYCLOAK_CLIENT_ID`. The token payload is then made available to server/plugin code for authorization decisions.

## Current limitations

The current TLS client verifies the server certificate chain and validity dates, but it does not perform hostname verification against the server name. Use a tightly controlled CA and avoid sharing a CA across unrelated services until hostname verification is implemented.

OAuth token acquisition and refresh are not implemented in UDA. Clients must obtain an access token externally and provide it via `UDA_AUTH_TOKEN`.

OAuth currently supports `UDA_SERVER_AUTHENTICATION=OAUTH` only. The value is case-sensitive.

OAuth tokens are sent in the UDA client block. Use `UDA_*_TLS_MODE=server` or `mutual`; otherwise the token can be exposed on the network.

The OAuth implementation fetches OpenID configuration and JWKS using libcurl. UDA does not currently enforce that the realm URL is HTTPS or add certificate pinning; it relies on the URL and libcurl/TLS configuration provided by the deployment.

Core UDA authenticates the token and exposes its claims. Fine-grained authorization based on scopes, roles, or claims is still the responsibility of server/plugin code.

OAuth authentication requires protocol support for the authentication block. Older clients that do not send the authentication block cannot authenticate with an OAuth-enabled server.
