# http_echo_server.py
from http.server import BaseHTTPRequestHandler, HTTPServer

HOST = "0.0.0.0"
PORT = 8080

class EchoHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        # Antwort: Methode, Pfad, Query
        self.send_response(200)
        self.send_header("Content-type", "text/plain; charset=utf-8")
        self.end_headers()
        response = f"Echo GET {self.path}\nHeaders:\n{self.headers}"
        self.wfile.write(response.encode("utf-8"))

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8")
        print(body)
        print(self.headers)

        self.send_response(200)
        self.send_header("Content-type", "text/plain; charset=utf-8")
        self.end_headers()
        response = f"Echo POST {self.path}\nHeaders:\n{self.headers}\nBody:\n{body}"
        self.wfile.write(response.encode("utf-8"))

def run():
    server = HTTPServer((HOST, PORT), EchoHandler)
    print(f"HTTP Echo Server läuft auf http://{HOST}:{PORT} (STRG-C zum beenden)")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServer beendet.")

if __name__ == "__main__":
    run()
