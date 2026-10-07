#!/usr/bin/env python3
"""Run a UDA server the way inetd does, without needing inetd.

The UDA server is not a daemon: it handles exactly one connection, on file descriptor 0,
and exits. In production that is arranged by inetd or systemd socket activation. For a
test rig neither is convenient — systemd is not available on macOS at all — so this
reproduces the same contract in ~40 lines: accept a connection, fork, dup the socket onto
fds 0 and 1, exec the server.

That fork-per-connection shape is not an approximation for the tests' benefit. It is the
model the server is actually written against, and several behaviours under test depend on
it: the per-connection process identity in refused_requests.log, the append-only audit
log, and the single-authentication-per-connection design.

    ./inetd_shim.py --server /path/to/uda_server --port 56565
"""

import argparse
import os
import signal
import socket
import sys


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--server", required=True, help="path to the uda_server binary")
    ap.add_argument("--port", type=int, default=56565)
    ap.add_argument("--host", default="127.0.0.1")
    args = ap.parse_args()

    if not os.access(args.server, os.X_OK):
        print(f"not executable: {args.server}", file=sys.stderr)
        return 2

    # Reap children as they exit; each connection is a short-lived process.
    #
    # SIG_IGN is the concise way to do that, but it is inherited across exec, and glibc's
    # system() then fails with ECHILD because it cannot wait for its own child. The UDA
    # server calls system() during startup, so the disposition is restored to SIG_DFL in
    # the child below. Without that the server fails on Linux with "mkdir command failed",
    # while working on macOS — whose system() tolerates it.
    signal.signal(signal.SIGCHLD, signal.SIG_IGN)

    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind((args.host, args.port))
    listener.listen(16)
    print(f"uda inetd shim: {args.host}:{args.port} -> {args.server}", flush=True)

    while True:
        try:
            conn, _peer = listener.accept()
        except KeyboardInterrupt:
            break
        except OSError:
            continue

        pid = os.fork()
        if pid == 0:
            # Child: become the server, with the connection on stdin and stdout.
            listener.close()
            # Undo the inherited SIG_IGN before exec — see the note above.
            signal.signal(signal.SIGCHLD, signal.SIG_DFL)
            os.dup2(conn.fileno(), 0)
            os.dup2(conn.fileno(), 1)
            conn.close()
            try:
                os.execv(args.server, [args.server])
            except OSError as exc:  # pragma: no cover - exec failure is fatal for the child
                print(f"exec failed: {exc}", file=sys.stderr)
                os._exit(127)
        conn.close()

    listener.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
