#!/usr/bin/env python3

import argparse
import http.server
import os
import queue
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from typing import Optional

SKIP_RETURN_CODE = 77


def find_free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def find_free_udp_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def find_chrome_binary(explicit: Optional[str]) -> Optional[str]:
    if explicit:
        return explicit

    env_binary = os.environ.get("CHROME") or os.environ.get("GOOGLE_CHROME_SHIM")
    if env_binary:
        return env_binary

    candidates = [
        "google-chrome",
        "google-chrome-stable",
        "chromium",
        "chromium-browser",
        "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
        "/Applications/Chromium.app/Contents/MacOS/Chromium",
    ]
    for candidate in candidates:
        resolved = shutil.which(candidate) if "/" not in candidate else candidate
        if resolved and Path(resolved).exists():
            return resolved
    return None


def read_cert_hash(path: Path) -> list[int]:
    text = path.read_text(encoding="ascii").strip()
    if len(text) != 64:
        raise ValueError(f"expected 64 hex characters in {path}, got {len(text)}")
    return [int(text[i : i + 2], 16) for i in range(0, len(text), 2)]


def render_index(wt_url: str, cert_hash: list[int], stream_count: int, stream_size: int) -> str:
    cert_hash_js = ", ".join(str(value) for value in cert_hash)
    return f"""<!doctype html>
<html>
<body>
<pre id="log"></pre>
<script>
const WT_URL = {wt_url!r};
const CERT_HASH = new Uint8Array([{cert_hash_js}]);
const STREAM_COUNT = {stream_count};
const STREAM_SIZE = {stream_size};
const encoder = new TextEncoder();

function log(message) {{
  console.log(message);
  document.getElementById("log").textContent += message + "\\n";
}}

function mark(id, text) {{
  const node = document.createElement("div");
  node.id = id;
  node.textContent = text || id;
  document.body.appendChild(node);
}}

function fail(error) {{
  console.error(error && error.stack ? error.stack : String(error));
  mark("failed", error && error.message ? error.message : String(error));
}}

function equalBytes(left, right) {{
  if (!left || !right || left.byteLength !== right.byteLength) return false;
  for (let i = 0; i < left.byteLength; i++) {{
    if (left[i] !== right[i]) return false;
  }}
  return true;
}}

function withTimeout(promise, timeoutMs, label) {{
  let timer;
  const timeout = new Promise((_, reject) => {{
    timer = setTimeout(() => reject(new Error(label)), timeoutMs);
  }});
  return Promise.race([promise, timeout]).finally(() => clearTimeout(timer));
}}

async function readAll(readable) {{
  const reader = readable.getReader();
  const chunks = [];
  let total = 0;
  try {{
    while (true) {{
      const result = await reader.read();
      if (result.done) break;
      chunks.push(result.value);
      total += result.value.byteLength;
    }}
  }} finally {{
    reader.releaseLock();
  }}

  const out = new Uint8Array(total);
  let offset = 0;
  for (const chunk of chunks) {{
    out.set(chunk, offset);
    offset += chunk.byteLength;
  }}
  return out;
}}

function makePattern(size) {{
  const data = new Uint8Array(size);
  for (let i = 0; i < data.byteLength; i++) {{
    data[i] = i & 0xff;
  }}
  return data;
}}

async function establishSession() {{
  const transport = new WebTransport(WT_URL, {{
    allowPooling: false,
    requireUnreliable: true,
    serverCertificateHashes: [{{
      algorithm: "sha-256",
      value: CERT_HASH.buffer,
    }}],
  }});

  transport.closed.then(() => {{
    log("transport closed");
  }}).catch((error) => {{
    console.error("transport closed with error", error);
  }});

  await withTimeout(transport.ready, 10000, "WebTransport ready timed out");
  return transport;
}}

async function runDatagramEcho(transport) {{
  const payload = encoder.encode("chrome-datagram");
  const writer = transport.datagrams.writable.getWriter();
  try {{
    await writer.write(payload);
  }} finally {{
    writer.releaseLock();
  }}

  const reader = transport.datagrams.readable.getReader();
  try {{
    const result = await withTimeout(reader.read(), 10000, "datagram echo timed out");
    if (result.done || !equalBytes(result.value, payload)) {{
      throw new Error("datagram echo mismatch");
    }}
  }} finally {{
    reader.releaseLock();
  }}
  log("datagram echo ok");
}}

async function runBidirectionalEcho(transport, text) {{
  const payload = encoder.encode(text);
  const stream = await transport.createBidirectionalStream();
  const writer = stream.writable.getWriter();
  try {{
    await writer.write(payload);
    await writer.close();
  }} finally {{
    writer.releaseLock();
  }}

  const echoed = await withTimeout(readAll(stream.readable), 10000, `${{text}} echo timed out`);
  if (!equalBytes(echoed, payload)) {{
    throw new Error(`${{text}} echo mismatch`);
  }}
  log(`${{text}} echo ok`);
}}

async function runUnidirectionalStreams(transport) {{
  const data = makePattern(STREAM_SIZE);
  for (let i = 0; i < STREAM_COUNT; i++) {{
    const stream = await transport.createUnidirectionalStream();
    const writer = stream.getWriter();
    try {{
      await writer.write(data);
      await writer.close();
    }} finally {{
      writer.releaseLock();
    }}
    log(`unidirectional stream ${{i + 1}}/${{STREAM_COUNT}} sent`);
  }}
}}

(async () => {{
  try {{
    const transport = await establishSession();
    await runDatagramEcho(transport);
    await runBidirectionalEcho(transport, "chrome-bidi");
    await runUnidirectionalStreams(transport);
    await runBidirectionalEcho(transport, "chrome-done");
    mark("done", "ok");
    transport.close();
  }} catch (error) {{
    fail(error);
  }}
}})();
</script>
</body>
</html>
"""


class InteropHandler(http.server.BaseHTTPRequestHandler):
    index_html: bytes = b""

    def do_GET(self) -> None:
        if self.path == "/favicon.ico":
            self.send_response(204)
            self.send_header("content-length", "0")
            self.end_headers()
            return
        if self.path not in ("/", "/index.html"):
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header("content-type", "text/html; charset=utf-8")
        self.send_header("content-length", str(len(self.index_html)))
        self.end_headers()
        self.wfile.write(self.index_html)

    def log_message(self, fmt: str, *args: object) -> None:
        sys.stderr.write("browser interop http: " + (fmt % args) + "\n")


def stream_process_output(process: subprocess.Popen, lines: queue.Queue) -> None:
    assert process.stdout is not None
    for line in process.stdout:
        line = line.rstrip()
        lines.put(line)
        print("browser interop server:", line)


def wait_for_server_ready(process: subprocess.Popen, lines: queue.Queue, timeout: float) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(f"server exited before ready with code {process.returncode}")
        try:
            line = lines.get(timeout=0.1)
        except queue.Empty:
            continue
        if line.startswith("LIBWTF_BROWSER_INTEROP_READY "):
            return
    raise TimeoutError("timed out waiting for interop server readiness")


def wait_for_server_done(process: subprocess.Popen, lines: queue.Queue, timeout: float) -> int:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process.poll() is not None:
            return int(process.returncode or 0)
        try:
            line = lines.get(timeout=0.1)
        except queue.Empty:
            continue
        if line.startswith("LIBWTF_BROWSER_INTEROP_DONE"):
            return 0
    raise TimeoutError("server did not report interop completion")


def create_webdriver(args: argparse.Namespace):
    try:
        from selenium import webdriver
        from selenium.webdriver.chrome.service import Service
    except ImportError:
        print("selenium is not installed; run `uv sync` in tests/browser_chrome", file=sys.stderr)
        sys.exit(SKIP_RETURN_CODE)

    chrome_binary = find_chrome_binary(args.chrome_binary)
    if not chrome_binary:
        print("Chrome/Chromium binary not found", file=sys.stderr)
        sys.exit(SKIP_RETURN_CODE)

    options = webdriver.ChromeOptions()
    options.binary_location = chrome_binary
    if args.headless:
        options.add_argument("--headless=new")
    options.add_argument("--no-sandbox")
    options.add_argument("--disable-gpu")
    options.add_argument("--disable-dev-shm-usage")
    options.add_argument("--enable-quic")
    options.add_argument(f"--origin-to-force-quic-on=127.0.0.1:{args.wt_port}")
    options.add_argument(f"--user-data-dir={args.user_data_dir}")
    options.set_capability("goog:loggingPrefs", {"browser": "ALL"})

    if args.chromedriver:
        return webdriver.Chrome(service=Service(args.chromedriver), options=options)
    return webdriver.Chrome(options=options)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run libwtf Chrome WebTransport interop")
    parser.add_argument("--server-binary", required=True)
    parser.add_argument("--cert", required=True)
    parser.add_argument("--key", required=True)
    parser.add_argument("--hash", required=True)
    parser.add_argument("--wt-port", type=int, default=0)
    parser.add_argument("--http-port", type=int, default=0)
    parser.add_argument("--stream-count", type=int, default=5)
    parser.add_argument("--stream-size", type=int, default=1024 * 1024)
    parser.add_argument("--timeout", type=float, default=15)
    parser.add_argument("--chrome-binary")
    parser.add_argument("--chromedriver")
    parser.add_argument("--no-headless", action="store_true")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()
    args.headless = not args.no_headless
    return args


def main() -> int:
    args = parse_args()
    server_binary = Path(args.server_binary)
    if not server_binary.exists():
        print(f"server binary does not exist: {server_binary}", file=sys.stderr)
        return 2

    cert = Path(args.cert)
    key = Path(args.key)
    cert_hash = read_cert_hash(Path(args.hash))
    if args.wt_port == 0:
        args.wt_port = find_free_udp_port()
    if args.http_port == 0:
        args.http_port = find_free_port()

    wt_url = f"https://127.0.0.1:{args.wt_port}/browser"
    InteropHandler.index_html = render_index(
        wt_url, cert_hash, args.stream_count, args.stream_size
    ).encode("utf-8")

    with tempfile.TemporaryDirectory(prefix="libwtf-chrome-") as tempdir:
        args.user_data_dir = tempdir
        server_cmd = [
            str(server_binary),
            "--port",
            str(args.wt_port),
            "--cert",
            str(cert),
            "--key",
            str(key),
            "--expected-streams",
            str(args.stream_count),
            "--expected-bytes",
            str(args.stream_size),
            "--timeout-ms",
            str(int(args.timeout * 1000)),
        ]
        if args.verbose:
            server_cmd.append("--verbose")

        server = subprocess.Popen(
            server_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        lines: queue.Queue = queue.Queue()
        output_thread = threading.Thread(
            target=stream_process_output, args=(server, lines), daemon=True
        )
        output_thread.start()

        httpd = http.server.ThreadingHTTPServer(("127.0.0.1", args.http_port), InteropHandler)
        http_thread = threading.Thread(target=httpd.serve_forever, daemon=True)
        http_thread.start()

        driver = None
        try:
            wait_for_server_ready(server, lines, 10)
            driver = create_webdriver(args)
            driver.set_page_load_timeout(args.timeout)
            driver.get(f"http://127.0.0.1:{args.http_port}/")

            from selenium.common.exceptions import TimeoutException
            from selenium.webdriver.common.by import By
            from selenium.webdriver.support.ui import WebDriverWait

            def browser_finished(active_driver):
                done = active_driver.find_elements(By.ID, "done")
                failed = active_driver.find_elements(By.ID, "failed")
                return done or failed

            try:
                WebDriverWait(driver, args.timeout, poll_frequency=0.1).until(
                    browser_finished
                )
            except TimeoutException as exc:
                log_text = ""
                try:
                    log_text = driver.find_element(By.ID, "log").text
                except Exception:
                    pass
                raise TimeoutError(f"browser did not finish within {args.timeout:g}s: {log_text}") from exc

            failed = driver.find_elements(By.ID, "failed")
            if failed:
                raise RuntimeError(f"browser reported failure: {failed[0].text}")

            try:
                for entry in driver.get_log("browser"):
                    print("browser console:", entry)
            except Exception:
                pass

            server_code = wait_for_server_done(server, lines, min(args.timeout, 3.0))
            if server_code != 0:
                raise RuntimeError(f"server exited with code {server_code}")
            return 0
        except SystemExit:
            raise
        except Exception as exc:
            print(f"browser interop failed: {exc}", file=sys.stderr)
            if driver is not None:
                try:
                    for entry in driver.get_log("browser"):
                        print("browser console:", entry, file=sys.stderr)
                except Exception:
                    pass
            return 1
        finally:
            if driver is not None:
                driver.quit()
            httpd.shutdown()
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    server.kill()
                    server.wait()


if __name__ == "__main__":
    sys.exit(main())
