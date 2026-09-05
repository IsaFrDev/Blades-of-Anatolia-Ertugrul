# -*- coding: utf-8 -*-
# Unreal MCP orqali Python kodini ishga tushirish: python mcp_py.py <script.py>
import sys, json, urllib.request, io

URL = "http://127.0.0.1:8000/mcp"

def post(body, sid=None):
    req = urllib.request.Request(URL, data=json.dumps(body).encode("utf-8"), method="POST")
    req.add_header("Content-Type", "application/json")
    req.add_header("Accept", "application/json, text/event-stream")
    if sid: req.add_header("Mcp-Session-Id", sid)
    with urllib.request.urlopen(req, timeout=600) as r:
        text = r.read().decode("utf-8", "replace")
        return r.headers.get("Mcp-Session-Id"), text

def parse(text):
    text = text.strip()
    if text.startswith("event:") or text.startswith("data:"):
        for line in text.splitlines():
            if line.startswith("data:"):
                text = line[5:].strip()
    return json.loads(text) if text else {}

code = io.open(sys.argv[1], encoding="utf-8").read()
sid, _ = post({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"protocolVersion": "2025-03-26", "capabilities": {}, "clientInfo": {"name": "claude-cli", "version": "1"}}})
post({"jsonrpc": "2.0", "method": "notifications/initialized"}, sid)
_, text = post({"jsonrpc": "2.0", "id": 2, "method": "tools/call", "params": {"name": "execute_python_code", "arguments": {"code": code}}}, sid)
d = parse(text)
res = d.get("result", d)
for c in res.get("content", []):
    if c.get("type") == "text":
        print(c["text"])
if res.get("isError"):
    print("ISERROR")
