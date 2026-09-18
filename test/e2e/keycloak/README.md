# End-to-end authentication tests against a real Keycloak

Everything in `test/unit_tests/authentication` is offline: generated keys, injected JWKS,
mock HTTP fetchers. That is the right shape for unit tests, but it cannot catch a
mismatch between what UDA expects and what a real identity provider actually issues.

This suite closes that gap. It runs a disposable Keycloak with a fixed realm, mints real
tokens, and puts them through UDA's real verification code.

Everything here binds to loopback and uses fixed, well-known test credentials. It is for a
developer machine or a CI runner. Never expose it, and never reuse the realm.

### About the credentials in this directory

The client secret and the users' passwords are written down here on purpose: the realm is
created from this file when the container starts and destroyed with it, and both the tests
and this document have to name them to be usable. They grant access to nothing outside the
throwaway container.

They are declared in `.gitguardian.yaml` at the repository root, file by file, so that
secret scanning does not flag them while still catching anything added elsewhere. If these
tests ever need a real credential, it must come from the environment or a CI secret rather
than from a file in here.

## Running it

```sh
docker compose up -d --build     # Keycloak + the dummy authorisation service
UDA_INSTALL=/path/to/install ./run_matrix.sh
docker compose down
```

`UDA_INSTALL` is an install prefix from a build with `-DENABLE_AUTH=ON`
(`cmake --install`). Given one, the suite starts a real UDA server, drives real requests
through it, and asserts on the audit log. Without one, the token and claim-policy groups
still run and the connection group reports SKIP.

`run_matrix.sh --list` shows the scenario groups without running them.

The claim-policy scenarios need `uda_token_check`, which is built as part of any
`-DENABLE_AUTH=ON` build. The script finds it automatically in the usual build
directories, or set `UDA_TOKEN_CHECK` to its path. Without it the token-shape checks
still run and the policy decisions are skipped — which is most of the point of the suite,
so the skip is loud.

## The realm

Two human users, and the difference between them is the whole design.

| | alice | adam |
| --- | --- | --- |
| Enabled, real credentials | yes | yes |
| Authenticates against Keycloak | yes | yes |
| Gets a signed token for audience `uda` | yes | yes |
| Realm role | `uda-user` | `uda-observer` |
| Group | `/uda-users` | `/uda-observers` |

**alice is authorised. adam is authenticated but not authorised.**

This pair exists to keep the two concepts apart. Adam is not an attacker and not a
mistake: he is a legitimate member of the organisation who simply is not in the group UDA
requires. His token is signed by the same issuer, for the same audience, with the same
algorithm, and is equally unexpired. The only thing that differs is a claim value.

That matters for two reasons:

- It proves the claim policy compares *values* rather than merely checking that a claim is
  present. Adam is in `/uda-observers`, so the `groups` claim exists in his token — the
  refusal message is "Claim 'groups' does not contain '/uda-users'", not "claim is
  missing". An implementation that only tested presence would wrongly admit him.
- It proves the refusal is about the policy and not about adam. The suite also runs his
  token against a `groups:contains:/uda-observers` policy, which accepts him.

There is also a `service` client-credentials account for the non-interactive case.

## Minting tokens by hand

```sh
UDA_AUTH_TOKEN=$(./mint-token.sh alice) && export UDA_AUTH_TOKEN
./mint-token.sh adam
./mint-token.sh service
```

The script prints only the token. It needs `curl` and `jq`.

Inspect one with Keycloak's introspection endpoint:

```sh
curl -fsS -u uda-client:uda-test-secret \
  -d "token=$UDA_AUTH_TOKEN" \
  http://127.0.0.1:8080/realms/uda-test/protocol/openid-connect/token/introspect \
  | jq '{active, username, aud, scope, exp}'
```

## Checking a token the way the server would

`uda_token_check` calls the same `authenticate()` the server calls, so its verdict is the
server's verdict:

```sh
uda_token_check --token "$UDA_AUTH_TOKEN" \
  --issuer http://127.0.0.1:8080/realms/uda-test \
  --audience uda \
  --required-claims 'groups:contains:/uda-users' \
  --allow-http
```

Exit status: `0` accepted, `2` refused (the UDA error code and reason are printed), `3`
usage or internal error. `--allow-http` is needed only because this local rig serves plain
HTTP; a real deployment must not set it.

## Running a UDA server by hand

`run_matrix.sh` starts and stops a server itself, but it is often more useful to leave one
running and poke at it.

```sh
./run_server.sh --install /path/to/install --port 56570 --mode oidc
```

`server_env.sh` holds the configuration: it sources the installed `udaserver.cfg` and then
layers the test overrides on top, which is how a deployment layers `machine.d` config over
the defaults. `--mode none` starts the same server with no authentication, which is the
baseline to reach for when something fails and you need to know whether the transport or
the auth is at fault.

Then, from another terminal:

```sh
export UDA_HOST=127.0.0.1 UDA_PORT=56570
UDA_AUTH_TOKEN=$(./mint-token.sh alice) uda_connect_check   # OK
UDA_AUTH_TOKEN=$(./mint-token.sh adam)  uda_connect_check   # REFUSED code=705
uda_connect_check                                           # REFUSED code=700
```

`uda_connect_check --repeat N` issues N requests on one connection, which is the path that
re-sends and re-decodes the client block, bearer token included.

### Why there is an inetd shim

The UDA server is not a daemon. It handles exactly one connection, on file descriptor 0,
and exits; in production inetd or systemd socket activation arranges that. `inetd_shim.py`
reproduces the same contract in about forty lines, because systemd is not available on
macOS and because depending on it would make the suite unrunnable on a developer machine.

The fork-per-connection shape is not a convenience. It is the model the server is written
against, and several behaviours the suite asserts on depend on it: each refusal record
naming its own `server_pid`, the audit log surviving across connections, and one
authentication per connection.

### What the connection group proves

The offline tests verify tokens. This group verifies a system:

- a protocol-11 handshake carrying the authentication block
- the server-side OIDC gate, and that a refusal reaches the client as the documented
  numeric code rather than a generic protocol failure
- that a refusal is written to `refused_requests.log`, with the right reason code
- that records from separate connections **accumulate**. The log is opened in append mode
  precisely because the server forks per connection; a truncating open would let each new
  connection erase its predecessor's evidence, and this assertion is what would catch that
  regressing
- that a successful connection leaves no refusal record

## The authorisation service

`authz_server.py` is the dummy decision endpoint `HELP::authorise()` calls. It listens on
`127.0.0.1:8765` and returns the exact body `True` only for the configured claim and
value, `False` otherwise. It never logs the claim value.

```sh
curl -fsS 'http://127.0.0.1:8765/authorize?claim=preferred_username&value=alice'  # True
curl -fsS 'http://127.0.0.1:8765/authorize?claim=preferred_username&value=adam'   # False
```

Change who it allows with `AUTHZ_ALLOWED_VALUE`. It does not verify JWTs and is not an
access control service.

For the UDA server:

```sh
export UDA_HELP_AUTHZ_URL=http://127.0.0.1:8765/authorize
export UDA_HELP_AUTHZ_CLAIM=preferred_username
export UDA_HELP_AUTHZ_EXPECT=True
```

Note the endpoint is spelled `/authorize`; the plugin function accepts both
`HELP::authorise()` and `HELP::authorize()`.

## Admin console

<http://127.0.0.1:8080/admin/>, `admin` / `admin-test-password`.
