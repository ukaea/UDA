#!/usr/bin/env bash
# Start a UDA server for the e2e tests, under the inetd shim.
#
#   ./run_server.sh --install <prefix> [--port 56570] [--mode oidc|none]
#
# Runs in the foreground; Ctrl-C to stop. The matrix script starts it in the background.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
install_dir=""
port=56570
mode=oidc

while [ $# -gt 0 ]; do
    case "$1" in
        --install) install_dir=$2; shift 2 ;;
        --port)    port=$2;        shift 2 ;;
        --mode)    mode=$2;        shift 2 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

if [ -z "$install_dir" ]; then
    echo "usage: $0 --install <prefix> [--port N] [--mode oidc|none]" >&2
    exit 2
fi

export UDA_E2E_MODE=$mode
# shellcheck disable=SC1091
source "$HERE/server_env.sh" "$install_dir"

echo "uda server: mode=$mode port=$port logs=$UDA_LOG"
exec python3 "$HERE/inetd_shim.py" --server "$install_dir/bin/uda_server" --port "$port"
