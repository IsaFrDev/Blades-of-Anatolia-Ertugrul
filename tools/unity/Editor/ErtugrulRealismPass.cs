// Realizm o'timi — "Ertugrul of Ulukayin" (Steam) uslubiga yaqinlashtirish:
//   • relyef: 3 qatlam (o't / quruq tuproq / qoya) — nishab, balandlik va shovqin bo'yicha splat
//   • relyef detal o'ti (tebranuvchi barg kartalari) — protsedural tekstura
//   • daraxtlar: Kenney kubik daraxtlar o'rniga protsedural realistik daraxtlar
//     (tana + barg kartalari, alpha-clip, 3 variant) — joyi/o'lchami saqlanadi
//   • qoya/to'nka materiallari: mot, shovqinli tekstura
//   • osmon: protsedural skybox + quyosh diski, eksponensial tuman, iliq quyosh
//   • post-processing: ACES tonemapping, bloom, rang, vinyet (URP Volume)
//   • URP: soya masofasi 160 m, 4 kaskad, yumshoq soya
//   • ko'l: relyefning eng past joyida suv yuzasi
// Menyu: Ertugrul > Realism Pass (current scene) / (ALL scenes)
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace Ertugrul.EditorTools
{
    public static class ErtugrulRealismPass
    {
        const string GenDir = "Assets/Ertugrul/Generated";

        [MenuItem("Ertugrul/Realism Pass (current scene)")]
        public static void ApplyCurrent() { Apply(); EditorSceneManager.MarkSceneDirty(EditorSceneManager.GetActiveScene()); EditorSceneManager.SaveOpenScenes(); }

        [MenuItem("Ertugrul/Realism Pass (ALL scenes)")]
        public static void ApplyAll()
        {
            foreach (var f in Directory.GetFiles("Assets/Ertugrul/Scenes", "*.unity"))
            {
                var sc = EditorSceneManager.OpenScene(f.Replace('\\', '/'), OpenSceneMode.Single);
                Apply();
                EditorSceneManager.SaveScene(sc);
                Debug.Log("Ertugrul: realizm o'timi " + f);
            }
        }

        public static void Apply()
        {
            Directory.CreateDirectory(GenDir);
            var terrain = Terrain.activeTerrain;
            if (terrain == null) { Debug.LogWarning("Ertugrul: sahnada Terrain yo'q"); return; }
            PipelineSettings();
            TerrainLayers(terrain);
            DetailGrass(terrain);
            ReplaceTrees();
            RockMaterials();
            SkyAndFog();
            PostProcessing();
            Lake(terrain);
            Debug.Log("Ertugrul: realizm o'timi bajarildi");
        }

        // ------------------------------------------------------------------ URP
        static void PipelineSettings()
        {
            var rp = GraphicsSettings.defaultRenderPipeline as UniversalRenderPipelineAsset;
            if (rp == null) return;
            rp.shadowDistance = 160f;
            rp.shadowCascadeCount = 4;
            { var so = new SerializedObject(rp); var pr = so.FindProperty("m_SoftShadowsSupported"); if (pr != null) { pr.boolValue = true; so.ApplyModifiedPropertiesWithoutUndo(); } }
            rp.supportsHDR = true;
            rp.msaaSampleCount = 4;
            EditorUtility.SetDirty(rp);
            var cam = Camera.main;
            if (cam != null)
            {
                var ad = cam.GetUniversalAdditionalCameraData();
                ad.renderPostProcessing = true;
                ad.antialiasing = AntialiasingMode.SubpixelMorphologicalAntiAliasing;
                cam.allowHDR = true;
            }
        }

        // ------------------------------------------------------------------ teksturalar
        static Texture2D NoiseTex(string name, int size, System.Func<float, float, float, Color> f, int seed, bool alpha = false)
        {
            string path = GenDir + "/" + name + ".png";
            var existing = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
            if (existing != null) return existing;
            var t = new Texture2D(size, size, alpha ? TextureFormat.RGBA32 : TextureFormat.RGB24, false);
            var rnd = new System.Random(seed);
            float ox = seed * 13.7f, oy = seed * 7.1f;
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                {
                    float u = (float)x / size, v = (float)y / size;
                    // tileable: ikkita oktava, chekkalar mos kelishi uchun sin bilan
                    float n = 0f, a = 0.5f, fr = 4f;
                    for (int o = 0; o < 4; ++o) { n += a * Mathf.PerlinNoise(ox + u * fr, oy + v * fr); a *= 0.5f; fr *= 2f; }
                    n = Mathf.Clamp01(n * 1.15f);
                    t.SetPixel(x, y, f(u, v, n));
                }
            t.Apply();
            File.WriteAllBytes(path, t.EncodeToPNG());
            AssetDatabase.ImportAsset(path);
            var imp = AssetImporter.GetAtPath(path) as TextureImporter;
            if (imp != null) { imp.wrapMode = TextureWrapMode.Repeat; imp.alphaIsTransparency = alpha; imp.mipmapEnabled = true; imp.SaveAndReimport(); }
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }

        static TerrainLayer Layer(string name, Texture2D tex, float tile, float smooth)
        {
            string path = GenDir + "/" + name + ".terrainlayer";
            var tl = AssetDatabase.LoadAssetAtPath<TerrainLayer>(path);
            if (tl == null) { tl = new TerrainLayer(); AssetDatabase.CreateAsset(tl, path); }
            tl.diffuseTexture = tex; tl.tileSize = new Vector2(tile, tile); tl.smoothness = smooth; tl.metallic = 0f;
            EditorUtility.SetDirty(tl);
            return tl;
        }

        // ------------------------------------------------------------------ relyef qatlamlari
        static void TerrainLayers(Terrain terrain)
        {
            var td = terrain.terrainData;
            var grass = NoiseTex("t_grass", 512, (u, v, n) => new Color(0.24f + n * 0.20f, 0.36f + n * 0.24f, 0.12f + n * 0.10f), 11);
            var dirt  = NoiseTex("t_dirt", 512, (u, v, n) => new Color(0.42f + n * 0.20f, 0.33f + n * 0.16f, 0.20f + n * 0.10f), 23);
            var rock  = NoiseTex("t_rock", 512, (u, v, n) => { float g = 0.30f + n * 0.32f; return new Color(g, g * 0.96f, g * 0.90f); }, 37);
            td.terrainLayers = new[] { Layer("L_grass", grass, 5f, 0.05f), Layer("L_dirt", dirt, 6f, 0.02f), Layer("L_rock", rock, 9f, 0.08f) };

            int R = td.alphamapResolution = 256;
            var map = new float[R, R, 3];
            float minH = 1e9f, maxH = -1e9f;
            for (int y = 0; y < R; ++y) for (int x = 0; x < R; ++x)
            { float h = td.GetInterpolatedHeight((float)x / (R - 1), (float)y / (R - 1)); minH = Mathf.Min(minH, h); maxH = Mathf.Max(maxH, h); }
            float span = Mathf.Max(1f, maxH - minH);
            for (int y = 0; y < R; ++y)
                for (int x = 0; x < R; ++x)
                {
                    float u = (float)x / (R - 1), v = (float)y / (R - 1);
                    float slope = td.GetSteepness(u, v) / 90f;                   // 0..1
                    float h01 = (td.GetInterpolatedHeight(u, v) - minH) / span;
                    float n = Mathf.PerlinNoise(u * 18f + 3f, v * 18f + 7f);
                    float rockW = Mathf.Clamp01((slope - 0.28f) * 3.2f) + Mathf.Clamp01((h01 - 0.55f) * 2f) * 0.6f;
                    float dirtW = Mathf.Clamp01((n - 0.45f) * 2.2f) * (1f - rockW) * 0.7f + Mathf.Clamp01((slope - 0.12f) * 2f) * 0.5f;
                    dirtW = Mathf.Clamp01(dirtW) * (1f - rockW);
                    float grassW = Mathf.Clamp01(1f - rockW - dirtW);
                    float sum = grassW + dirtW + rockW;
                    map[y, x, 0] = grassW / sum; map[y, x, 1] = dirtW / sum; map[y, x, 2] = rockW / sum;
                }
            td.SetAlphamaps(0, 0, map);
        }

        // ------------------------------------------------------------------ detal o't
        static void DetailGrass(Terrain terrain)
        {
            var td = terrain.terrainData;
            // Barg kartasi: bir nechta ingichka barglar, alpha
            var blade = NoiseTex("t_grassblade", 256, (u, v, n) =>
            {
                float a = 0f;
                for (int k = 0; k < 7; ++k)
                {
                    float cx = 0.08f + k * 0.14f + Mathf.Sin(k * 3.1f) * 0.03f;
                    float sway = (v * v) * 0.10f * ((k % 2 == 0) ? 1f : -1f);
                    float w = Mathf.Lerp(0.035f, 0.004f, v);
                    float d = Mathf.Abs(u - cx - sway);
                    if (d < w && v < 0.97f) a = Mathf.Max(a, 1f);
                }
                float g = 0.32f + n * 0.30f + v * 0.15f;
                return new Color(g * 0.75f, g, g * 0.35f, a);
            }, 51, true);
            var grassMesh = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Ertugrul/Models/nature/grass_large.obj")
                         ?? AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Ertugrul/Models/nature/grass.obj");
            var proto = new DetailPrototype
            {
                prototype = grassMesh, usePrototypeMesh = grassMesh != null, renderMode = DetailRenderMode.VertexLit,
                prototypeTexture = grassMesh != null ? null : blade,
                minWidth = 0.7f, maxWidth = 1.3f, minHeight = 0.6f, maxHeight = 1.1f,
                healthyColor = new Color(0.55f, 0.75f, 0.30f), dryColor = new Color(0.72f, 0.66f, 0.30f), noiseSpread = 0.25f,
                useInstancing = grassMesh != null
            };
            td.detailPrototypes = new[] { proto };
            int R = 256;
            td.SetDetailResolution(R, 16);
            var alpha = td.GetAlphamaps(0, 0, td.alphamapWidth, td.alphamapHeight);
            var layer = new int[R, R];
            var rnd = new System.Random(5);
            for (int y = 0; y < R; ++y)
                for (int x = 0; x < R; ++x)
                {
                    int ax = Mathf.Clamp(x * td.alphamapWidth / R, 0, td.alphamapWidth - 1);
                    int ay = Mathf.Clamp(y * td.alphamapHeight / R, 0, td.alphamapHeight - 1);
                    float g = alpha[ay, ax, 0];
                    float n = Mathf.PerlinNoise(x * 0.07f, y * 0.07f);
                    int d = Mathf.RoundToInt(g * (6f + 6f * n));
                    layer[y, x] = d + (rnd.NextDouble() < 0.5 ? 0 : 1);
                }
            td.SetDetailLayer(0, 0, 0, layer);
            terrain.detailObjectDistance = 120f;
            terrain.detailObjectDensity = 1.0f;
            terrain.drawInstanced = true;
        }

        // ------------------------------------------------------------------ daraxtlar
        static Mesh TrunkMesh(float h, float r0, float r1, int seg = 10)
        {
            var m = new Mesh { name = "trunk" };
            var v = new List<Vector3>(); var nrm = new List<Vector3>(); var uv = new List<Vector2>(); var tri = new List<int>();
            int rings = 5;
            for (int j = 0; j <= rings; ++j)
            {
                float t = (float)j / rings; float y = t * h; float r = Mathf.Lerp(r0, r1, t);
                float bend = Mathf.Sin(t * 2.2f) * 0.06f * h;
                for (int i = 0; i <= seg; ++i)
                {
                    float a = i * Mathf.PI * 2f / seg;
                    var dir = new Vector3(Mathf.Cos(a), 0, Mathf.Sin(a));
                    v.Add(dir * r + new Vector3(bend, y, 0)); nrm.Add(dir); uv.Add(new Vector2((float)i / seg * 2f, t * 3f));
                }
            }
            for (int j = 0; j < rings; ++j) for (int i = 0; i < seg; ++i)
            {
                int a = j * (seg + 1) + i, b = a + seg + 1;
                tri.AddRange(new[] { a, b, a + 1, a + 1, b, b + 1 });
            }
            m.SetVertices(v); m.SetNormals(nrm); m.SetUVs(0, uv); m.SetTriangles(tri, 0); m.RecalculateBounds();
            return m;
        }

        static Mesh CanopyMesh(System.Random rnd, float baseY, float height, float radius, int cards)
        {
            var m = new Mesh { name = "canopy" };
            var v = new List<Vector3>(); var nrm = new List<Vector3>(); var uv = new List<Vector2>(); var tri = new List<int>();
            for (int c = 0; c < cards; ++c)
            {
                float t = (float)c / cards;
                float y = baseY + (float)rnd.NextDouble() * height;
                float rr = radius * (1f - Mathf.Abs((y - baseY) / height - 0.45f) * 0.9f) * (0.5f + (float)rnd.NextDouble() * 0.6f);
                float a = (float)rnd.NextDouble() * Mathf.PI * 2f;
                var center = new Vector3(Mathf.Cos(a) * rr * 0.6f, y, Mathf.Sin(a) * rr * 0.6f);
                var n = new Vector3(Mathf.Cos(a), 0.35f, Mathf.Sin(a)).normalized;
                var right = Vector3.Cross(Vector3.up, n).normalized;
                var up = Vector3.Cross(n, right).normalized;
                float s = radius * (0.55f + (float)rnd.NextDouble() * 0.5f);
                int b = v.Count;
                v.Add(center - right * s - up * s * 0.6f); v.Add(center + right * s - up * s * 0.6f);
                v.Add(center + right * s + up * s * 0.6f); v.Add(center - right * s + up * s * 0.6f);
                for (int k = 0; k < 4; ++k) nrm.Add((center - new Vector3(0, baseY + height * 0.4f, 0)).normalized * 0.7f + Vector3.up * 0.3f);
                uv.Add(new Vector2(0, 0)); uv.Add(new Vector2(1, 0)); uv.Add(new Vector2(1, 1)); uv.Add(new Vector2(0, 1));
                tri.AddRange(new[] { b, b + 2, b + 1, b, b + 3, b + 2 });
            }
            m.SetVertices(v); m.SetNormals(nrm); m.SetUVs(0, uv); m.SetTriangles(tri, 0); m.RecalculateBounds();
            return m;
        }

        static Material Mat(string name, Texture2D tex, Color col, float smooth, bool alphaClip, bool twoSided)
        {
            string path = GenDir + "/" + name + ".mat";
            var m = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (m == null) { m = new Material(Shader.Find("Universal Render Pipeline/Lit")); AssetDatabase.CreateAsset(m, path); }
            m.SetTexture("_BaseMap", tex); m.SetColor("_BaseColor", col); m.SetFloat("_Smoothness", smooth); m.SetFloat("_Metallic", 0f);
            if (alphaClip) { m.SetFloat("_AlphaClip", 1f); m.SetFloat("_Cutoff", 0.45f); m.EnableKeyword("_ALPHATEST_ON"); }
            if (twoSided) m.SetFloat("_Cull", (float)CullMode.Off);
            EditorUtility.SetDirty(m);
            return m;
        }

        static GameObject TreePrefab(int variant, bool pine)
        {
            string path = GenDir + "/tree_" + (pine ? "pine" : "oak") + variant + ".prefab";
            var existing = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            if (existing != null) return existing;
            var rnd = new System.Random(100 + variant + (pine ? 50 : 0));
            var bark = NoiseTex("t_bark", 256, (u, v, n) => { float g = 0.22f + n * 0.25f; float stripe = 0.85f + 0.15f * Mathf.Sin(u * 60f + n * 8f); return new Color(g * 1.1f * stripe, g * 0.85f * stripe, g * 0.6f * stripe); }, 71);
            var leaf = NoiseTex(pine ? "t_needles" : "t_leaves", 256, (u, v, n) =>
            {
                float dx = u - 0.5f, dy = v - 0.5f;
                float r = Mathf.Sqrt(dx * dx + dy * dy);
                float edge = 0.30f + 0.16f * Mathf.PerlinNoise(u * 9f, v * 9f) + (pine ? 0.05f * Mathf.Sin(Mathf.Atan2(dy, dx) * 14f) : 0f);
                float a = r < edge ? 1f : 0f;
                if (pine) { float ray = Mathf.Abs(Mathf.Sin(Mathf.Atan2(dy, dx) * 22f)); if (ray < 0.25f && r > 0.14f) a = 0f; }
                else if (Mathf.PerlinNoise(u * 40f, v * 40f) > 0.72f && r > 0.12f) a = 0f;
                float g = pine ? 0.20f + n * 0.20f : 0.30f + n * 0.32f;
                return pine ? new Color(g * 0.55f, g, g * 0.55f, a) : new Color(g * 0.72f, g, g * 0.30f, a);
            }, pine ? 83 : 91, true);
            var barkMat = Mat("m_bark", bark, Color.white, 0.05f, false, false);
            var leafMat = Mat(pine ? "m_needles" : "m_leaves", leaf, Color.white, 0.12f, true, true);

            float h = pine ? 9f + variant * 1.5f : 6.5f + variant * 1.2f;
            var root = new GameObject("tree");
            var trunkGo = new GameObject("trunk"); trunkGo.transform.SetParent(root.transform, false);
            var trunk = TrunkMesh(pine ? h * 0.95f : h * 0.55f, pine ? 0.22f : 0.30f, 0.06f);
            AssetDatabase.CreateAsset(trunk, GenDir + "/mesh_trunk_" + (pine ? "p" : "o") + variant + ".asset");
            trunkGo.AddComponent<MeshFilter>().sharedMesh = trunk;
            trunkGo.AddComponent<MeshRenderer>().sharedMaterial = barkMat;
            var canGo = new GameObject("canopy"); canGo.transform.SetParent(root.transform, false);
            Mesh canopy = pine ? CanopyMesh(rnd, h * 0.25f, h * 0.7f, 1.7f + variant * 0.2f, 34)
                               : CanopyMesh(rnd, h * 0.45f, h * 0.55f, 2.6f + variant * 0.3f, 26);
            AssetDatabase.CreateAsset(canopy, GenDir + "/mesh_canopy_" + (pine ? "p" : "o") + variant + ".asset");
            canGo.AddComponent<MeshFilter>().sharedMesh = canopy;
            var mr = canGo.AddComponent<MeshRenderer>(); mr.sharedMaterial = leafMat; mr.shadowCastingMode = ShadowCastingMode.On;
            var col = root.AddComponent<CapsuleCollider>(); col.radius = 0.35f; col.height = h; col.center = new Vector3(0, h * 0.5f, 0);
            var prefab = PrefabUtility.SaveAsPrefabAsset(root, path);
            Object.DestroyImmediate(root);
            return prefab;
        }

        static void ReplaceTrees()
        {
            var props = GameObject.Find("Props");
            if (props == null) return;
            var rnd = new System.Random(3);
            int n = 0;
            var list = new List<Transform>();
            foreach (Transform t in props.transform) list.Add(t);
            foreach (var t in list)
            {
                string nm = t.name.ToLower();
                if (!nm.StartsWith("tree_") || nm.EndsWith("_real")) continue;
                bool pine = nm.Contains("pine");
                var prefab = TreePrefab(rnd.Next(0, 3), pine);
                var go = (GameObject)PrefabUtility.InstantiatePrefab(prefab, props.transform);
                go.transform.position = t.position;
                go.transform.rotation = Quaternion.Euler(0f, (float)rnd.NextDouble() * 360f, 0f);
                // Kenney daraxti ~ 1 birlik balandlikda edi, scale = balandlik (m). Yangi daraxt ~7-11 m: masshtab 1
                float s = Mathf.Clamp(t.localScale.x / 6f, 0.55f, 1.6f);
                go.transform.localScale = Vector3.one * s;
                go.name = t.name + "_real";
                Object.DestroyImmediate(t.gameObject);
                ++n;
            }
            Debug.Log("Ertugrul: " + n + " daraxt almashtirildi");
        }

        // ------------------------------------------------------------------ qoyalar
        static void RockMaterials()
        {
            var rockMat = Mat("m_rock_plain", null, new Color(0.50f, 0.46f, 0.41f), 0.10f, false, false);
            var props = GameObject.Find("Props");
            if (props == null) return;
            int n = 0;
            foreach (var r in props.GetComponentsInChildren<MeshRenderer>())
            {
                string nm = r.transform.root.name.ToLower() + "/" + r.gameObject.name.ToLower();
                if (nm.Contains("rock") || nm.Contains("stone"))
                {
                    // UV siz meshlar — teksturasiz mot material; MPB tozalanadi (eski tint qolmasin)
                    var mats = r.sharedMaterials;
                    for (int i = 0; i < mats.Length; ++i) mats[i] = rockMat;
                    r.sharedMaterials = mats;
                    r.SetPropertyBlock(null); ++n;
                }
                else
                {
                    // Kenney rekvizitlari: yaltiroqlikni o'chirish (plastik ko'rinmasin)
                    var mpb = new MaterialPropertyBlock();
                    r.GetPropertyBlock(mpb); mpb.SetFloat("_Smoothness", 0.08f); r.SetPropertyBlock(mpb);
                }
            }
            Debug.Log("Ertugrul: " + n + " qoya materiali");
        }

        // ------------------------------------------------------------------ osmon
        static void SkyAndFog()
        {
            string path = GenDir + "/m_sky.mat";
            var sky = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (sky == null) { sky = new Material(Shader.Find("Skybox/Procedural")); AssetDatabase.CreateAsset(sky, path); }
            sky.SetFloat("_SunSize", 0.035f); sky.SetFloat("_SunSizeConvergence", 6f);
            sky.SetFloat("_AtmosphereThickness", 1.05f); sky.SetColor("_SkyTint", new Color(0.5f, 0.5f, 0.5f));
            sky.SetColor("_GroundColor", new Color(0.36f, 0.34f, 0.30f)); sky.SetFloat("_Exposure", 1.0f);
            RenderSettings.skybox = sky;
            RenderSettings.ambientMode = AmbientMode.Trilight;
            RenderSettings.ambientSkyColor = new Color(0.52f, 0.62f, 0.80f);
            RenderSettings.ambientEquatorColor = new Color(0.45f, 0.44f, 0.40f);
            RenderSettings.ambientGroundColor = new Color(0.20f, 0.18f, 0.15f);
            RenderSettings.fog = true;
            RenderSettings.fogMode = FogMode.ExponentialSquared;
            RenderSettings.fogDensity = 0.0032f;
            RenderSettings.fogColor = new Color(0.72f, 0.76f, 0.82f);
            var sun = GameObject.Find("Sun");
            if (sun != null)
            {
                var l = sun.GetComponent<Light>();
                l.intensity = 1.9f; l.color = new Color(1.0f, 0.93f, 0.82f);
                l.shadows = LightShadows.Soft; l.shadowStrength = 0.9f; l.shadowBias = 0.02f; l.shadowNormalBias = 0.4f;
                RenderSettings.sun = l;
                // quyosh past bo'lsa (shom) uni biroz ko'taramiz — soyalar cho'zilib xunuk bo'lmasin
                var e = sun.transform.eulerAngles;
                if (e.x < 18f || e.x > 180f) sun.transform.rotation = Quaternion.Euler(24f, e.y, 0f);
            }
        }

        // ------------------------------------------------------------------ post-processing
        static void PostProcessing()
        {
            var old = GameObject.Find("PostProcessVolume");
            if (old != null) Object.DestroyImmediate(old);
            string path = GenDir + "/pp_profile.asset";
            var profile = AssetDatabase.LoadAssetAtPath<VolumeProfile>(path);
            if (profile == null) { profile = ScriptableObject.CreateInstance<VolumeProfile>(); AssetDatabase.CreateAsset(profile, path); }
            T Comp<T>() where T : VolumeComponent
            {
                if (!profile.TryGet<T>(out var c)) { c = profile.Add<T>(true); AssetDatabase.AddObjectToAsset(c, profile); }
                return c;
            }
            var tone = Comp<Tonemapping>(); tone.mode.Override(TonemappingMode.ACES);
            var bloom = Comp<Bloom>(); bloom.intensity.Override(0.35f); bloom.threshold.Override(1.1f); bloom.scatter.Override(0.6f);
            var ca = Comp<ColorAdjustments>(); ca.postExposure.Override(0.25f); ca.contrast.Override(12f); ca.saturation.Override(8f);
            var vig = Comp<Vignette>(); vig.intensity.Override(0.28f); vig.smoothness.Override(0.5f);
            var wb = Comp<WhiteBalance>(); wb.temperature.Override(6f);
            var smh = Comp<ShadowsMidtonesHighlights>(); smh.shadows.Override(new Vector4(0.95f, 0.97f, 1.05f, 0f));
            EditorUtility.SetDirty(profile);
            var go = new GameObject("PostProcessVolume");
            var vol = go.AddComponent<Volume>(); vol.isGlobal = true; vol.priority = 1; vol.sharedProfile = profile;
        }

        // ------------------------------------------------------------------ ko'l
        static void Lake(Terrain terrain)
        {
            var old = GameObject.Find("Lake");
            if (old != null) Object.DestroyImmediate(old);
            var td = terrain.terrainData;
            // eng past nuqta (chekkadan 15% ichkarida)
            float best = 1e9f; Vector2 bu = Vector2.zero;
            for (int y = 8; y < 56; ++y) for (int x = 8; x < 56; ++x)
            {
                float u = x / 63f, v = y / 63f;
                float h = td.GetInterpolatedHeight(u, v);
                if (h < best) { best = h; bu = new Vector2(u, v); }
            }
            float center = td.GetInterpolatedHeight(0.5f, 0.5f);
            if (center - best < 1.2f) return;                 // chuqurlik yo'q — ko'l yo'q
            var go = GameObject.CreatePrimitive(PrimitiveType.Plane);
            go.name = "Lake";
            Object.DestroyImmediate(go.GetComponent<Collider>());
            var pos = terrain.transform.position + new Vector3(bu.x * td.size.x, best + 0.9f, bu.y * td.size.z);
            go.transform.position = pos;
            go.transform.localScale = new Vector3(2.6f, 1f, 2.6f);   // 26 m
            var wave = NoiseTex("t_wave_n", 256, (u, v, n) => new Color(0.5f + (n - 0.5f) * 0.4f, 0.5f + (Mathf.PerlinNoise(v * 9f, u * 9f) - 0.5f) * 0.4f, 1f), 61);
            string mp = GenDir + "/m_water.mat";
            var m = AssetDatabase.LoadAssetAtPath<Material>(mp);
            if (m == null) { m = new Material(Shader.Find("Universal Render Pipeline/Lit")); AssetDatabase.CreateAsset(m, mp); }
            m.SetFloat("_Surface", 1f); m.SetFloat("_Blend", 0f); m.renderQueue = 3000;
            m.SetOverrideTag("RenderType", "Transparent"); m.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");
            m.SetInt("_SrcBlend", (int)BlendMode.SrcAlpha); m.SetInt("_DstBlend", (int)BlendMode.OneMinusSrcAlpha); m.SetInt("_ZWrite", 0);
            m.SetColor("_BaseColor", new Color(0.10f, 0.25f, 0.30f, 0.82f));
            m.SetFloat("_Smoothness", 0.96f); m.SetFloat("_Metallic", 0.05f);
            m.SetTexture("_BumpMap", wave); m.EnableKeyword("_NORMALMAP"); m.SetFloat("_BumpScale", 0.35f);
            m.SetTextureScale("_BumpMap", new Vector2(8f, 8f));
            EditorUtility.SetDirty(m);
            go.GetComponent<MeshRenderer>().sharedMaterial = m;
            go.AddComponent<Ertugrul.ErtugrulWater>();
        }
    }
}
