#!/usr/bin/env python3
"""Local dummy authorisation endpoint for UDA HELP::authorise()."""

import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlsplit


ALLOWED_CLAIM = os.environ.get("AUTHZ_ALLOWED_CLAIM", "preferred_username")
ALLOWED_VALUE = os.environ.get("AUTHZ_ALLOWED_VALUE", "alice")


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        url = urlsplit(self.path)
        if url.path == "/health":
            self.reply(200, "OK")
            return
        if url.path != "/authorize":
            self.reply(404, "Not Found")
            return

        query = parse_qs(url.query, keep_blank_values=True)
        allowed = (
            query.get("claim") == [ALLOWED_CLAIM]
            and query.get("value") == [ALLOWED_VALUE]
        )
        self.reply(200, "True" if allowed else "False")

    def reply(self, status, body):
        data = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, format, *args):
        # The query contains a token claim value; do not log it.
        pass


if __name__ == "__main__":
    host = os.environ.get("AUTHZ_HOST", "127.0.0.1")
    port = int(os.environ.get("AUTHZ_PORT", "8765"))
    with ThreadingHTTPServer((host, port), Handler) as server:
        print(f"Dummy authorisation server listening on http://{host}:{port}/authorize", flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass
