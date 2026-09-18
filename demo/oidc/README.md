# UDA with OIDC authentication — demonstrator

A self-contained demonstration of UDA's bearer-token authentication, for administrators
who need to understand what it does before deploying it.

Three containers: an identity provider (Keycloak), a UDA server that trusts it, and a
small authorisation service the help plugin can ask for decisions.

```sh
cd demo/oidc
docker compose up -d --build     # first build takes a few minutes
./demo.sh                        # guided tour; --pause to step through it
docker compose down
```

The UDA server is published on **port 56600**, not UDA's default 56565, so the
demonstrator can run alongside an existing server. Set `UDA_DEMO_PORT` to change it.

Everything binds to loopback and uses fixed, well-known credentials. It is a
demonstrator. Do not expose it, and see "What is not realistic here" before copying
anything into a deployment.

## The idea in one paragraph

A client obtains an access token from your identity provider and puts it in
`UDA_AUTH_TOKEN`. The UDA server verifies that token locally — signature against the
provider's published keys, issuer, audience, expiry — and then applies a **claim policy**:
a rule about what must be in the token for this bearer to be allowed in. Verified claims
are then handed to plugins, which can make finer-grained decisions. UDA never sees a
password, and never calls the provider per request.

## Authentication is not authorisation

The demonstrator ships two users to make this concrete:

| | alice | adam |
| --- | --- | --- |
| Real, enabled account | yes | yes |
| Gets a valid signed token | yes | yes |
| Group | `/uda-users` | `/uda-observers` |
| Role | `uda-user` | `uda-observer` |

The server is configured with `UDA_SERVER_OIDC_REQUIRED_CLAIMS='groups:contains:/uda-users'`.

- **alice** — request succeeds.
- **adam** — request refused, UDA error **705**, message
  `Claim 'groups' does not contain '/uda-users'`.

Adam is not an attacker and nothing is broken. He authenticated perfectly; he is simply
not in the group this server requires. That is the distinction to take away: adding a user
to your identity provider does **not** grant them access to UDA.

## Expected responses

| Situation | Client sees | Audit log `reason_code` |
| --- | --- | --- |
| alice, in the required group | request succeeds | *(no record — only refusals are logged)* |
| adam, valid token, wrong group | error **705** claim policy failed | `OIDC_CLAIM_POLICY_FAILED` |
| no `UDA_AUTH_TOKEN` set | error **700** no bearer token | `OIDC_TOKEN_MISSING` |
| token expired | error **706** | `OIDC_TOKEN_EXPIRED` |
| token for another audience | error **708** | `OIDC_TOKEN_BAD_AUDIENCE` |
| token from another issuer | error **707** | `OIDC_TOKEN_BAD_ISSUER` |
| garbled or tampered token | error **704** | `OIDC_TOKEN_INVALID_SIGNATURE` |
| server misconfigured | error **701** | `OIDC_CONFIG_INVALID` |

The full list is in `docs/authentication.md`.

## Minting a token by hand

```sh
../../test/e2e/keycloak/mint-token.sh alice     # authorised
../../test/e2e/keycloak/mint-token.sh adam      # authenticated, not authorised
../../test/e2e/keycloak/mint-token.sh service   # a service account
```

It prints only the token, so:

```sh
export UDA_AUTH_TOKEN=$(../../test/e2e/keycloak/mint-token.sh alice)
export UDA_HOST=localhost UDA_PORT=56600
uda_cli --request "HELP::ping()"
```

To see inside a token (no verification — diagnostic only):

```sh
cut -d. -f2 <<< "$UDA_AUTH_TOKEN" | base64 -d 2>/dev/null | jq .
```

## Adding a user and a group

Through the admin console at <http://127.0.0.1:8080/admin/> (`admin` /
`admin-test-password`), realm `uda-test`:

1. **Groups → Create group** — say `uda-writers`.
2. **Users → Add user** — username `bob`, then **Credentials** → set a password with
   *Temporary* off, then **Groups** → *Join Group* → `uda-writers`.
3. Mint a token for bob:

   ```sh
   KEYCLOAK_USERNAME=bob KEYCLOAK_PASSWORD=<password> \
     ../../test/e2e/keycloak/mint-token.sh alice > /tmp/bob.jwt
   ```

   (The script's `alice` mode is a password grant; the username and password override it.)

4. Confirm the group reached the token:

   ```sh
   cut -d. -f2 < /tmp/bob.jwt | base64 -d 2>/dev/null | jq .groups
   ```

   If `groups` is missing, the realm's group-membership mapper is not attached to the
   client — that mapper is what puts groups in the token, and it is a common first
   stumble.

5. Let bob in, by changing the server's rule in `docker-compose.yml`:

   ```yaml
   UDA_DEMO_POLICY: groups:contains:/uda-writers
   ```

   then `docker compose up -d uda`. Now bob is accepted and alice is not.

To make the change permanent for everyone, edit `test/e2e/keycloak/uda-test-realm.json`
and rebuild — the realm is imported on first start, so an existing container keeps its
old state until removed with `docker compose down -v`.

## Writing a claim policy

`UDA_SERVER_OIDC_REQUIRED_CLAIMS` is a semicolon-separated list of rules,
`<path>:<op>` or `<path>:<op>:<value>`:

| Op | Meaning |
| --- | --- |
| `exists` | the claim is present |
| `equals` | exact match |
| `contains` | element of an array claim |
| `contains_word` | whitespace-delimited token — right for `scope` |
| `contains_any` | any of a comma-separated list |

Paths navigate nested claims and index arrays: `realm_access.roles`,
`resource_access.uda-client.roles[0]`, and `wlcg\.groups` for a claim name that itself
contains a dot.

Multiple rules are ANDed:

```sh
UDA_SERVER_OIDC_REQUIRED_CLAIMS='groups:contains:/uda-users;scope:contains_word:profile'
```

**A policy is required.** With no audience and no claim policy, the server refuses every
connection with error 701 rather than accepting any valid token the issuer ever minted —
including tokens issued for a completely different application. `UDA_SERVER_OIDC_POLICY=none`
opts out deliberately and logs a warning on every connection.

## Where to look when it does not work

```sh
docker compose logs uda
docker exec uda-demo-server tail -5 /opt/uda/logs/refused_requests.log | jq .
docker exec uda-demo-server tail -20 /opt/uda/logs/auth.log
```

`refused_requests.log` is one JSON object per refused connection, and it is the only place
the full reason is recorded — a client cannot always be told why, particularly for TLS
failures, which happen before UDA's protocol is even established.

Common first problems:

| Symptom | Usually |
| --- | --- |
| everything refused with 707 | `UDA_SERVER_OIDC_ISSUER` does not match the token's `iss` exactly — a trailing slash or `localhost` vs `127.0.0.1` is enough |
| everything refused with 708 | no audience mapper on the client, so `aud` is not what the server expects |
| everything refused with 705 | the claim exists but not the value, or the mapper is missing so the claim is absent |
| everything refused with 701 | server has an issuer but no policy, or no issuer at all |
| refused with 702/703 | the server cannot reach the issuer — check the URL is reachable *from the server*, not just from your laptop |

## What is not realistic here

- **No TLS.** The demonstrator carries tokens over plain TCP and fetches the provider's
  keys over plain HTTP, which needs `UDA_SERVER_OIDC_ALLOW_HTTP=1` and
  `UDA_ALLOW_TOKEN_WITHOUT_TLS=1`. A real deployment sets neither and runs with
  `UDA_SERVER_TLS_MODE=server` or `mutual`. A bearer token on an unencrypted connection is
  readable by anything on the path.
- **Fixed, published credentials** for the realm, the client secret and both users.
- **Keycloak in dev mode** with no persistence.
- **The authorisation service is a stub** that compares a string. It does not verify
  tokens and is not an access-control system.

## Verifying it automatically

The same behaviours the tour shows by hand are asserted by the end-to-end suite, which can
point at the demonstrator:

```sh
UDA_EXTERNAL_SERVER=1 UDA_HOST=localhost UDA_PORT=56600 \
  ../../test/e2e/keycloak/run_matrix.sh
```

## How this relates to the tests

The demonstrator shares its realm and helper scripts with the end-to-end test suite in
`test/e2e/keycloak/`, which asserts the same behaviours automatically. If you want to
check a real deployment rather than read about one, that suite can point at an existing
server:

```sh
UDA_EXTERNAL_SERVER=1 UDA_HOST=myserver UDA_PORT=56565 \
  UDA_E2E_LOGDIR=/path/to/server/logs \
  test/e2e/keycloak/run_matrix.sh
```
