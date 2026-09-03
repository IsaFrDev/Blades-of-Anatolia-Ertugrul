// Blades of Anatolia: Ertugrul — daraja importeri (Unity 6, URP).
//
// C++ dvijok `ertugrul.exe --level <id> --export-unity <papka>` bilan
// <id>.level.json chiqaradi (relyef balandliklari, barcha rekvizitlar — scatter
// allaqachon joylashtirilgan, spawn nuqtalari, osmon). Bu skript o'sha JSON dan
// Unity sahnasini quradi: Terrain + rekvizit prefablari + spawn markerlari +
// quyosh (Directional Light) + tuman.
//
// Ishlatish: Unity menyusi  Ertugrul > Import Level JSON...
// Modellar Assets/Ertugrul/Models/ ostida bo'lishi kerak (C++ dagi
// assets/models/ bilan bir xil tuzilma).
using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace Ertugrul.EditorTools
{
    [Serializable] class SkyJ { public float[] sunDir; public float[] sunColor; public float[] ambient; public float[] fogColor; public float fogStart; public float fogEnd; }
    [Serializable] class PropJ { public string mesh; public float[] pos; public float yaw; public float scale; public float[] tint; public bool collide; public float radius; }
    [Serializable] class SpawnJ { public string id; public float[] pos; public float yaw; }
    [Serializable] class LevelJ
    {
        public string id; public string name; public float size; public int heightRes;
        public float minH; public float maxH; public float[] heights;
        public SkyJ sky; public List<PropJ> props; public List<SpawnJ> spawns;
    }

    public static class ErtugrulLevelImporter
    {
        // C++ dvijok o'ng qo'lli (OpenGL), Unity chap qo'lli: Z o'qi teskari.
        // OBJ importeri ham X ni teskari qiladi — shu ikkisi bilan "yurt eshigi
        // qayoqqa qaragani" mos tushadi. Agar sahnada ko'zgu ko'rsangiz, FlipZ ni
        // o'zgartiring.
        const bool FlipZ = true;
        const string ModelsRoot = "Assets/Ertugrul/Models/";

        [MenuItem("Ertugrul/Import Level JSON...")]
        public static void ImportMenu()
        {
            string path = EditorUtility.OpenFilePanel("Ertugrul level JSON", "Assets/Ertugrul/Levels", "json");
            if (string.IsNullOrEmpty(path)) return;
            Import(path);
        }

        // Barcha darajalarni alohida sahnalarga import qiladi (Assets/Ertugrul/Scenes/<id>.unity).
        // Menyu yoki batch rejim:
        //   Unity.exe -batchmode -quit -projectPath "D:\My project"
        //             -executeMethod Ertugrul.EditorTools.ErtugrulLevelImporter.ImportAll
        [MenuItem("Ertugrul/Import ALL levels -> scenes")]
        public static void ImportAll()
        {
            string dir = "Assets/Ertugrul/Levels";
            string sceneDir = "Assets/Ertugrul/Scenes";
            Directory.CreateDirectory(sceneDir);
            var files = Directory.GetFiles(dir, "*.level.json");
            foreach (var f in files)
            {
                var scene = UnityEditor.SceneManagement.EditorSceneManager.NewScene(
                    UnityEditor.SceneManagement.NewSceneSetup.DefaultGameObjects,
                    UnityEditor.SceneManagement.NewSceneMode.Single);
                // Shablon yorug'ligi o'rniga JSON dagi quyosh ishlatiladi
                var old = GameObject.Find("Directional Light");
                if (old != null) UnityEngine.Object.DestroyImmediate(old);
                Import(f.Replace('\\', '/'));
                string id = Path.GetFileName(f).Replace(".level.json", "");
                UnityEditor.SceneManagement.EditorSceneManager.SaveScene(scene, sceneDir + "/" + id + ".unity");
                Debug.Log("Ertugrul: sahna saqlandi " + sceneDir + "/" + id + ".unity");
            }
            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
        }

        public static void Import(string jsonPath)
        {
            var lv = JsonUtility.FromJson<LevelJ>(File.ReadAllText(jsonPath));
            if (lv == null || lv.heights == null) { Debug.LogError("Ertugrul: JSON o'qilmadi: " + jsonPath); return; }

            var root = new GameObject("Level_" + lv.id);
            Undo.RegisterCreatedObjectUndo(root, "Import Ertugrul level");

            // ---------------- Relyef ----------------
            int R = lv.heightRes;
            float range = Mathf.Max(0.01f, lv.maxH - lv.minH);
            var td = new TerrainData();
            td.heightmapResolution = R;
            td.size = new Vector3(lv.size, range, lv.size);
            var hm = new float[R, R];
            for (int r = 0; r < R; ++r)
                for (int c = 0; c < R; ++c)
                {
                    // JSON: qator = z (janubdan shimolga), ustun = x. Unity heightmap [z, x].
                    int rr = FlipZ ? (R - 1 - r) : r;
                    hm[rr, c] = (lv.heights[r * R + c] - lv.minH) / range;
                }
            td.SetHeights(0, 0, hm);
            td.terrainLayers = new[] { GrassLayer() };
            string dir = "Assets/Ertugrul/Levels";
            Directory.CreateDirectory(dir);
            AssetDatabase.CreateAsset(td, dir + "/" + lv.id + "_terrain.asset");
            var tgo = Terrain.CreateTerrainGameObject(td);
            tgo.name = "Terrain";
            tgo.transform.SetParent(root.transform);
            tgo.transform.position = new Vector3(-lv.size * 0.5f, lv.minH, -lv.size * 0.5f);

            // ---------------- Rekvizitlar ----------------
            var propsRoot = new GameObject("Props"); propsRoot.transform.SetParent(root.transform);
            var cache = new Dictionary<string, GameObject>();
            int placed = 0, missing = 0;
            var missingSet = new HashSet<string>();
            foreach (var p in lv.props)
            {
                string rel = p.mesh.Replace("assets/models/", "");
                if (!cache.TryGetValue(rel, out var prefab))
                {
                    prefab = AssetDatabase.LoadAssetAtPath<GameObject>(ModelsRoot + rel);
                    cache[rel] = prefab;
                }
                if (prefab == null) { ++missing; missingSet.Add(rel); continue; }
                var go = (GameObject)PrefabUtility.InstantiatePrefab(prefab, propsRoot.transform);
                go.name = Path.GetFileNameWithoutExtension(rel);
                go.transform.position = W(p.pos);
                go.transform.rotation = Quaternion.Euler(0f, FlipZ ? -p.yaw : p.yaw, 0f);
                go.transform.localScale = Vector3.one * p.scale;
                ApplyTint(go, p.tint);
                if (p.collide)
                {
                    var col = go.AddComponent<CapsuleCollider>();
                    col.radius = p.radius / Mathf.Max(0.01f, p.scale);
                    col.height = 3f / Mathf.Max(0.01f, p.scale);
                    col.center = new Vector3(0f, col.height * 0.5f, 0f);
                }
                ++placed;
            }

            // ---------------- Spawn nuqtalari ----------------
            var spRoot = new GameObject("Spawns"); spRoot.transform.SetParent(root.transform);
            foreach (var s in lv.spawns)
            {
                var go = new GameObject("Spawn_" + s.id);
                go.transform.SetParent(spRoot.transform);
                go.transform.position = W(s.pos);
                // C++ yaw: (sin, 0, cos); Z teskari bo'lganda 180 - yaw (tekshirildi: kameraga oba ko'rinadi)
                go.transform.rotation = Quaternion.Euler(0f, FlipZ ? 180f - s.yaw : s.yaw, 0f);
            }

            // ---------------- Osmon / yorug'lik ----------------
            if (lv.sky != null)
            {
                var sun = new GameObject("Sun").AddComponent<Light>();
                sun.transform.SetParent(root.transform);
                sun.type = LightType.Directional;
                var d = W(lv.sky.sunDir);                       // quyoshga YO'NALGAN vektor
                sun.transform.rotation = Quaternion.LookRotation(-d.normalized, Vector3.up);
                sun.color = C(lv.sky.sunColor);
                sun.intensity = 1.6f;
                sun.shadows = LightShadows.Soft;
                RenderSettings.ambientMode = UnityEngine.Rendering.AmbientMode.Flat;
                RenderSettings.ambientLight = C(lv.sky.ambient);
                RenderSettings.fog = true;
                RenderSettings.fogMode = FogMode.Linear;
                RenderSettings.fogColor = C(lv.sky.fogColor);
                RenderSettings.fogStartDistance = lv.sky.fogStart;
                RenderSettings.fogEndDistance = lv.sky.fogEnd;
            }

            AssetDatabase.SaveAssets();
            Debug.Log($"Ertugrul: '{lv.name}' import qilindi — rekvizit {placed}, topilmagan model {missing}" +
                      (missing > 0 ? ("\n  " + string.Join("\n  ", missingSet)) : ""));
            Selection.activeGameObject = root;
        }

        // Relyef uchun o't qatlami: tekstura yo'q bo'lsa protsedural (shovqinli yashil) yaratiladi.
        static TerrainLayer GrassLayer()
        {
            const string dir = "Assets/Ertugrul/Textures";
            const string tex = dir + "/grass.png";
            const string layer = dir + "/grass.terrainlayer";
            Directory.CreateDirectory(dir);
            if (!File.Exists(tex))
            {
                var t = new Texture2D(256, 256, TextureFormat.RGB24, false);
                var rnd = new System.Random(7);
                for (int y = 0; y < 256; ++y)
                    for (int x = 0; x < 256; ++x)
                    {
                        float n = Mathf.PerlinNoise(x * 0.09f, y * 0.09f) * 0.6f + Mathf.PerlinNoise(x * 0.31f, y * 0.31f) * 0.4f;
                        float j = (float)rnd.NextDouble() * 0.08f;
                        t.SetPixel(x, y, new Color(0.30f + n * 0.22f + j, 0.42f + n * 0.26f + j, 0.16f + n * 0.12f));
                    }
                t.Apply();
                File.WriteAllBytes(tex, t.EncodeToPNG());
                AssetDatabase.ImportAsset(tex);
                var imp = AssetImporter.GetAtPath(tex) as TextureImporter;
                if (imp != null) { imp.wrapMode = TextureWrapMode.Repeat; imp.SaveAndReimport(); }
            }
            var tl = AssetDatabase.LoadAssetAtPath<TerrainLayer>(layer);
            if (tl == null)
            {
                tl = new TerrainLayer();
                tl.diffuseTexture = AssetDatabase.LoadAssetAtPath<Texture2D>(tex);
                tl.tileSize = new Vector2(6f, 6f);
                AssetDatabase.CreateAsset(tl, layer);
            }
            return tl;
        }

        static Vector3 W(float[] v) => new Vector3(v[0], v[1], FlipZ ? -v[2] : v[2]);
        static Color C(float[] v) => new Color(v[0], v[1], v[2], 1f);

        // Kd rangi ustiga tint (C++ da glColor tint * material). URP Lit: _BaseColor.
        static void ApplyTint(GameObject go, float[] tint)
        {
            if (tint == null || tint.Length < 3) return;
            if (Mathf.Abs(tint[0] - 1f) < 0.01f && Mathf.Abs(tint[1] - 1f) < 0.01f && Mathf.Abs(tint[2] - 1f) < 0.01f) return;
            var mpb = new MaterialPropertyBlock();
            foreach (var r in go.GetComponentsInChildren<Renderer>())
            {
                var baseCol = Color.white;
                if (r.sharedMaterial != null && r.sharedMaterial.HasProperty("_BaseColor")) baseCol = r.sharedMaterial.GetColor("_BaseColor");
                else if (r.sharedMaterial != null && r.sharedMaterial.HasProperty("_Color")) baseCol = r.sharedMaterial.GetColor("_Color");
                r.GetPropertyBlock(mpb);
                mpb.SetColor("_BaseColor", baseCol * C(tint));
                mpb.SetColor("_Color", baseCol * C(tint));
                r.SetPropertyBlock(mpb);
            }
        }
    }
}
