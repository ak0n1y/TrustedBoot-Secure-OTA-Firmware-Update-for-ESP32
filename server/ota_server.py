import argparse
import functools
import http.server
import ssl
from pathlib import Path


class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


def main():
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--bind", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8443)
    parser.add_argument("--cert", default=str(root / "certs" / "server.crt"))
    parser.add_argument("--key", default=str(root / "certs" / "server.key"))
    parser.add_argument("--dir", default=str(root / "firmware"))
    args = parser.parse_args()

    handler = functools.partial(Handler, directory=args.dir)
    httpd = http.server.ThreadingHTTPServer((args.bind, args.port), handler)

    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile=args.cert, keyfile=args.key)
    httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

    print(f"serving {args.dir} on https://{args.bind}:{args.port}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        httpd.server_close()


if __name__ == "__main__":
    main()
