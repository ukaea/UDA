#!/usr/bin/env bash
# A guided tour of UDA's OIDC authentication.
#
# Each step prints the command it is about to run, runs it, and shows what came back.
# Nothing is hidden: every step is something you can type yourself.
#
#   ./demo.sh            run the whole tour
#   ./demo.sh --pause    stop between steps

set -uo pipefail

KEYCLOAK=${KEYCLOAK_URL:-http://127.0.0.1:8080}
REALM=${KEYCLOAK_REALM:-uda-test}
ISSUER="$KEYCLOAK/realms/$REALM"
AUTHZ=${AUTHZ_URL:-http://127.0.0.1:8765/authorize}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MINT="$HERE/../../test/e2e/keycloak/mint-token.sh"
PAUSE=0
[ "${1:-}" = "--pause" ] && PAUSE=1

b() { printf '\033[1m%s\033[0m\n' "$*"; }
dim() { printf '\033[2m%s\033[0m\n' "$*"; }
step() {
    printf '\n\033[1;36m── %s\033[0m\n\n' "$*"
    [ "$PAUSE" = "1" ] && { read -rp "   (enter to continue) " _ </dev/tty || true; }
    return 0
}
run() { dim "\$ $*"; eval "$@"; echo; }

command -v jq > /dev/null || { echo "this tour needs jq"; exit 2; }

if ! curl -fsS --max-time 5 "$ISSUER/.well-known/openid-configuration" > /dev/null 2>&1; then
    echo "Keycloak is not answering at $ISSUER"
    echo "Start the demonstrator first:  docker compose up -d --build"
    exit 2
fi

cat <<'INTRO'

  UDA OIDC authentication — a guided tour
  ======================================

  Three things are running: an identity provider (Keycloak), a UDA server that trusts it,
  and a small authorisation service.

  The tour shows the distinction that matters most: authentication is "who are you",
  authorisation is "what may you do". UDA does the first and gives plugins what they need
  for the second. A user can pass the first and still be refused.

INTRO

# ---------------------------------------------------------------------------------------
step "1. The identity provider publishes how to verify its tokens"

dim "Every OIDC provider publishes a discovery document. UDA reads it to find the signing"
dim "keys. Nothing here is UDA-specific — this is standard OIDC."
run "curl -fsS $ISSUER/.well-known/openid-configuration | jq '{issuer, jwks_uri}'"

dim "And the keys themselves. UDA fetches these to check token signatures, and caches them."
run "curl -fsS \$(curl -fsS $ISSUER/.well-known/openid-configuration | jq -r .jwks_uri) | jq '.keys[0] | {kty, alg, kid}'"

# ---------------------------------------------------------------------------------------
step "2. Two users, identical in every way that authentication cares about"

dim "alice is in the group /uda-users. adam is in /uda-observers."
dim "Both are real, enabled accounts with valid passwords."
run "$MINT alice > /tmp/alice.jwt && echo 'alice: token obtained'"
run "$MINT adam  > /tmp/adam.jwt  && echo 'adam:  token obtained'"

dim "Look at what is actually inside them. Same issuer, same audience, both unexpired."
dim "The only difference is the groups claim."
for who in alice adam; do
    printf '   %s:\n' "$who"
    cut -d. -f2 < /tmp/$who.jwt \
      | tr '_-' '/+' | awk '{ while (length($0)%4) $0=$0"="; print }' \
      | base64 -d 2>/dev/null \
      | jq -c '{preferred_username, iss, aud, groups, roles: .realm_access.roles}' \
      | sed 's/^/     /'
done
echo

# ---------------------------------------------------------------------------------------
step "3. The server's rule: a valid token is not enough"

dim "This UDA server is configured with:"
dim "    UDA_SERVER_OIDC_ISSUER=$ISSUER"
dim "    UDA_SERVER_OIDC_AUDIENCE=uda"
dim "    UDA_SERVER_OIDC_REQUIRED_CLAIMS='groups:contains:/uda-users'"
echo
dim "The first two are authentication: is this token genuine, and is it meant for us?"
dim "The third is authorisation: is this particular bearer allowed in?"

# ---------------------------------------------------------------------------------------
step "4. alice makes a request — accepted"

export UDA_HOST=${UDA_HOST:-localhost}
export UDA_PORT=${UDA_PORT:-${UDA_DEMO_PORT:-56600}}
CLIENT=${UDA_CLIENT:-uda_cli}

if ! command -v "$CLIENT" > /dev/null && [ ! -x "$CLIENT" ]; then
    dim "No UDA client on PATH. Set UDA_CLIENT to uda_cli or uda_connect_check to run the"
    dim "request steps. The token steps above all ran for real."
else
    run "UDA_AUTH_TOKEN=\$(cat /tmp/alice.jwt) $CLIENT --request 'HELP::ping()'"

    step "5. adam makes the same request — refused"
    dim "He authenticated perfectly well. He is simply not in the group the server requires."
    run "UDA_AUTH_TOKEN=\$(cat /tmp/adam.jwt) $CLIENT --request 'HELP::ping()' || true"
    dim "UDA error 705 is 'claim policy failed'. Note the message names the claim and the"
    dim "value it wanted — it is a comparison, not merely a missing-claim complaint."

    step "6. No token at all"
    run "$CLIENT --request 'HELP::ping()' || true"
    dim "700 is 'no bearer token'. Distinct from 705, because the fix is different."
fi

# ---------------------------------------------------------------------------------------
step "7. Every refusal is written to the audit log"

dim "refused_requests.log holds one JSON object per refused connection. It is the only"
dim "place the reason for a refusal is recorded in full, and it appends, so a later"
dim "connection never erases an earlier one's record."
run "docker exec uda-demo-server sh -c 'tail -3 /opt/uda/logs/refused_requests.log' 2>/dev/null | jq -c '{stage, reason_code, uda_error_code, peer_ip, server_pid}' || echo '   (server container not running)'"

# ---------------------------------------------------------------------------------------
step "8. Authorisation beyond UDA: HELP::authorise()"

dim "UDA verifies the token and hands the claims to plugins. What a plugin does with them"
dim "is up to the deployment. This one asks an external service about a claim value."
run "curl -fsS '$AUTHZ?claim=preferred_username&value=alice'; echo"
run "curl -fsS '$AUTHZ?claim=preferred_username&value=adam'; echo"

# ---------------------------------------------------------------------------------------
cat <<'OUTRO'

  ── Things to try next

  Change the rule and restart the server container to see the decision flip:

    UDA_DEMO_POLICY='groups:contains:/uda-observers'      adam in, alice out
    UDA_DEMO_POLICY='realm_access.roles:contains:uda-user' the same split, by role
    UDA_DEMO_POLICY='scope:contains_word:profile'          by scope

  Add a user of your own — see README.md, "Adding a user".

  Remove the policy entirely and the server refuses everyone with error 701, rather than
  accepting any valid token from the issuer. That default is deliberate.

OUTRO
