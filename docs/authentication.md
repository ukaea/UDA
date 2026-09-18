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

TLS 1.2 is the minimum protocol version on both the client and the server; TLS 1.0, TLS
1.1 and the SSL versions cannot be negotiated. When a CRL is configured the whole chain is
checked, not just the leaf certificate, and an expired CRL is reported at startup rather
than surfacing as a blanket client-certificate failure.

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

## OIDC / JWT authentication

OIDC bearer-token authentication is enabled separately from TLS. TLS protects the
connection; OIDC authenticates the request using a JWT the client supplies.

The server verifies the token signature against the issuer's JWKS, checks the issuer,
audience and lifetime, applies the configured claim policy, and makes the verified claims
available to plugins. It does this locally — it does not call the issuer's introspection
endpoint per request.

### Enabling it

Build with `-DENABLE_AUTH=ON` (or `-DENABLE_OIDC_AUTH=ON`), then on the server:

```sh
export UDA_SERVER_AUTHENTICATION=OIDC     # OIDC or OAUTH; both select this path
export UDA_SERVER_OIDC_ISSUER="https://<idp-host>/realms/<realm>"
export UDA_SERVER_OIDC_AUDIENCE="uda"
```

and on the client:

```sh
export UDA_AUTH_TOKEN="<access-token>"
```

UDA does not acquire or refresh tokens. Clients obtain an access token externally and
provide it in `UDA_AUTH_TOKEN`.

### Server configuration reference

| Variable | Default | Meaning |
| --- | --- | --- |
| `UDA_SERVER_AUTHENTICATION` | unset | `OIDC` or `OAUTH` enables bearer-token authentication |
| `UDA_SERVER_OIDC_ISSUER` | — | Issuer URL. Discovery is fetched from `<issuer>/.well-known/openid-configuration` |
| `UDA_SERVER_OIDC_JWKS_URI` | — | Explicit JWKS endpoint. Set this to skip discovery entirely |
| `UDA_SERVER_OIDC_AUDIENCE` | — | Required `aud` value. Setting it enables audience verification |
| `UDA_SERVER_OIDC_CLIENT_ID` | — | Client id, used by the legacy `azp` policy |
| `UDA_SERVER_OIDC_REQUIRED_CLAIMS` | — | Claim policy rules — see below |
| `UDA_SERVER_OIDC_ALLOWED_ALGS` | `RS256` | Comma-separated. `RS256/384/512` and `ES256/384/512` are supported |
| `UDA_SERVER_OIDC_VERIFY_ISSUER` | `1` | Verify the `iss` claim |
| `UDA_SERVER_OIDC_VERIFY_AUDIENCE` | `1` when an audience is set | Verify the `aud` claim |
| `UDA_SERVER_OIDC_CLOCK_SKEW_SECONDS` | `60` | Leeway applied to `exp` and `nbf` |
| `UDA_SERVER_OIDC_JWKS_CACHE_TTL` | `300` | JWKS cache lifetime, seconds |
| `UDA_SERVER_OIDC_POLICY` | unset | `none` accepts any valid token from the issuer — see below |
| `UDA_SERVER_OIDC_ALLOW_HTTP` | unset | Permit non-HTTPS discovery/JWKS URLs. Testing only |
| `UDA_ALLOW_TOKEN_WITHOUT_TLS` | unset | Silence the warning about tokens on unencrypted connections |

`exp` and `nbf` are always validated when present; there is no option to disable that.
Clock skew tolerance is configurable, the checks themselves are not.

The algorithm is pinned by `UDA_SERVER_OIDC_ALLOWED_ALGS`, not taken from the token's own
`alg` header. A token whose header asks for an algorithm outside the whitelist is
rejected, including an unsigned (`alg: none`) token and one signed with HMAC using the
issuer's public key.

Legacy aliases, still accepted: `UDA_SERVER_KEYCLOAK_REALM` for
`UDA_SERVER_OIDC_ISSUER`, and `UDA_SERVER_KEYCLOAK_CLIENT_ID` for
`UDA_SERVER_OIDC_CLIENT_ID`. When configured this way the server applies the legacy
policy of requiring `azp` to equal the client id.

### A claim policy is required

The server refuses to start authenticating unless at least one of `UDA_SERVER_OIDC_AUDIENCE`,
`UDA_SERVER_OIDC_REQUIRED_CLAIMS`, or the legacy client-id policy is configured. Without
one, any valid token from the issuer would be accepted — including tokens issued for a
completely different application.

To accept any valid token deliberately, set `UDA_SERVER_OIDC_POLICY=none`. The server
logs a warning on every connection when this is in effect. It is not suitable for
production.

### Claim policy syntax

`UDA_SERVER_OIDC_REQUIRED_CLAIMS` is a semicolon-separated list of rules, each
`<path>:<op>` or `<path>:<op>:<value>`:

| Op | Meaning |
| --- | --- |
| `exists` | the claim is present, with any value |
| `equals` | exact string match |
| `contains` | element of a JSON array claim, or substring of a string claim |
| `contains_word` | whitespace-delimited token — the right op for `scope` |
| `contains_any` | any of a comma-separated list of values |

Claim paths navigate nested claims and index arrays:

```text
preferred_username                     top-level claim
realm_access.roles                     nested object
realm_access.roles[0]                  array element
resource_access.uda-client.roles[1]    deeper nesting
wlcg\.groups                           a claim name containing a literal dot
```

Example:

```sh
export UDA_SERVER_OIDC_REQUIRED_CLAIMS="scope:contains_word:uda.read;realm_access.roles:contains:uda-user"
```

The same path syntax is used by the plugin helpers `authPayloadPath()` and
`authPayloadContains()`, so a policy path and a plugin path mean the same thing.

### Recommended combined setup

```sh
# Server
export UDA_SERVER_TLS_MODE=server
export UDA_SERVER_TLS_CERT="${UDA_ROOT}/etc/.uda/certs/<server_address>.cert.pem"
export UDA_SERVER_TLS_KEY="${UDA_ROOT}/etc/.uda/keys/<server_address>.key.pem"
export UDA_SERVER_AUTHENTICATION=OIDC
export UDA_SERVER_OIDC_ISSUER="https://<idp-host>/realms/<realm>"
export UDA_SERVER_OIDC_AUDIENCE="uda"
export UDA_SERVER_OIDC_REQUIRED_CLAIMS="scope:contains_word:uda.read"

# Client
export UDA_CLIENT_TLS_MODE=server
export UDA_CLIENT_CA_TLS_CERT="${SSL_HOME}/certs/uda-ca.cert.pem"
export UDA_AUTH_TOKEN="<access-token>"
```

A bearer token sent over a `TLS_MODE=off` connection is readable by anything on the
network path. UDA does not refuse to send it — testing against a local development IdP
over plain TCP is a legitimate workflow — but both the client and the server log a
warning until `UDA_ALLOW_TOKEN_WITHOUT_TLS=1` is set.

## Error codes

Authentication and TLS failures return stable numeric codes. An OIDC failure occurs after
the client block has been received, so it reaches the client in the server block. A TLS
failure happens before the UDA protocol is established and therefore cannot: the client
sees a local TLS error, and the server's specific reason is recorded server-side in
`refused_requests.log`.

| Code | Name | Meaning and action |
| --- | --- | --- |
| 700 | `MISSING_TOKEN` | No token supplied. Set `UDA_AUTH_TOKEN`, or upgrade a pre-protocol-11 client |
| 701 | `INVALID_CONFIG` | Server OIDC configuration is wrong or has no claim policy |
| 702 | `DISCOVERY_FAILED` | The issuer's discovery document could not be fetched or parsed |
| 703 | `JWKS_FETCH` | The issuer's signing keys could not be fetched |
| 704 | `INVALID_TOKEN` | Signature invalid, malformed token, or unknown key id |
| 705 | `CLAIM_POLICY` | The token is valid but does not satisfy the claim policy |
| 706 | `TOKEN_EXPIRED` | `exp` is in the past (or `nbf` in the future). Obtain a fresh token |
| 707 | `TOKEN_BAD_ISSUER` | `iss` does not match the configured issuer |
| 708 | `TOKEN_BAD_AUDIENCE` | `aud` does not contain the configured audience |
| 710 | `TLS_CONFIG` | Server TLS configuration error — certificate, key, or CRL |
| 711 | `TLS_HANDSHAKE` | TLS handshake failed, including client certificate rejection |
| 712 | `TLS_HOSTNAME_MISMATCH` | The server certificate does not match the connected hostname |

## Log files

The server forks per connection, and its log files are opened in **append** mode so that
records from successive connections accumulate. They therefore grow without bound and a
deployment should rotate them. `UDA_LOG_MODE=w` restores the previous truncate-on-open
behaviour, in which each new connection discarded the previous connection's logs.

## Audit log: `refused_requests.log`

Every connection refused before it completes normally is recorded as one JSON object per
line in `<logdir>/refused_requests.log`. This is the record of refusals that `access.log`
(completed requests) and `auth.log` (diagnostics) do not cover, and it is the only place
the precise reason for a server-side TLS rejection appears.

The file is opened in append mode and is not affected by `UDA_LOG`. Each record is
written in a single append, so server processes handling concurrent connections cannot
interleave.

Fields that do not apply to a given stage are omitted rather than emitted empty.

| Field | Always | Meaning |
| --- | --- | --- |
| `ts` | yes | ISO 8601 UTC, milliseconds |
| `event` | yes | always `refused_request` |
| `stage` | yes | `tls_config`, `tls_handshake`, `client_block`, `oidc_auth`, `protocol` |
| `outcome` | yes | always `refused` |
| `reason_code` | yes | see below |
| `server_pid` | yes | the per-connection server process |
| `uda_error_code` | | the numeric code from the table above |
| `message` | | human-readable detail |
| `peer_ip`, `peer_port` | | the refused client |
| `client_username`, `client_version` | | from the client block, when decoded |
| `tls_mode`, `tls_version`, `tls_cipher` | | TLS stages |
| `client_cert_subject`, `client_cert_issuer`, `client_cert_serial` | | TLS stages |
| `client_cert_not_before`, `client_cert_not_after` | | TLS stages |
| `client_cert_fingerprint_sha256` | | TLS stages |
| `client_cert_verify_result` | | X509_V_* code; omitted when not applicable |
| `token_error` | | `missing`, `expired`, `bad_issuer`, `bad_audience`, `claim_policy`, `config`, `invalid` |
| `oidc_issuer` | | the issuer the server was configured with |
| `decode_error` | | client block / protocol detail |

`reason_code` vocabulary, stable across releases:

```text
TLS_SERVER_CERT_CONFIG_ERROR   TLS_CRL_LOAD_FAILED
TLS_CLIENT_CERT_EXPIRED        TLS_CLIENT_CERT_NOT_YET_VALID
TLS_CLIENT_CERT_UNTRUSTED      TLS_CLIENT_CERT_REVOKED
TLS_CLIENT_CERT_MISSING        TLS_HANDSHAKE_FAILED
OIDC_TOKEN_MISSING             OIDC_TOKEN_EXPIRED
OIDC_TOKEN_INVALID_SIGNATURE   OIDC_TOKEN_BAD_ISSUER
OIDC_TOKEN_BAD_AUDIENCE        OIDC_CLAIM_POLICY_FAILED
OIDC_CONFIG_INVALID            CLIENT_PROTOCOL_TOO_OLD
CLIENT_BLOCK_MALFORMED
```

Example — an expired client certificate rejected during a mutual-TLS handshake:

```json
{"ts":"2026-09-17T10:31:02.417Z","event":"refused_request","stage":"tls_handshake","outcome":"refused","reason_code":"TLS_CLIENT_CERT_EXPIRED","uda_error_code":711,"message":"TLS handshake failed (SSL_ERROR_SSL)","peer_ip":"192.0.2.10","peer_port":41234,"server_pid":21877,"tls_mode":"mutual","client_cert_verify_result":10,"client_cert_subject":"/CN=alice","client_cert_not_after":"2026-08-01T00:00:00Z"}
```

## Verified claims in plugins

The verified token payload is available to plugins through the plugin interface:

| Function | Use |
| --- | --- |
| `authPayloadValue(key, pi)` | exact top-level claim name |
| `authPayloadPath(path, pi)` | claim path with nesting and array indexing |
| `authPayloadContains(path, value, pi)` | membership in an array claim or a space-delimited claim |

String claims are returned as-is; arrays and objects are returned as JSON text. On plugin
invocation paths that carry no authenticated token, the payload is null and all three
return nothing rather than reading uninitialised memory.

UDA authenticates the token and exposes its claims. Authorisation — deciding what a given
subject may read — remains the responsibility of server and plugin code.

## Diagnostics

`HELP::servermetadata()` reports what the server was compiled with and its effective
runtime TLS and authentication configuration. It is the fastest way to tell "the server
ignores my token" from "the server was not built with authentication support".

`HELP::authorise()` exercises the claim-to-authorisation-service path against an external
HTTP endpoint, configured with `UDA_HELP_AUTHZ_URL`, `UDA_HELP_AUTHZ_CLAIM` and
`UDA_HELP_AUTHZ_EXPECT`. It is a test and demonstration function, not an access control
mechanism.

## Session lifetime

**An authenticated session lasts for the lifetime of the client–server connection or the
lifetime of the token, whichever is shorter.**

You attach a token obtained elsewhere; the server verifies it once, during the handshake;
and the claims from that single verification are what the connection runs on. When the
token's own expiry passes, the server closes the connection.

That is the whole rule. The detail behind it:

- **The token is verified once, not per request.** Verification happens at the handshake.
  No signature check, no JWKS lookup and no claim evaluation happens on the data path.
- **The token sent with later requests is ignored, not examined.** The client re-sends the
  client block, token included, with every request, because the wire format is fixed by
  the protocol version. From the handshake onwards the server reads those bytes and drops
  them without decoding: nothing is allocated, nothing is parsed, and no later token is
  ever trusted.
- **Expiry is enforced before each request is served**, using the expiry from the token
  verified at the handshake, and the same clock skew allowance used to verify it. Once it
  passes, the server refuses with error **706** and closes the connection. The client
  receives the error before the socket closes, so it can tell an expired session from a
  network failure.
- **A connection may therefore end at any time**, when its token expires. Clients that
  hold long-lived connections should expect 706 and reconnect with a fresh token.

### Changing token mid-session is not supported

A client cannot change identity on an existing connection. A different token sent on a
connection that is already established is ignored — not honoured, and not rejected.

**To use a new token, open a new connection.** That is the client's responsibility: UDA
has no mechanism for a server to answer a request while also saying "by the way, the token
you just sent was ignored", and inventing one for this case is not worth the protocol
surface.

Caveats worth stating plainly:

- A token refreshed mid-session has no effect until the client reconnects, and nothing
  reports that.
- A token revoked at the identity provider continues to work until it expires or the
  connection closes. Revocation is not observed; only expiry is. Deployments needing
  prompt revocation should issue short-lived tokens, which shortens the session with them.
- Nothing in a response identifies which token authorised it. Recording a token
  fingerprint against each request is a natural extension when provenance work needs it,
  and would be the point at which a server could report the token actually in scope.

## Client compatibility

The authentication block was introduced with UDA protocol version 11.

| Client | Server | Behaviour |
| --- | --- | --- |
| 10 | 10 | unchanged |
| 10 | 11, authentication off | works |
| 10 | 11, authentication on | refused with 700 and `CLIENT_PROTOCOL_TOO_OLD` |
| 11 | 10 | works; the token is ignored by the older server |

## Current limitations

OAuth token acquisition and refresh are not implemented in UDA. Clients obtain an access
token externally and provide it via `UDA_AUTH_TOKEN`.

Claim names containing a literal dot must be escaped as `wlcg\.groups` in claim paths.

Authorisation based on scopes, roles or claims is the responsibility of server and plugin
code. UDA verifies the token and exposes the claims.
