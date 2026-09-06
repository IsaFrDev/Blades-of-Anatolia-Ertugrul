import socket,json,sys,io
code=io.open(sys.argv[1],encoding="utf-8").read()
s=socket.create_connection(("localhost",9876),timeout=1500)
s.sendall(json.dumps({"type":"execute_code","params":{"code":code}}).encode())
data=b""
while True:
    ch=s.recv(65536)
    if not ch: break
    data+=ch
    try: r=json.loads(data.decode()); break
    except Exception: continue
print(json.dumps(r,ensure_ascii=False)[:3000])
