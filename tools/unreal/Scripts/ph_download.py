# Poly Haven glTF modellarini (1k teksturalar) yuklab olish -> D:/Unreal_projects/Ertugrul/art/polyhaven/<id>/
import json, os, sys, urllib.request
ROOT = "D:/Unreal_projects/Ertugrul/art/polyhaven"
ids = sys.argv[1:]
UA = {"User-Agent": "Mozilla/5.0 ErtugrulBot"}


def get(url, dest):
    if os.path.exists(dest) and os.path.getsize(dest) > 0:
        return
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=300) as r, open(dest, "wb") as f:
        while True:
            b = r.read(1 << 20)
            if not b: break
            f.write(b)


done = []
lst = os.path.join(ROOT, "downloaded.json")
if os.path.exists(lst):
    done = json.load(open(lst))
for i in ids:
    req = urllib.request.Request("https://api.polyhaven.com/files/" + i, headers=UA)
    d = json.load(urllib.request.urlopen(req, timeout=60))
    g = d["gltf"]["1k"]["gltf"]
    folder = os.path.join(ROOT, i)
    get(g["url"], os.path.join(folder, i + ".gltf"))
    total = g["size"]
    for rel, inc in g.get("include", {}).items():
        get(inc["url"], os.path.join(folder, rel.replace("\\", "/")))
        total += inc["size"]
    print("%s: %.1f MB, %d fayl" % (i, total / 1e6, 1 + len(g.get("include", {}))))
    if i not in done: done.append(i)
json.dump(done, open(lst, "w"))
print("downloaded.json:", done)
