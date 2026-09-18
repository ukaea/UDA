# End-to-end authentication tests against a real Keycloak

Everything in `test/unit_tests/authentication` is offline: generated keys, injected JWKS,
mock HTTP fetchers. That is the right shape for unit tests, but it cannot catch a
mismatch between what UDA expects and what a real identity provider actually issues.

This suite closes that gap. It runs a disposable Keycloak with a fixed realm, mints real
tokens, and puts them through UDA's real verification code.

Everything here binds to loopback and uses fixed, well-known test credentials. It is for a
developer machine or a CI runner. Never expose it, and never reuse the realm.

## Running it

```sh
docker compose up -d --build     # Keycloak + the dummy authorisation service
./run_matrix.sh
docker compose down
```

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

## Pointing a UDA server at the rig

Build with `-DENABLE_AUTH=ON`, then configure the server process:

```sh
export UDA_SERVER_AUTHENTICATION=OIDC
export UDA_SERVER_OIDC_ISSUER=http://127.0.0.1:8080/realms/uda-test
export UDA_SERVER_OIDC_AUDIENCE=uda
export UDA_SERVER_OIDC_REQUIRED_CLAIMS='groups:contains:/uda-users'
export UDA_SERVER_OIDC_ALLOW_HTTP=1
export UDA_ALLOW_TOKEN_WITHOUT_TLS=1   # this rig is plain HTTP on loopback
```

Then, from a client with alice's token exported, a normal request succeeds; with adam's it
fails with UDA error 705 and a `OIDC_CLAIM_POLICY_FAILED` line in
`refused_requests.log`.

Driving a real UDA server from `run_matrix.sh` is not yet scripted — the connection group
reports SKIP. That is the next piece of work.

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
