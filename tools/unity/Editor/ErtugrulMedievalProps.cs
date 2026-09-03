// O'rta asr rekvizitlari — Asset Store'siz, protsedural (teksturalar ham kodda):
//   • Kiyiz O'TOV (yurt): panjara devor, konus tom, tunduk halqasi, eshik, arqonlar
//     — Kenney kubik yurt/chodirlar o'rniga, joyi va o'lchami saqlanadi
//   • TOSH QAL'A (Karacahisar uslubi): 4 devor + tishlar, 4 burchak minorasi,
//     darvoza minoralari — So'g'ut sahnasining chekkasida (o'yinda ko'rinadigan orientir)
//   • So'g'ut uylari: yog'och devorlar tosh-poydevor rangiga (UV siz Kenney meshlar — faqat rang)
// Menyu: Ertugrul > Medieval Props (current scene) / (ALL scenes)
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;

namespace Ertugrul.EditorTools
{
    public static class ErtugrulMedievalProps
    {
        const string GenDir = "Assets/Ertugrul/Generated";

        [MenuItem("Ertugrul/Medieval Props (current scene)")]
        public static void ApplyCurrent() { Apply(); EditorSceneManager.SaveOpenScenes(); }

        [MenuItem("Ertugrul/Medieval Props (ALL scenes)")]
        public static void ApplyAll()
        {
            foreach (var f in Directory.GetFiles("Assets/Ertugrul/Scenes", "*.unity"))
            {
                var sc = EditorSceneManager.OpenScene(f.Replace('\\', '/'), OpenSceneMode.Single);
                Apply();
                EditorSceneManager.SaveScene(sc);
            }
        }

        public static void Apply()
        {
            Directory.CreateDirectory(GenDir);
            int y = ReplaceYurts();
            int w = StoneVillage();
            string level = "";
            foreach (var go in Object.FindObjectsByType<GameObject>(FindObjectsSortMode.None))
                if (go.name.StartsWith("Level_")) { level = go.name.Substring(6); break; }
            bool fort = false;
            if (level == "sogut_village" || level == "aleppo_road") fort = BuildFortress(level);
            Debug.Log("Ertugrul: o'tov " + y + ", tosh devor " + w + ", qal'a " + fort + " (" + level + ")");
        }

        // ------------------------------------------------------------------ teksturalar
        static Texture2D Tex(string name, int size, System.Func<float, float, Color> f)
        {
            string path = GenDir + "/" + name + ".png";
            var existing = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
            if (existing != null) return existing;
            var t = new Texture2D(size, size, TextureFormat.RGB24, false);
            for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) t.SetPixel(x, y, f((float)x / size, (float)y / size));
            t.Apply();
            File.WriteAllBytes(path, t.EncodeToPNG());
            AssetDatabase.ImportAsset(path);
            var imp = AssetImporter.GetAtPath(path) as TextureImporter;
            if (imp != null) { imp.wrapMode = TextureWrapMode.Repeat; imp.SaveAndReimport(); }
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }
        static float Fbm(float x, float y, int oct = 4)
        {
            float n = 0f, a = 0.5f, fr = 1f;
            for (int i = 0; i < oct; ++i) { n += a * Mathf.PerlinNoise(x * fr, y * fr); a *= 0.5f; fr *= 2f; }
            return n;
        }
        static Texture2D FeltTex() => Tex("t_felt", 512, (u, v) =>
        {
            float fiber = Mathf.PerlinNoise(u * 90f, v * 6f) * 0.5f + Fbm(u * 8f + 3f, v * 8f) * 0.5f;
            float g = 0.62f + fiber * 0.22f;
            return new Color(g, g * 0.92f, g * 0.78f);
        });
        static Texture2D StoneTex() => Tex("t_stone", 512, (u, v) =>
        {
            // g'isht-tosh qatorlari: har qator siljigan bloklar, choklar qorong'i
            float row = v * 8f; int r = Mathf.FloorToInt(row);
            float col = u * 6f + (r % 2 == 0 ? 0f : 0.5f);
            float fy = row - r, fx = col - Mathf.Floor(col);
            float edge = Mathf.Min(Mathf.Min(fx, 1f - fx) * 6f, Mathf.Min(fy, 1f - fy) * 8f);
            float mortar = Mathf.Clamp01(edge * 2.2f);
            float n = Fbm(u * 14f + r * 0.7f, v * 14f + Mathf.Floor(col) * 1.3f);
            float g = Mathf.Lerp(0.28f, 0.42f + n * 0.30f, mortar);
            return new Color(g, g * 0.95f, g * 0.86f);
        });
        static Texture2D PlankTex() => Tex("t_plank", 512, (u, v) =>
        {
            float grain = Mathf.PerlinNoise(u * 4f, v * 80f) * 0.5f + Fbm(u * 6f, v * 40f) * 0.5f;
            float plank = Mathf.Abs(Mathf.Sin(u * Mathf.PI * 6f)) < 0.06f ? 0.6f : 1f;
            float g = (0.36f + grain * 0.22f) * plank;
            return new Color(g * 1.15f, g * 0.85f, g * 0.55f);
        });
        static Material Mat(string name, Texture2D tex, Color col, float smooth, Vector2 tiling)
        {
            string path = GenDir + "/" + name + ".mat";
            var m = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (m == null) { m = new Material(Shader.Find("Universal Render Pipeline/Lit")); AssetDatabase.CreateAsset(m, path); }
            m.SetTexture("_BaseMap", tex); m.SetColor("_BaseColor", col); m.SetFloat("_Smoothness", smooth); m.SetFloat("_Metallic", 0f);
            m.SetTextureScale("_BaseMap", tiling); m.enableInstancing = true;
            EditorUtility.SetDirty(m);
            return m;
        }

        // ------------------------------------------------------------------ primitivlar
        static GameObject Prim(PrimitiveType t, Transform parent, Vector3 pos, Vector3 scale, Material m, Vector3? euler = null)
        {
            var go = GameObject.CreatePrimitive(t);
            go.transform.SetParent(parent, false);
            go.transform.localPosition = pos; go.transform.localScale = scale;
            if (euler.HasValue) go.transform.localRotation = Quaternion.Euler(euler.Value);
            go.GetComponent<MeshRenderer>().sharedMaterial = m;
            Object.DestroyImmediate(go.GetComponent<Collider>());
            return go;
        }

        static Mesh ConeMesh(float r, float h, int seg, float rTop = 0f)
        {
            var m = new Mesh { name = "cone" };
            var v = new List<Vector3>(); var n = new List<Vector3>(); var uv = new List<Vector2>(); var tri = new List<int>();
            for (int i = 0; i <= seg; ++i)
            {
                float a = i * Mathf.PI * 2f / seg;
                var d = new Vector3(Mathf.Cos(a), 0, Mathf.Sin(a));
                var nn = (d * h + Vector3.up * (r - rTop)).normalized;
                v.Add(d * r); n.Add(nn); uv.Add(new Vector2((float)i / seg * 6f, 0));
                v.Add(d * rTop + Vector3.up * h); n.Add(nn); uv.Add(new Vector2((float)i / seg * 6f, 2f));
            }
            for (int i = 0; i < seg; ++i) { int b = i * 2; tri.AddRange(new[] { b, b + 1, b + 2, b + 2, b + 1, b + 3 }); }
            m.SetVertices(v); m.SetNormals(n); m.SetUVs(0, uv); m.SetTriangles(tri, 0); m.RecalculateBounds();
            return m;
        }

        // ------------------------------------------------------------------ O'TOV
        static GameObject YurtPrefab(int variant, bool open)
        {
            string path = GenDir + "/yurt_" + variant + (open ? "_open" : "") + ".prefab";
            var existing = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            if (existing != null) return existing;
            var felt = Mat("m_felt", FeltTex(), new Color(0.92f, 0.88f, 0.78f), 0.03f, new Vector2(3f, 1f));
            var feltRoof = Mat("m_felt_roof", FeltTex(), new Color(0.80f, 0.74f, 0.62f), 0.03f, new Vector2(1f, 1f));
            var wood = Mat("m_wood", PlankTex(), Color.white, 0.10f, new Vector2(1f, 1f));
            var rope = Mat("m_rope", PlankTex(), new Color(0.55f, 0.45f, 0.30f), 0.05f, new Vector2(1f, 8f));

            float R = 1f, wallH = 0.42f, roofH = 0.36f;   // birlik radius; keyin masshtablanadi
            var root = new GameObject("yurt");
            // devor (silindr) — panjara ko'rinishi uchun felt + tashqi ingichka yog'och halqalar
            Prim(PrimitiveType.Cylinder, root.transform, new Vector3(0, wallH * 0.5f, 0), new Vector3(R * 2f, wallH * 0.5f, R * 2f), felt);
            for (int k = 0; k < 2; ++k)
                Prim(PrimitiveType.Cylinder, root.transform, new Vector3(0, wallH * (0.25f + 0.5f * k), 0), new Vector3(R * 2.03f, 0.012f, R * 2.03f), rope);
            // tom (konus)
            var roofGo = new GameObject("roof"); roofGo.transform.SetParent(root.transform, false);
            roofGo.transform.localPosition = new Vector3(0, wallH, 0);
            var cone = ConeMesh(R * 1.06f, roofH, 28, R * 0.12f);
            AssetDatabase.CreateAsset(cone, GenDir + "/mesh_yurt_roof_" + variant + ".asset");
            roofGo.AddComponent<MeshFilter>().sharedMesh = cone;
            roofGo.AddComponent<MeshRenderer>().sharedMaterial = feltRoof;
            // tunduk (tepa halqa)
            Prim(PrimitiveType.Cylinder, root.transform, new Vector3(0, wallH + roofH, 0), new Vector3(R * 0.30f, 0.02f, R * 0.30f), wood);
            // tom arqonlari (8 ta)
            for (int i = 0; i < 8; ++i)
            {
                float a = i * Mathf.PI * 2f / 8f + variant * 0.3f;
                var p0 = new Vector3(Mathf.Cos(a) * R * 1.05f, wallH + 0.02f, Mathf.Sin(a) * R * 1.05f);
                var p1 = new Vector3(Mathf.Cos(a) * R * 0.14f, wallH + roofH, Mathf.Sin(a) * R * 0.14f);
                var mid = (p0 + p1) * 0.5f; var dir = p1 - p0;
                var g = Prim(PrimitiveType.Cylinder, root.transform, mid, new Vector3(0.012f, dir.magnitude * 0.5f, 0.012f), rope);
                g.transform.localRotation = Quaternion.FromToRotation(Vector3.up, dir.normalized);
            }
            // eshik (+Z): yog'och rom va qanot
            Prim(PrimitiveType.Cube, root.transform, new Vector3(-0.15f * R, wallH * 0.5f, R * 0.99f), new Vector3(0.03f, wallH * 0.95f, 0.05f), wood);
            Prim(PrimitiveType.Cube, root.transform, new Vector3(0.15f * R, wallH * 0.5f, R * 0.99f), new Vector3(0.03f, wallH * 0.95f, 0.05f), wood);
            Prim(PrimitiveType.Cube, root.transform, new Vector3(0, wallH * 0.95f, R * 0.99f), new Vector3(0.34f * R, 0.03f, 0.05f), wood);
            if (!open) Prim(PrimitiveType.Cube, root.transform, new Vector3(0, wallH * 0.47f, R * 1.0f), new Vector3(0.28f * R, wallH * 0.9f, 0.02f), wood);
            var col = root.AddComponent<CapsuleCollider>(); col.radius = R; col.height = wallH + roofH; col.center = new Vector3(0, (wallH + roofH) * 0.5f, 0);
            var prefab = PrefabUtility.SaveAsPrefabAsset(root, path);
            Object.DestroyImmediate(root);
            return prefab;
        }

        static int ReplaceYurts()
        {
            var props = GameObject.Find("Props");
            if (props == null) return 0;
            var list = new List<Transform>();
            foreach (Transform t in props.transform) list.Add(t);
            var rnd = new System.Random(9);
            int n = 0;
            foreach (var t in list)
            {
                string nm = t.name.ToLower();
                if (!(nm.StartsWith("yurt_") || nm.StartsWith("tent_")) || nm.EndsWith("_real")) continue;
                var rs = t.GetComponentsInChildren<Renderer>();
                if (rs.Length == 0) continue;
                var b = rs[0].bounds; foreach (var r in rs) b.Encapsulate(r.bounds);
                float radius = Mathf.Max(b.extents.x, b.extents.z) * 0.95f;
                float height = b.size.y;
                bool open = nm.Contains("open");
                var prefab = YurtPrefab(rnd.Next(0, 3), open);
                var go = (GameObject)PrefabUtility.InstantiatePrefab(prefab, props.transform);
                go.transform.position = new Vector3(b.center.x, b.min.y, b.center.z);
                go.transform.rotation = t.rotation;
                // prefab: radius 1, balandlik 0.78 -> real: radius, balandlik ~ radius*0.78 (o'tov nisbati), lekin Kenney balandligidan oshmasin
                float s = radius;
                float sy = Mathf.Min(radius, height / 0.78f);
                go.transform.localScale = new Vector3(s, Mathf.Max(sy, radius * 0.75f), s);
                go.name = t.name + "_real";
                Object.DestroyImmediate(t.gameObject);
                ++n;
            }
            return n;
        }

        // ------------------------------------------------------------------ So'g'ut: tosh poydevor ranglari
        static int StoneVillage()
        {
            var props = GameObject.Find("Props");
            if (props == null) return 0;
            int n = 0;
            foreach (var r in props.GetComponentsInChildren<MeshRenderer>())
            {
                string nm = r.transform.parent != null ? r.transform.parent.name.ToLower() + "/" + r.name.ToLower() : r.name.ToLower();
                if (!nm.Contains("wall-wood") && !nm.Contains("wall_")) continue;
                var mpb = new MaterialPropertyBlock();
                r.GetPropertyBlock(mpb);
                // pastki devorlar — ohak-tosh, ustki — quruq yog'och
                bool block = nm.Contains("block") || nm.Contains("corner");
                mpb.SetColor("_BaseColor", block ? new Color(0.72f, 0.68f, 0.60f, 1f) : new Color(0.62f, 0.46f, 0.30f, 1f));
                mpb.SetFloat("_Smoothness", 0.06f);
                r.SetPropertyBlock(mpb); ++n;
            }
            return n;
        }

        // ------------------------------------------------------------------ QAL'A
        static float Ground(float x, float z)
        {
            var t = Terrain.activeTerrain;
            return t != null ? t.SampleHeight(new Vector3(x, 0, z)) + t.transform.position.y : 0f;
        }

        static bool BuildFortress(string level)
        {
            var old = GameObject.Find("Fortress"); if (old != null) Object.DestroyImmediate(old);
            var stone = Mat("m_stone", StoneTex(), Color.white, 0.06f, new Vector2(1f, 1f));
            var stoneDark = Mat("m_stone_dark", StoneTex(), new Color(0.75f, 0.72f, 0.68f), 0.06f, new Vector2(1f, 1f));
            var wood = Mat("m_wood", PlankTex(), Color.white, 0.10f, new Vector2(1f, 1f));

            // joy: qishloqdan 75 m sharqda, eng baland tekis joy
            Vector3 c = new Vector3(78f, 0f, level == "sogut_village" ? -30f : 40f);
            float half = 21f;
            float gmax = -1e9f;
            for (int i = -2; i <= 2; ++i) for (int j = -2; j <= 2; ++j) gmax = Mathf.Max(gmax, Ground(c.x + i * half * 0.5f, c.z + j * half * 0.5f));
            c.y = gmax + 0.2f;
            var root = new GameObject("Fortress");
            root.transform.position = c;
            float wallH = 7f, wallT = 1.6f, towerR = 3.4f, towerH = 11f;
            // 4 devor (janubda darvoza ochiq)
            Vector3[] centers = { new Vector3(0, 0, half), new Vector3(0, 0, -half), new Vector3(half, 0, 0), new Vector3(-half, 0, 0) };
            for (int w = 0; w < 4; ++w)
            {
                bool ns = w < 2; bool gate = (w == 1);
                if (!gate)
                {
                    var g = Prim(PrimitiveType.Cube, root.transform, centers[w] + Vector3.up * (wallH * 0.5f - 2f),
                                 ns ? new Vector3(half * 2f, wallH + 4f, wallT) : new Vector3(wallT, wallH + 4f, half * 2f), stone);
                    g.GetComponent<MeshRenderer>().sharedMaterial = stone;
                    g.name = "wall" + w; g.AddComponent<BoxCollider>();
                    SetTiling(g, ns ? half * 2f / 4f : half * 2f / 4f, (wallH + 4f) / 4f);
                }
                else
                {
                    float segLen = half - 3.2f;   // darvoza uchun 6.4 m ochiq
                    foreach (float sgn in new[] { -1f, 1f })
                    {
                        var g = Prim(PrimitiveType.Cube, root.transform, centers[w] + new Vector3(sgn * (3.2f + segLen * 0.5f), wallH * 0.5f - 2f, 0),
                                     new Vector3(segLen, wallH + 4f, wallT), stone);
                        g.name = "gatewall"; g.AddComponent<BoxCollider>(); SetTiling(g, segLen / 4f, (wallH + 4f) / 4f);
                    }
                    // darvoza minoralari va yog'och darvoza (ochiq)
                    foreach (float sgn in new[] { -1f, 1f })
                    {
                        var tw = Prim(PrimitiveType.Cylinder, root.transform, centers[w] + new Vector3(sgn * 4.4f, towerH * 0.5f - 2f, 0),
                                      new Vector3(2.6f * 2f, (towerH + 4f) * 0.5f, 2.6f * 2f), stoneDark);
                        tw.name = "gatetower"; tw.AddComponent<CapsuleCollider>();
                        Crenels(root.transform, centers[w] + new Vector3(sgn * 4.4f, towerH, 0), 2.6f, stone, 10);
                    }
                    var lintel = Prim(PrimitiveType.Cube, root.transform, centers[w] + new Vector3(0, wallH - 0.6f, 0), new Vector3(6.6f, 1.4f, wallT), stone);
                    lintel.name = "lintel"; lintel.AddComponent<BoxCollider>();
                    var door = Prim(PrimitiveType.Cube, root.transform, centers[w] + new Vector3(-2.9f, (wallH - 1.3f) * 0.5f, 0.9f), new Vector3(0.18f, wallH - 1.3f, 2.8f), wood, new Vector3(0, 20f, 0));
                    door.name = "door_l";
                }
                // tishlar (crenellation)
                if (!gate)
                {
                    int count = Mathf.RoundToInt(half * 2f / 2.2f);
                    for (int i = 0; i < count; ++i)
                    {
                        float off = -half + 1.1f + i * 2.2f;
                        var p = ns ? centers[w] + new Vector3(off, wallH + 0.55f, 0) : centers[w] + new Vector3(0, wallH + 0.55f, off);
                        Prim(PrimitiveType.Cube, root.transform, p, ns ? new Vector3(1.1f, 1.1f, wallT + 0.1f) : new Vector3(wallT + 0.1f, 1.1f, 1.1f), stone);
                    }
                    // yurish yo'li (ichkarida)
                    var walk = Prim(PrimitiveType.Cube, root.transform, centers[w] * 0.93f + Vector3.up * (wallH - 1.2f),
                                    ns ? new Vector3(half * 2f - 4f, 0.4f, 1.8f) : new Vector3(1.8f, 0.4f, half * 2f - 4f), wood);
                    walk.name = "walk"; walk.AddComponent<BoxCollider>();
                }
            }
            // burchak minoralari
            foreach (var sx in new[] { -1f, 1f }) foreach (var sz in new[] { -1f, 1f })
            {
                var p = new Vector3(sx * half, towerH * 0.5f - 2f, sz * half);
                var tw = Prim(PrimitiveType.Cylinder, root.transform, p, new Vector3(towerR * 2f, (towerH + 4f) * 0.5f, towerR * 2f), stoneDark);
                tw.name = "tower"; tw.AddComponent<CapsuleCollider>();
                Crenels(root.transform, new Vector3(sx * half, towerH, sz * half), towerR, stone, 12);
                // konus tom
                var roofGo = new GameObject("towerroof"); roofGo.transform.SetParent(root.transform, false);
                roofGo.transform.localPosition = new Vector3(sx * half, towerH + 1.0f, sz * half);
                var cone = ConeMesh(towerR * 0.95f, 3.2f, 16);
                roofGo.AddComponent<MeshFilter>().sharedMesh = cone;
                roofGo.AddComponent<MeshRenderer>().sharedMaterial = wood;
            }
            // ichki hovli: markaziy bino (donjon)
            var keep = Prim(PrimitiveType.Cube, root.transform, new Vector3(0, 5f - 2f, -4f), new Vector3(12f, 14f, 9f), stone);
            keep.name = "keep"; keep.AddComponent<BoxCollider>(); SetTiling(keep, 3f, 3.5f);
            var keepRoof = new GameObject("keeproof"); keepRoof.transform.SetParent(root.transform, false);
            keepRoof.transform.localPosition = new Vector3(0, 12f, -4f);
            var kc = ConeMesh(7.5f, 4f, 4); keepRoof.AddComponent<MeshFilter>().sharedMesh = kc; keepRoof.AddComponent<MeshRenderer>().sharedMaterial = wood;
            keepRoof.transform.localRotation = Quaternion.Euler(0, 45f, 0);
            // bayroq
            Prim(PrimitiveType.Cylinder, root.transform, new Vector3(0, 12f + 4f + 2f, -4f), new Vector3(0.12f, 2.2f, 0.12f), wood);
            var flag = Prim(PrimitiveType.Cube, root.transform, new Vector3(0.9f, 12f + 4f + 3.5f, -4f), new Vector3(1.8f, 1.0f, 0.04f), Mat("m_flag", null, new Color(0.62f, 0.10f, 0.10f), 0.2f, Vector2.one));
            flag.name = "flag";
            // relyefga o'rnashish: poydevor (devorlar 4 m pastga cho'zilgan)
            return true;
        }

        static void Crenels(Transform root, Vector3 top, float r, Material m, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                float a = i * Mathf.PI * 2f / count;
                var p = top + new Vector3(Mathf.Cos(a) * r, 0.5f, Mathf.Sin(a) * r);
                var g = Prim(PrimitiveType.Cube, root, p, new Vector3(0.9f, 1.0f, 0.5f), m, new Vector3(0, -a * Mathf.Rad2Deg, 0));
                g.name = "crenel";
            }
        }

        static void SetTiling(GameObject g, float u, float v)
        {
            var r = g.GetComponent<MeshRenderer>();
            var mpb = new MaterialPropertyBlock(); r.GetPropertyBlock(mpb);
            mpb.SetVector("_BaseMap_ST", new Vector4(u, v, 0, 0)); r.SetPropertyBlock(mpb);
        }
    }
}
