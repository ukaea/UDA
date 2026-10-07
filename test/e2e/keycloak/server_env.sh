#!/usr/bin/env bash
# Environment for a UDA server under test, layered the way a deployment layers it:
# the installed udaserver.cfg first, then the overrides this rig needs.
#
# Source it, do not execute it:  source ./server_env.sh /path/to/install
#
# UDA_E2E_MODE selects the authentication posture:
#   none   plain TCP, no OIDC       (baseline: proves the transport works)
#   oidc   OIDC with a claim policy (the real subject of the tests)

UDA_INSTALL=${1:-${UDA_INSTALL:-}}
if [ -z "$UDA_INSTALL" ] || [ ! -x "$UDA_INSTALL/bin/uda_server" ]; then
    echo "server_env.sh: pass the install prefix containing bin/uda_server" >&2
    return 1 2>/dev/null || exit 1
fi

# The installed config sets UDA_ROOT, plugin paths and library paths.
# Two things about the shipped cfg make it hostile to a strict caller, so errexit and
# nounset are both suspended across the source:
#   - it probes for optional tools with `which`, which returns non-zero when absent;
#   - it expands ${LD_LIBRARY_PATH} unguarded, which is fatal under `set -u` on a machine
#     where that variable is not already set.
# Neither is a problem for the init script it was written for; both kill a `set -euo` one.
__saved_opts=$-
set +eu
# shellcheck disable=SC1091
source "$UDA_INSTALL/etc/udaserver.cfg" > /dev/null 2>&1
case "$__saved_opts" in *e*) set -e ;; esac
case "$__saved_opts" in *u*) set -u ;; esac
unset __saved_opts

# Test overrides. These come after the cfg, because the cfg exports its own values.
export UDA_LOG=${UDA_E2E_LOGDIR:-/tmp/uda-e2e-logs}
export UDA_LOG_LEVEL=DEBUG
export UDA_LOG_MODE=a
mkdir -p "$UDA_LOG"

export DYLD_LIBRARY_PATH="$UDA_INSTALL/lib:$UDA_INSTALL/lib/plugins"
export LD_LIBRARY_PATH="$UDA_INSTALL/lib:$UDA_INSTALL/lib/plugins"

case "${UDA_E2E_MODE:-oidc}" in
  none)
    unset UDA_SERVER_AUTHENTICATION
    ;;
  oidc)
    export UDA_SERVER_AUTHENTICATION=OIDC
    export UDA_SERVER_OIDC_ISSUER=${KEYCLOAK_ISSUER:-http://127.0.0.1:8080/realms/uda-test}
    export UDA_SERVER_OIDC_AUDIENCE=uda
    export UDA_SERVER_OIDC_REQUIRED_CLAIMS=${UDA_E2E_POLICY:-'groups:contains:/uda-users'}
    # This rig serves plain HTTP on loopback and carries tokens over plain TCP.
    # Both overrides are why a real deployment must not copy this file verbatim.
    export UDA_SERVER_OIDC_ALLOW_HTTP=1
    export UDA_ALLOW_TOKEN_WITHOUT_TLS=1

    # HELP::authorise() asks this service for a decision about a claim from the verified
    # token. It is a demonstration of the claim-to-authorisation-service path, not an
    # access control mechanism.
    export UDA_HELP_AUTHZ_URL=${AUTHZ_URL:-http://127.0.0.1:8765/authorize}
    export UDA_HELP_AUTHZ_CLAIM=preferred_username
    export UDA_HELP_AUTHZ_EXPECT=True
    ;;
  *)
    echo "server_env.sh: unknown UDA_E2E_MODE '${UDA_E2E_MODE}'" >&2
    return 1 2>/dev/null || exit 1
    ;;
esac
