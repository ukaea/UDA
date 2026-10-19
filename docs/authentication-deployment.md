---
layout: default
title: Deployment
parent: Authentication
nav_order: 1
---

# Deploying UDA with TLS and OIDC authentication
{:.no_toc}

A runbook for enabling authentication on a real UDA server. It assumes you have an
identity provider already; UDA does not issue tokens.

If you want to see the mechanism working before committing to a design, `demo/oidc/`
brings the whole thing up in containers with a guided walkthrough.

## Contents
{:.no_toc}
1. TOC
{:toc}

## Decide these four things first

Most failed deployments are a wrong answer to one of these, not a bug.

**1. What identifies a UDA user in your IdP?** A group, a realm role, a client role, a
scope? Whatever you pick has to actually appear in the access token, which usually means
attaching a protocol mapper. A claim that is not in the token does not exist as far as UDA
is concerned.

**2. What audience will tokens carry?** UDA checks `aud`. If your IdP does not add one,
add an audience mapper. Accepting any audience means accepting tokens minted for a
different application entirely.

**3. Is the issuer URL identical everywhere?** UDA compares the token's `iss` claim to
`UDA_SERVER_OIDC_ISSUER` as a **string**. `https://idp.example/realms/x` and
`https://idp.example/realms/x/` are different. So are `localhost` and `127.0.0.1`.

**4. Does the UDA server host have network access to the IdP?** The server fetches the
discovery document and signing keys itself. Firewalls between the server and the IdP are a
common and confusing failure, because the client can reach the IdP perfectly well.

## Build

```sh
cmake -B build -DENABLE_AUTH=ON -DBUILD_SHARED_LIBS=ON
cmake --build build && cmake --install build
```

`ENABLE_AUTH=ON` enables TLS transport and OIDC together. `ENABLE_TLS` and
`ENABLE_OIDC_AUTH` can be set separately; enabling OIDC alone auto-enables TLS unless you
pass `-DALLOW_OIDC_WITHOUT_TLS=ON`, which you should not do outside a test rig.

Dependencies: nlohmann/json is required and is found with `find_package`; jwt-cpp is
required only for OIDC. Either is downloaded at a pinned version when the system does not
provide it. For packaging or an air-gapped build, set `-DUDA_FETCH_DEPENDENCIES=OFF` and
provide both as system packages, or point `FETCHCONTENT_SOURCE_DIR_<NAME>` at a local copy.

Verify what a built server actually has:

```
HELP::servermetadata()
```

## Configure, in stages

Do not enable everything at once. Each stage below is independently verifiable, and when
something breaks you want to know which layer it was.

### Stage 1 — TLS only, no tokens

```sh
export UDA_SERVER_TLS_MODE=server
export UDA_SERVER_TLS_CERT=/etc/uda/certs/server.cert.pem
export UDA_SERVER_TLS_KEY=/etc/uda/private/server.key.pem
```

Client:

```sh
export UDA_CLIENT_TLS_MODE=server
export UDA_CLIENT_CA_TLS_CERT=/etc/uda/certs/ca.cert.pem
```

The server certificate must match the name clients connect to — UDA verifies the hostname
against the certificate. Connecting by IP to a certificate issued for a name fails with
error 712, correctly.

Use `mutual` instead of `server` if you want certificate-based client identity as well.
That needs `UDA_SERVER_CA_TLS_CERT` on the server and a certificate and key on each client.

Confirm a plain request works before continuing.

### Stage 2 — add OIDC, with a permissive policy

```sh
export UDA_SERVER_AUTHENTICATION=OIDC
export UDA_SERVER_OIDC_ISSUER=https://idp.example.org/realms/fusion
export UDA_SERVER_OIDC_AUDIENCE=uda
```

An audience is a claim policy, so this is already a real gate: it accepts any token minted
for UDA by that issuer. Confirm that a user with a token gets in and that a user without
one is refused with 700.

### Stage 3 — tighten to the rule you actually want

```sh
export UDA_SERVER_OIDC_REQUIRED_CLAIMS='groups:contains:/uda-users'
```

Now confirm the part that matters: **a valid token from someone outside that group is
refused with 705.** If you cannot demonstrate a refusal, you have not demonstrated a
policy.

## Environment reference

See `docs/authentication.md` for the full table. The ones a deployment normally sets:

| Variable | Notes |
| --- | --- |
| `UDA_SERVER_AUTHENTICATION` | `OIDC` |
| `UDA_SERVER_OIDC_ISSUER` | must equal the token's `iss` exactly |
| `UDA_SERVER_OIDC_AUDIENCE` | required unless you set `REQUIRED_CLAIMS` |
| `UDA_SERVER_OIDC_REQUIRED_CLAIMS` | the authorisation rule |
| `UDA_SERVER_OIDC_JWKS_URI` | only when discovery cannot be used, or the issuer is reachable under a different URL than the one in `iss` |
| `UDA_SERVER_OIDC_JWKS_CACHE_TTL` | default 300s; lower means faster key-rotation pickup and more requests to the IdP |
| `UDA_SERVER_OIDC_CLOCK_SKEW_SECONDS` | default 60 |
| `UDA_SERVER_OIDC_ALLOWED_ALGS` | default `RS256`; add `ES256` for issuers that use EC keys |

Never set in production: `UDA_SERVER_OIDC_POLICY=none`, `UDA_SERVER_OIDC_ALLOW_HTTP=1`,
`UDA_ALLOW_TOKEN_WITHOUT_TLS=1`. Each disables a protection deliberately, and the first two
log a warning every time they take effect.

Where these go depends on how the server is started. Under inetd or systemd socket
activation the usual place is `$UDA_ROOT/etc/udaserver.cfg`, or a file in
`$UDA_ROOT/etc/plugins.d/*.cfg` — every `.cfg` there is sourced after the main config,
which makes it a convenient place for site settings that survive a reinstall.

## Rolling it out

The authentication block arrived with UDA protocol 11. The compatibility behaviour is:

| Client | Server | Result |
| --- | --- | --- |
| 10 | 10 | unchanged |
| 11 | 10 | works — the token is not sent, because the block is only serialised when both ends are at 11 |
| 10 | 11, auth off | works |
| 11 | 11, auth off | works |
| 11 | 11, auth on | works with a token; 700 without |
| 10 | 11, auth on | refused with 700 and a message telling the user to upgrade the client |

The practical consequence: **upgrade servers before you require authentication, and
upgrade clients before you turn it on.** A protocol-10 client cannot carry a token at all,
so turning on authentication while old clients remain locks them out with no workaround
other than upgrading.

A safe order:

1. Deploy protocol-11 servers with authentication **off**. Nothing changes for anyone.
2. Roll out protocol-11 clients. Still nothing changes.
3. Enable authentication on one canary server with a permissive policy (audience only).
4. Watch `refused_requests.log`. A run of `CLIENT_PROTOCOL_TOO_OLD` means clients are
   lagging; a run of `OIDC_TOKEN_MISSING` means users have not got tokens yet.
5. Tighten the claim policy.
6. Roll out to the rest.

## Operating it

### Logs

| File | Contents |
| --- | --- |
| `refused_requests.log` | one JSON object per refused connection — the audit trail |
| `auth.log` | authentication diagnostics |
| `Access.log` | completed requests |
| `Error.log`, `DebugServer.log` | general server logs |

Log files are opened in **append** mode. The server forks per connection, so they grow
continuously and need rotation — a plain `logrotate` entry with `copytruncate` is enough.
`refused_requests.log` is opened independently of `UDA_LOG`, so turning down the debug log
level does not switch off the audit trail.

Worth alerting on:

- any `TLS_CLIENT_CERT_REVOKED`
- a sustained rate of `OIDC_TOKEN_INVALID_SIGNATURE` from one peer
- any `OIDC_CONFIG_INVALID`, which means the server is misconfigured and refusing everyone
- `oidc_policy_none` being true in `HELP::servermetadata()` on a production server

### Key rotation

UDA caches the issuer's signing keys for `UDA_SERVER_OIDC_JWKS_CACHE_TTL` seconds
(default 300). When a token arrives signed with an unknown key id, it refreshes once
immediately, so ordinary rotation needs no intervention. Only a rotation faster than a
single fetch can fail, and it fails closed.

### Token lifetime bounds the session

A session lasts for the connection or the token, whichever is shorter. The token is
verified once at the handshake; when its expiry passes, the server refuses the next
request with 706 and closes the connection.

So **token lifetime is a deployment parameter**, not just an IdP detail:

- Short tokens mean sessions end more often. Clients that hold long connections must
  handle 706 by reconnecting with a fresh token.
- Long tokens mean a revoked credential keeps working longer, because UDA observes expiry
  but not revocation.
- Five to fifteen minutes suits most deployments. Go shorter only if clients reconnect
  cleanly, and confirm that before requiring it in production.

A client cannot swap tokens on an open connection — a replacement is ignored. Clients
refreshing tokens must reconnect.

## When it does not work

Read `refused_requests.log` first: it has the reason, and for TLS failures it is the only
place that does — those happen before UDA's protocol exists, so the client cannot be told.

| `reason_code` | Meaning | Usual cause |
| --- | --- | --- |
| `OIDC_TOKEN_BAD_ISSUER` | `iss` mismatch | trailing slash, or a different host name than the one in the token |
| `OIDC_TOKEN_BAD_AUDIENCE` | `aud` mismatch | no audience mapper on the IdP client |
| `OIDC_CLAIM_POLICY_FAILED` | valid token, wrong claims | user not in the group, or the mapper is absent so the claim never arrives |
| `OIDC_TOKEN_EXPIRED` | expired | clock skew between server and IdP, if it is persistent |
| `OIDC_CONFIG_INVALID` | server config | no claim policy, or no issuer |
| `OIDC_TOKEN_MISSING` | no token sent | `UDA_AUTH_TOKEN` unset, or a protocol-10 client |
| `CLIENT_PROTOCOL_TOO_OLD` | client too old | upgrade the client library |
| `TLS_CLIENT_CERT_*` | client certificate | the record names the subject and expiry |

To check a token without involving UDA at all, decode it:

```sh
cut -d. -f2 <<< "$UDA_AUTH_TOKEN" | base64 -d 2>/dev/null | jq .
```

Compare `iss`, `aud`, `exp` and your policy claim against the server's configuration. Most
problems are visible from that one command.

## Verifying a deployment

The end-to-end suite can be pointed at a running server:

```sh
UDA_EXTERNAL_SERVER=1 UDA_HOST=uda.example.org UDA_PORT=56565 \
  UDA_E2E_LOGDIR=/var/log/uda \
  test/e2e/keycloak/run_matrix.sh
```

It checks acceptance, refusal with the right codes, the audit records, and protocol
compatibility. It needs the test realm for its tokens, so it suits a staging server rather
than production.
