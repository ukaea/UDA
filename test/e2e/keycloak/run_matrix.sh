#!/usr/bin/env bash
# End-to-end authentication matrix against a real Keycloak.
#
# Requires: docker compose up -d --build   (see README.md)
# Optional: a UDA server built with -DENABLE_AUTH=ON. Without one, the token-shape and
#           claim-policy checks still run; the connection checks are skipped.
#
#   ./run_matrix.sh              run everything available
#   ./run_matrix.sh --list       show the scenarios without running them
#
# Exit status: 0 all passed, 1 one or more failed, 2 the rig is not up.

set -uo pipefail

KEYCLOAK_URL=${KEYCLOAK_URL:-http://127.0.0.1:8080}
REALM=${KEYCLOAK_REALM:-uda-test}
ISSUER="${KEYCLOAK_URL%/}/realms/${REALM}"
AUTHZ_URL=${AUTHZ_URL:-http://127.0.0.1:8765/authorize}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

pass=0; fail=0; skip=0

ok()    { printf '  \033[32mPASS\033[0m %s\n' "$1"; pass=$((pass+1)); }
no()    { printf '  \033[31mFAIL\033[0m %s\n'   "$1"; shift; [ $# -gt 0 ] && printf '       %s\n' "$*"; fail=$((fail+1)); }
skipit(){ printf '  \033[33mSKIP\033[0m %s\n' "$1"; skip=$((skip+1)); }
group() { printf '\n\033[1m%s\033[0m\n' "$1"; }

# Decode a JWT payload without verifying it — for asserting on token shape only.
jwt_payload() {
    local token=$1 body
    body=$(cut -d. -f2 <<<"$token")
    # base64url -> base64, pad to a multiple of 4
    body=${body//-/+}; body=${body//_//}
    while [ $(( ${#body} % 4 )) -ne 0 ]; do body="${body}="; done
    printf '%s' "$body" | base64 -d 2>/dev/null
}

require_rig() {
    if ! curl -fsS --max-time 5 "${ISSUER}/.well-known/openid-configuration" >/dev/null 2>&1; then
        printf 'Keycloak is not reachable at %s\n' "$ISSUER" >&2
        printf 'Start the rig first:  docker compose up -d --build\n' >&2
        exit 2
    fi
}

if [ "${1:-}" = "--list" ]; then
    grep -oE '^\s*group "[^"]+"' "$0" | sed 's/.*group "/  /; s/"$//'
    exit 0
fi

require_rig

# ---------------------------------------------------------------------------
group "Identity provider"

if curl -fsS --max-time 5 "${ISSUER}/.well-known/openid-configuration" | jq -e '.jwks_uri' >/dev/null; then
    ok "discovery document advertises a jwks_uri"
else
    no "discovery document advertises a jwks_uri"
fi

if curl -fsS --max-time 5 "$(curl -fsS "${ISSUER}/.well-known/openid-configuration" | jq -r .jwks_uri)" \
     | jq -e '.keys | length > 0' >/dev/null; then
    ok "JWKS endpoint serves at least one signing key"
else
    no "JWKS endpoint serves at least one signing key"
fi

# ---------------------------------------------------------------------------
group "Authentication — who you are"

ALICE_TOKEN=$("$HERE/mint-token.sh" alice 2>/dev/null) && ok "alice can obtain a token" \
    || no "alice can obtain a token"

# The point of adam: authentication succeeds for him exactly as it does for alice.
# He is a real, enabled account with valid credentials. Everything that follows
# separates that from authorisation.
ADAM_TOKEN=$("$HERE/mint-token.sh" adam 2>/dev/null) && ok "adam can obtain a token (he is a valid user)" \
    || no "adam can obtain a token (he is a valid user)"

SERVICE_TOKEN=$("$HERE/mint-token.sh" service 2>/dev/null) && ok "the service account can obtain a token" \
    || no "the service account can obtain a token"

if ! "$HERE/mint-token.sh" alice >/dev/null 2>&1 <<<"" ; then :; fi
if KEYCLOAK_PASSWORD=wrong-password "$HERE/mint-token.sh" alice >/dev/null 2>&1; then
    no "a wrong password is refused by the IdP"
else
    ok "a wrong password is refused by the IdP"
fi

# ---------------------------------------------------------------------------
group "Token shape — both users get equally valid tokens"

for pair in "alice:$ALICE_TOKEN" "adam:$ADAM_TOKEN"; do
    who=${pair%%:*}; tok=${pair#*:}
    payload=$(jwt_payload "$tok")

    [ "$(jq -r .iss <<<"$payload")" = "$ISSUER" ] \
        && ok "$who: iss is the expected issuer" || no "$who: iss is the expected issuer"

    jq -e '.aud | if type == "array" then index("uda") else . == "uda" end' <<<"$payload" >/dev/null \
        && ok "$who: aud contains uda" || no "$who: aud contains uda"

    [ "$(jq -r .preferred_username <<<"$payload")" = "$who" ] \
        && ok "$who: preferred_username is $who" || no "$who: preferred_username is $who"

    exp=$(jq -r .exp <<<"$payload"); now=$(date +%s)
    [ "$exp" -gt "$now" ] && ok "$who: token is not expired" || no "$who: token is not expired"
done

# ---------------------------------------------------------------------------
group "Authorisation — what you may do"

# This is the pair that matters. Both tokens are signed by the same issuer, for the same
# audience, and are unexpired. The only difference is group and role membership, and that
# is the difference the claim policy must act on.

alice_payload=$(jwt_payload "$ALICE_TOKEN")
adam_payload=$(jwt_payload "$ADAM_TOKEN")

jq -e '.realm_access.roles | index("uda-user")' <<<"$alice_payload" >/dev/null \
    && ok "alice holds the uda-user realm role" || no "alice holds the uda-user realm role"

jq -e '(.realm_access.roles // []) | index("uda-user") | not' <<<"$adam_payload" >/dev/null \
    && ok "adam does NOT hold the uda-user realm role" || no "adam does NOT hold the uda-user realm role"

jq -e '.groups | index("/uda-users")' <<<"$alice_payload" >/dev/null \
    && ok "alice is in the /uda-users group" || no "alice is in the /uda-users group"

jq -e '(.groups // []) | index("/uda-users") | not' <<<"$adam_payload" >/dev/null \
    && ok "adam is NOT in the /uda-users group" || no "adam is NOT in the /uda-users group"

# ---------------------------------------------------------------------------
group "Claim policy — the decision UDA actually makes"

# uda_token_check runs the production authenticate() — discovery, JWKS fetch, signature
# verification, issuer and audience checks, claim policy — against the live IdP. Its
# verdict is the verdict the server would reach, so these are not a re-implementation of
# the policy, they are the policy.
#
#   exit 0 = accepted, 2 = refused, 3 = internal error
CHECK=${UDA_TOKEN_CHECK:-}
if [ -z "$CHECK" ]; then
    for candidate in \
        "$HERE/../../../build-auth-tests-review/test/e2e/keycloak/uda_token_check" \
        "$HERE/../../../build/test/e2e/keycloak/uda_token_check"; do
        [ -x "$candidate" ] && { CHECK=$candidate; break; }
    done
fi

# verdict <token> <policy-spec> <audience>  -> prints ACCEPTED/REFUSED line, returns its code
verdict() {
    "$CHECK" --token "$1" --issuer "$ISSUER" --audience "$3" \
             ${2:+--required-claims "$2"} --allow-http 2>&1
}

if [ -n "$CHECK" ] && [ -x "$CHECK" ]; then
    ROLE_POLICY='realm_access.roles:contains:uda-user'
    GROUP_POLICY='groups:contains:/uda-users'

    # The pair that matters. Same issuer, same audience, both unexpired, both correctly
    # signed. Only membership differs.
    out=$(verdict "$ALICE_TOKEN" "$ROLE_POLICY" uda); rc=$?
    [ $rc -eq 0 ] && ok "role policy ACCEPTS alice" || no "role policy ACCEPTS alice" "$out"

    out=$(verdict "$ADAM_TOKEN" "$ROLE_POLICY" uda); rc=$?
    if [ $rc -eq 2 ] && grep -q "CLAIM_POLICY" <<<"$out"; then
        ok "role policy REFUSES adam with 705 CLAIM_POLICY"
    else
        no "role policy REFUSES adam with 705 CLAIM_POLICY" "$out"
    fi

    out=$(verdict "$ALICE_TOKEN" "$GROUP_POLICY" uda); rc=$?
    [ $rc -eq 0 ] && ok "group policy ACCEPTS alice" || no "group policy ACCEPTS alice" "$out"

    out=$(verdict "$ADAM_TOKEN" "$GROUP_POLICY" uda); rc=$?
    if [ $rc -eq 2 ] && grep -q "does not contain" <<<"$out"; then
        # Not "claim is missing": adam IS in a group, just not this one. The policy has to
        # compare values, not merely check presence.
        ok "group policy REFUSES adam on value, not absence"
    else
        no "group policy REFUSES adam on value, not absence" "$out"
    fi

    # ...and the same adam token is accepted by a policy he does satisfy, which shows the
    # refusals above are about the policy and not about adam.
    out=$(verdict "$ADAM_TOKEN" 'groups:contains:/uda-observers' uda); rc=$?
    [ $rc -eq 0 ] && ok "an observer policy ACCEPTS adam" || no "an observer policy ACCEPTS adam" "$out"

    # Audience, configuration and malformed-token handling through the same path.
    out=$(verdict "$ALICE_TOKEN" "$ROLE_POLICY" wrong-audience); rc=$?
    if [ $rc -eq 2 ] && grep -q "TOKEN_BAD_AUDIENCE" <<<"$out"; then
        ok "a wrong audience is refused with 708 TOKEN_BAD_AUDIENCE"
    else
        no "a wrong audience is refused with 708 TOKEN_BAD_AUDIENCE" "$out"
    fi

    out=$("$CHECK" --token "$ALICE_TOKEN" --issuer "$ISSUER" --allow-http 2>&1); rc=$?
    if [ $rc -eq 2 ] && grep -q "INVALID_CONFIG" <<<"$out"; then
        ok "a server with no claim policy refuses rather than accepting anything"
    else
        no "a server with no claim policy refuses rather than accepting anything" "$out"
    fi

    out=$("$CHECK" --token "not.a.jwt" --issuer "$ISSUER" --audience uda --allow-http 2>&1); rc=$?
    if [ $rc -eq 2 ] && grep -q "INVALID_TOKEN" <<<"$out"; then
        ok "a malformed token is refused with 704, not an internal error"
    else
        no "a malformed token is refused with 704, not an internal error" "$out"
    fi

    out=$("$CHECK" --token "$ALICE_TOKEN" --issuer "https://wrong-issuer.example" \
                   --audience uda --allow-http 2>&1); rc=$?
    if [ $rc -eq 2 ]; then
        ok "a token from the wrong issuer is refused"
    else
        no "a token from the wrong issuer is refused" "$out"
    fi
else
    skipit "uda_token_check not built — build with -DENABLE_AUTH=ON, or set UDA_TOKEN_CHECK"
    skipit "  (the claim-policy decisions are the part this suite exists to prove)"
fi

# ---------------------------------------------------------------------------
group "Authorisation service (HELP::authorise backend)"

if curl -fsS --max-time 5 "${AUTHZ_URL%/authorize}/health" >/dev/null 2>&1; then
    body=$(curl -fsS "${AUTHZ_URL}?claim=preferred_username&value=alice")
    [ "$body" = "True" ] && ok "authz service authorises alice" || no "authz service authorises alice" "got '$body'"

    body=$(curl -fsS "${AUTHZ_URL}?claim=preferred_username&value=adam")
    [ "$body" = "False" ] && ok "authz service refuses adam" || no "authz service refuses adam" "got '$body'"
else
    skipit "authz service not running (docker compose up -d authz)"
fi

# ---------------------------------------------------------------------------
group "UDA server connection"

# Everything above stops at the token. This group drives a real UDA server over a real
# socket: the protocol-11 handshake carrying the authentication block, the server-side
# OIDC gate, a plugin call, and the error code the client actually receives.
#
# Set UDA_INSTALL to an install prefix built with -DENABLE_AUTH=ON. The server is started
# here, under inetd_shim.py, and stopped on exit.
CONNECT=${UDA_CONNECT_CHECK:-}
if [ -z "$CONNECT" ]; then
    for candidate in \
        "$HERE/../../../build-auth-tests-review/test/e2e/keycloak/uda_connect_check" \
        "$HERE/../../../build/test/e2e/keycloak/uda_connect_check"; do
        [ -x "$candidate" ] && { CONNECT=$candidate; break; }
    done
fi

UDA_INSTALL=${UDA_INSTALL:-}
SERVER_PORT=${UDA_E2E_PORT:-56570}
SERVER_PID=""
OWN_SERVER=0
LOGDIR=""

stop_server() {
    if [ -n "$SERVER_PID" ]; then
        kill "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null
        SERVER_PID=""
    fi
}
trap stop_server EXIT

# Two ways to get a server:
#   UDA_EXTERNAL_SERVER=1  one is already running and managed elsewhere — launchd, systemd
#                          socket activation, a container. Point UDA_HOST/UDA_PORT at it.
#                          This is the mode that validates a real deployment rather than
#                          one this script spawned.
#   UDA_INSTALL=<prefix>   start one here for the duration of the run.
#
# In external mode the log assertions need to know where the server writes; set
# UDA_E2E_LOGDIR. Without it they are skipped rather than guessed at.
if [ "${UDA_EXTERNAL_SERVER:-0}" = "1" ]; then
    SERVER_HOST=${UDA_HOST:-127.0.0.1}
    SERVER_PORT=${UDA_PORT:-$SERVER_PORT}
    LOGDIR=${UDA_E2E_LOGDIR:-}
elif [ -n "$UDA_INSTALL" ] && [ -x "$UDA_INSTALL/bin/uda_server" ]; then
    OWN_SERVER=1
    SERVER_HOST=127.0.0.1
    LOGDIR=$(mktemp -d "${TMPDIR:-/tmp}/uda-e2e-logs.XXXXXX")
fi

if [ -n "${SERVER_HOST:-}" ] && [ -n "$CONNECT" ] && [ -x "$CONNECT" ]; then

    if [ "$OWN_SERVER" = "1" ]; then
        UDA_E2E_LOGDIR="$LOGDIR" "$HERE/run_server.sh" \
            --install "$UDA_INSTALL" --port "$SERVER_PORT" --mode oidc \
            > "$LOGDIR/server.out" 2>&1 &
        SERVER_PID=$!

        # Wait for the listener rather than sleeping a fixed amount.
        for _ in $(seq 1 30); do
            if nc -z "$SERVER_HOST" "$SERVER_PORT" 2>/dev/null; then break; fi
            sleep 0.2
        done
    fi

    export UDA_HOST=$SERVER_HOST
    export UDA_PORT=$SERVER_PORT

    if nc -z "$SERVER_HOST" "$SERVER_PORT" 2>/dev/null; then
        # alice: authorised, so a normal request must simply work.
        out=$(UDA_AUTH_TOKEN="$ALICE_TOKEN" "$CONNECT" --request "HELP::ping()" 2>&1); rc=$?
        [ $rc -eq 0 ] && ok "alice completes a real request" || no "alice completes a real request" "$out"

        # adam: authenticated, refused by policy, and the refusal must reach the client as
        # the documented code rather than a generic protocol failure.
        out=$(UDA_AUTH_TOKEN="$ADAM_TOKEN" "$CONNECT" --request "HELP::ping()" 2>&1); rc=$?
        if [ $rc -eq 2 ] && grep -q "code=705" <<<"$out"; then
            ok "adam is refused at the server with code 705"
        else
            no "adam is refused at the server with code 705" "$out"
        fi

        # No token at all.
        out=$(env -u UDA_AUTH_TOKEN "$CONNECT" --request "HELP::ping()" 2>&1); rc=$?
        if [ $rc -eq 2 ] && grep -q "code=700" <<<"$out"; then
            ok "a connection with no token is refused with code 700"
        else
            no "a connection with no token is refused with code 700" "$out"
        fi

        # A malformed token must be refused as a token fault, not an internal error.
        out=$(UDA_AUTH_TOKEN="not.a.jwt" "$CONNECT" --request "HELP::ping()" 2>&1); rc=$?
        if [ $rc -eq 2 ] && grep -q "code=704" <<<"$out"; then
            ok "a malformed token is refused with code 704"
        else
            no "a malformed token is refused with code 704" "$out"
        fi

        # Many requests on one connection. This is the path where the client block, bearer
        # token included, is re-sent and re-decoded per request.
        out=$(UDA_AUTH_TOKEN="$ALICE_TOKEN" "$CONNECT" --request "HELP::ping()" --repeat 25 2>&1); rc=$?
        [ $rc -eq 0 ] && ok "25 requests on one connection" || no "25 requests on one connection" "$out"

        # HELP::authorise() reads a claim from the verified token and asks the external
        # service about it. Both users authenticate; the service is configured to allow
        # only alice, so this exercises authorisation downstream of authentication.
        if curl -fsS --max-time 5 "${AUTHZ_URL%/authorize}/health" >/dev/null 2>&1; then
            out=$(UDA_AUTH_TOKEN="$ALICE_TOKEN" "$CONNECT" --request "HELP::authorise()" 2>&1); rc=$?
            [ $rc -eq 0 ] && ok "HELP::authorise() runs for alice" \
                          || no "HELP::authorise() runs for alice" "$out"
        else
            skipit "HELP::authorise() — authz service not running"
        fi

        # --- the audit trail -------------------------------------------------
        if [ -z "$LOGDIR" ]; then
            skipit "audit log assertions — set UDA_E2E_LOGDIR to the server's log directory"
        else
        REFUSALS="$LOGDIR/refused_requests.log"
        if [ -f "$REFUSALS" ]; then
            ok "refused_requests.log was created"

            if [ "$(wc -l < "$REFUSALS" | tr -d ' ')" -ge 3 ]; then
                # Three refusals happened above. If the log truncated per connection —
                # which it did before it was opened in append mode — only the last would
                # survive, and this is the assertion that catches a regression.
                ok "every refusal is retained, not truncated by the next connection"
            else
                no "every refusal is retained, not truncated by the next connection" \
                   "$(wc -l < "$REFUSALS") records"
            fi

            if grep -q '"reason_code":"OIDC_CLAIM_POLICY_FAILED"' "$REFUSALS"; then
                ok "adam's refusal is recorded as OIDC_CLAIM_POLICY_FAILED"
            else
                no "adam's refusal is recorded as OIDC_CLAIM_POLICY_FAILED"
            fi

            if grep -q '"reason_code":"OIDC_TOKEN_MISSING"' "$REFUSALS"; then
                ok "the missing-token refusal is recorded as OIDC_TOKEN_MISSING"
            else
                no "the missing-token refusal is recorded as OIDC_TOKEN_MISSING"
            fi

            if python3 -c "
import json,sys
pids=set()
for line in open('$REFUSALS'):
    line=line.strip()
    if line:
        pids.add(json.loads(line).get('server_pid'))
sys.exit(0 if len(pids) > 1 else 1)" 2>/dev/null; then
                ok "each refusal names its own server process"
            else
                no "each refusal names its own server process"
            fi

            if python3 -c "
import json,sys
for line in open('$REFUSALS'):
    line=line.strip()
    if line:
        json.loads(line)
sys.exit(0)" 2>/dev/null; then
                ok "every record is valid JSON"
            else
                no "every record is valid JSON"
            fi

            # alice succeeded, so nothing of hers belongs in a log of refusals.
            if grep -q "alice" "$REFUSALS"; then
                no "a successful connection leaves no refusal record"
            else
                ok "a successful connection leaves no refusal record"
            fi
        else
            no "refused_requests.log was created" "not found in $LOGDIR"
        fi
        fi
    else
        no "the UDA server is reachable" "nothing listening on ${SERVER_HOST}:${SERVER_PORT}"
        [ "$OWN_SERVER" = "1" ] && { printf '       server output:\n'; sed 's/^/       /' "$LOGDIR/server.out" | head -10; }
    fi

    stop_server
    # Only clean up logs this script created.
    if [ "$OWN_SERVER" = "1" ]; then
        if [ "${UDA_E2E_KEEP_LOGS:-0}" = "1" ]; then
            printf '  logs kept in %s\n' "$LOGDIR"
        else
            rm -rf "$LOGDIR"
        fi
    fi
else
    skipit "no UDA server available"
    skipit "  either UDA_INSTALL=<prefix built with -DENABLE_AUTH=ON> to start one here,"
    skipit "  or UDA_EXTERNAL_SERVER=1 with UDA_HOST/UDA_PORT for one already running"
fi

# ---------------------------------------------------------------------------
printf '\n\033[1mResult\033[0m  %d passed, %d failed, %d skipped\n' "$pass" "$fail" "$skip"
[ "$fail" -eq 0 ]
