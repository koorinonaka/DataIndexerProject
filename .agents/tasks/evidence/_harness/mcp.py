# Minimal UE MCP HTTP client (stdlib only). Session id cached in %TEMP%\ue_mcp_session.txt.
#   python mcp.py list                      -> tool names
#   python mcp.py call <tool> '<json args>' -> result text
#   python mcp.py schema <tool>             -> input schema
#   python mcp.py ts <Toolset.Path.tool> '<json args>' -> call a toolset tool via call_tool
#   python mcp.py cmd '<console command>'  -> type into the status-bar Cmd box (e.g. 'py print(1)')
import http.client, json, os, sys, tempfile, urllib.error, urllib.parse

URL = os.environ.get("UE_MCP_URL", "http://127.0.0.1:8001/mcp")
SID_FILE = os.path.join(tempfile.gettempdir(), "ue_mcp_session.txt")
TIMEOUT = int(os.environ.get("UE_MCP_TIMEOUT", "320"))


def post(body, sid=None):
    # http.client, not urllib: urllib sends "Connection: close" and the UE server then drops the
    # SSE body of tools/call (headers arrive, body empty).
    h = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
    if sid:
        h["Mcp-Session-Id"] = sid
    u = urllib.parse.urlparse(URL)
    c = http.client.HTTPConnection(u.hostname, u.port, timeout=TIMEOUT)
    c.request("POST", u.path, json.dumps(body).encode(), h)
    r = c.getresponse()
    if r.status >= 400:
        raise urllib.error.HTTPError(URL, r.status, r.reason, None, None)
    new_sid = r.getheader("Mcp-Session-Id") or sid
    raw = ""
    if "id" in body:  # requests: read SSE until the response line arrives
        while True:
            line = r.fp.readline().decode("utf-8", "replace")
            if not line:
                break
            raw += line
            if line.startswith("data:") or (line.lstrip().startswith("{") and line.rstrip().endswith("}")):
                break
    c.close()
    if not raw.strip():
        return None, new_sid
    if raw.lstrip().startswith("{"):
        return json.loads(raw), new_sid
    msg = None  # SSE: take the last data: line carrying a JSON-RPC response
    for line in raw.splitlines():
        if line.startswith("data:"):
            try:
                msg = json.loads(line[5:].strip())
            except ValueError:
                pass
    return msg, new_sid


def session():
    if os.path.exists(SID_FILE):
        return open(SID_FILE).read().strip()
    _, sid = post({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {
        "protocolVersion": "2025-03-26", "capabilities": {},
        "clientInfo": {"name": "harness", "version": "1"}}})
    post({"jsonrpc": "2.0", "method": "notifications/initialized"}, sid)
    open(SID_FILE, "w").write(sid or "")
    return sid


def rpc(method, params):
    for attempt in range(2):
        sid = session()
        try:
            msg, _ = post({"jsonrpc": "2.0", "id": 2, "method": method, "params": params}, sid)
            if msg and "error" in msg and attempt == 0 and "session" in json.dumps(msg["error"]).lower():
                raise urllib.error.HTTPError(URL, 404, "session", None, None)
            return msg
        except urllib.error.HTTPError as e:
            if attempt == 0 and e.code in (400, 404):
                os.remove(SID_FILE)
                continue
            raise


def ts(tool, args):
    t, n = tool.rsplit(".", 1)
    msg = rpc("tools/call", {"name": "call_tool", "arguments": {"toolset_name": t, "tool_name": n, "arguments": args}})
    return json.loads(msg["result"]["content"][0]["text"]).get("returnValue")


def console(command):
    # The Cmd box ref changes with the front tab: re-find the textbox at the bottom-left (y > 950, x < 700).
    import re
    s = ts("SlateInspectorToolset.SlateInspectorToolset.Snapshot", {"ref": "", "maxDepth": 40})
    for m in re.finditer(r"textbox[^\n]*\[pos=(\d+),(\d+)[^\n]*\[ref=(\w+)\]", s):
        if int(m.group(2)) > 950 and int(m.group(1)) < 700:
            return ts("SlateInspectorToolset.SlateInspectorToolset.Type", {"ref": m.group(3), "text": command, "submit": True})
    raise SystemExit("Cmd textbox not found")


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    cmd = sys.argv[1]
    if cmd == "list":
        for t in rpc("tools/list", {})["result"]["tools"]:
            print(t["name"], "-", (t.get("description") or "").split("\n")[0][:120])
    elif cmd == "cmd":
        print(console(sys.argv[2]))
    elif cmd == "schema":
        for t in rpc("tools/list", {})["result"]["tools"]:
            if t["name"] == sys.argv[2]:
                print(json.dumps(t, ensure_ascii=False, indent=1))
    else:
        args = json.loads(sys.argv[3]) if len(sys.argv) > 3 else {}
        name = sys.argv[2]
        if cmd == "ts":
            ts, tool = name.rsplit(".", 1)
            args, name = {"toolset_name": ts, "tool_name": tool, "arguments": args}, "call_tool"
        msg = rpc("tools/call", {"name": name, "arguments": args})
        if msg is None:
            print("ERROR no response (connection closed; modal open?)"); sys.exit(2)
        if "error" in msg:
            print("ERROR", json.dumps(msg["error"], ensure_ascii=False)); sys.exit(1)
        for c in msg["result"].get("content", []):
            print(c.get("text", json.dumps(c, ensure_ascii=False)[:2000]))
        if msg["result"].get("isError"):
            sys.exit(1)
