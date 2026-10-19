#!/usr/bin/env bash
set -euo pipefail

# Mint an access token from the disposable uda-test realm.
# Prints only the token, so it can be captured directly into UDA_AUTH_TOKEN.
#
#   ./mint-token.sh alice     # authorised: in the uda-user role and the /uda-users group
#   ./mint-token.sh adam      # authenticated but NOT authorised: in neither
#   ./mint-token.sh service   # client-credentials service account

mode=${1:-alice}
base_url=${KEYCLOAK_URL:-http://127.0.0.1:8080}
realm=${KEYCLOAK_REALM:-uda-test}
client_id=${KEYCLOAK_CLIENT_ID:-uda-client}
client_secret=${KEYCLOAK_CLIENT_SECRET:-uda-test-secret}

case "$mode" in
  alice)
    grant_args=(
      --data-urlencode grant_type=password
      --data-urlencode "username=${KEYCLOAK_USERNAME:-alice}"
      --data-urlencode "password=${KEYCLOAK_PASSWORD:-alice-test-password}"
    )
    ;;
  adam)
    grant_args=(
      --data-urlencode grant_type=password
      --data-urlencode username=adam
      --data-urlencode password=adam-test-password
    )
    ;;
  user)
    # Backwards-compatible alias for alice.
    grant_args=(
      --data-urlencode grant_type=password
      --data-urlencode "username=${KEYCLOAK_USERNAME:-alice}"
      --data-urlencode "password=${KEYCLOAK_PASSWORD:-alice-test-password}"
    )
    ;;
  service)
    grant_args=(--data-urlencode grant_type=client_credentials)
    ;;
  *)
    printf 'Usage: %s [alice|adam|service]\n' "$0" >&2
    exit 2
    ;;
esac

curl --fail-with-body --silent --show-error --max-time 30 \
  --user "$client_id:$client_secret" \
  "${grant_args[@]}" \
  "${base_url%/}/realms/$realm/protocol/openid-connect/token" \
  | jq --exit-status --raw-output '.access_token | select(type == "string" and length > 0)'
