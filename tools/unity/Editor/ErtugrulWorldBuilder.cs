// DUNYO XARITASI — foydalanuvchi bergan eskiz bo'yicha (4 mintaqa, bitta katta relyef):
//   NW  o'rmon + QAYI OBASI (aniq reja: 250x250 m, markazda Bey chodiri 12 m, o'tovlar 5 m,
//       asosiy yo'l 4 m N-S, temirchi 15x15, mashq maydoni 20x20, janubda darvoza, 4 qorovul minorasi)
//   NE  qorli tog'lar + TOSH QAL'A (Karacahisar) — tog' tepasida, ko'prik bilan
//   SW  devorli SHAHAR (sakkiz burchakli devor, minoralar, oltin gumbaz, minoralar) — dasht
//   SE  MO'G'UL LAGERI — qora yer, chodirlar, gulxanlar, bayroqlar
//   W   daryo,  markaz — chorraha, yo'llar hammasini bog'laydi
// Sahna: Assets/Ertugrul/Scenes/world.unity.  Menyu: Ertugrul > Build WORLD map
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace Ertugrul.EditorTools
{
    public static class ErtugrulWorldBuilder
    {
        // ---- o'lchamlar (m) — epizodlar o'sgan sari mintaqalar kengaytiriladi
        public const float WorldSize = 2000f;
        public const int   HeightRes = 1025;
        public static readonly Vector2 ObaCenter   = new Vector2(-560f,  560f);   // NW
        public static readonly Vector2 FortCenter  = new Vector2( 620f,  660f);   // NE (tog' tepasi)
        public static readonly Vector2 CityCenter  = new Vector2(-470f, -470f);   // SW
        public static readonly Vector2 CampCenter  = new Vector2( 520f, -460f);   // SE
        public static readonly Vector2 Crossroads  = new Vector2(  10f,   40f);
        const float ObaHalf = 125f, CityR = 190f, CampR = 150f, FortHalf = 34f;

        static float[,] heights;
        static float cellM;

        [MenuItem("Ertugrul/Build WORLD map (2000 m)")]
        public static void Build()
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            Directory.CreateDirectory("Assets/Ertugrul/Generated");
            Directory.CreateDirectory("Assets/Ertugrul/Scenes");
            var root = new GameObject("Level_world");

            var terrain = BuildTerrain(root.transform);
            var props = new GameObject("Props"); props.transform.SetParent(root.transform);
            BuildRoads(terrain);
            BuildRiver(root.transform, terrain);
            BuildOba(props.transform);
            BuildCity(props.transform);
            BuildCamp(props.transform);
            BuildFortress(props.transform);
            PlaceTrees(props.transform);
            PlaceRocks(props.transform);
            Spawns(root.transform);

            // yorug'lik / osmon / post-processing (RealismPass dan)
            var sun = new GameObject("Sun").AddComponent<Light>();
            sun.transform.SetParent(root.transform);
            sun.type = LightType.Directional; sun.transform.rotation = Quaternion.Euler(38f, 140f, 0f);
            sun.color = new Color(1f, 0.94f, 0.84f); sun.intensity = 1.9f; sun.shadows = LightShadows.Soft;
            RenderSettings.sun = sun;
            ErtugrulRealismPass.PipelineSettings();
            ErtugrulRealismPass.SkyAndFog();
            ErtugrulRealismPass.PostProcessing();
            RenderSettings.fogDensity = 0.0016f;                 // katta xarita — tuman siyrakroq

            EditorSceneManager.SaveScene(scene, "Assets/Ertugrul/Scenes/world.unity");
            ErtugrulGameplaySetup.Setup();
            EditorSceneManager.SaveScene(scene);
            Debug.Log("Ertugrul: DUNYO xaritasi qurildi: Assets/Ertugrul/Scenes/world.unity");
        }

        // ================================================================== RELYEF
        static float Fbm(float x, float y, int oct = 5)
        {
            float n = 0f, a = 0.5f, fr = 1f;
            for (int i = 0; i < oct; ++i) { n += a * Mathf.PerlinNoise(x * fr + 31.7f, y * fr + 17.3f); a *= 0.5f; fr *= 2f; }
            return n;
        }
        static float Mask(Vector2 p, Vector2 c, float inner, float outer)
        {
            float d = Vector2.Distance(p, c);
            return 1f - Mathf.SmoothStep(0f, 1f, Mathf.InverseLerp(inner, outer, d));
        }
        // dunyo (x,z) -> balandlik funksiyasi
        static float HeightAt(float x, float z)
        {
            Vector2 p = new Vector2(x, z);
            float h = Fbm(x * 0.0012f, z * 0.0012f) * 26f + Fbm(x * 0.006f, z * 0.006f, 3) * 7f;
            // NE tog'lar (qorli) + NE/E chekka tizmalari
            float mtn = Mask(p, new Vector2(700f, 720f), 120f, 520f);
            h += mtn * (150f + Fbm(x * 0.004f, z * 0.004f) * 90f);
            float edge = Mathf.Max(Mathf.InverseLerp(0.55f, 1f, Mathf.Abs(x) / (WorldSize * 0.5f)),
                                   Mathf.InverseLerp(0.55f, 1f, Mathf.Abs(z) / (WorldSize * 0.5f)));
            h += Mathf.SmoothStep(0f, 1f, edge) * 70f * (0.6f + 0.4f * Fbm(x * 0.003f, z * 0.003f));
            // NW o'rmonli tepaliklar
            h += Mask(p, ObaCenter + new Vector2(60f, -40f), 250f, 700f) * Fbm(x * 0.003f, z * 0.003f) * 38f;
            // SW dasht — tekis; SE lager — biroz notekis
            h *= Mathf.Lerp(1f, 0.35f, Mask(p, CityCenter, 300f, 650f));
            // tekislangan joylar: oba, shahar, lager, chorraha, qal'a tepaligi
            h = Mathf.Lerp(h, 34f, Mask(p, ObaCenter, ObaHalf * 1.05f, ObaHalf * 1.6f));
            h = Mathf.Lerp(h, 10f, Mask(p, CityCenter, CityR * 1.05f, CityR * 1.7f));
            h = Mathf.Lerp(h, 22f, Mask(p, CampCenter, CampR * 1.05f, CampR * 1.7f));
            h = Mathf.Lerp(h, 26f, Mask(p, Crossroads, 60f, 160f));
            float fortM = Mask(p, FortCenter, FortHalf * 1.5f, FortHalf * 4.5f);
            h = Mathf.Lerp(h, 232f, fortM);                       // plato tog'ning eng baland joyi
            h -= Mask(p, FortCenter, FortHalf * 4.5f, FortHalf * 9f) * (1f - fortM) * 25f;   // atrof pastroq — cho'qqi ajralib tursin
            return h;
        }

        static Terrain BuildTerrain(Transform parent)
        {
            var td = new TerrainData();
            td.heightmapResolution = HeightRes;
            const float maxH = 320f;
            td.size = new Vector3(WorldSize, maxH, WorldSize);
            heights = new float[HeightRes, HeightRes];
            cellM = WorldSize / (HeightRes - 1);
            for (int r = 0; r < HeightRes; ++r)
                for (int c = 0; c < HeightRes; ++c)
                {
                    float x = -WorldSize * 0.5f + c * cellM, z = -WorldSize * 0.5f + r * cellM;
                    heights[r, c] = Mathf.Clamp01(HeightAt(x, z) / maxH);
                }
            // yo'llar va daryo keyin relyefni o'zgartiradi — SetHeights BuildRoads/BuildRiver dan keyin
            td.SetHeights(0, 0, heights);
            AssetDatabase.CreateAsset(td, "Assets/Ertugrul/Generated/world_terrain.asset");
            var tgo = Terrain.CreateTerrainGameObject(td);
            tgo.name = "Terrain"; tgo.transform.SetParent(parent);
            tgo.transform.position = new Vector3(-WorldSize * 0.5f, 0f, -WorldSize * 0.5f);
            var t = tgo.GetComponent<Terrain>();
            t.drawInstanced = true; t.heightmapPixelError = 6f; t.basemapDistance = 900f;
            return t;
        }

        static void ApplyLayers(Terrain t, System.Func<float, float, float> roadMask)
        {
            var td = t.terrainData;
            var grass = ErtugrulRealismPass.NoiseTex("t_grass", 512, (u, v, n) => new Color(0.24f + n * 0.20f, 0.36f + n * 0.24f, 0.12f + n * 0.10f), 11);
            var dirt  = ErtugrulRealismPass.NoiseTex("t_dirt", 512, (u, v, n) => new Color(0.42f + n * 0.20f, 0.33f + n * 0.16f, 0.20f + n * 0.10f), 23);
            var rock  = ErtugrulRealismPass.NoiseTex("t_rock", 512, (u, v, n) => { float g = 0.30f + n * 0.32f; return new Color(g, g * 0.96f, g * 0.90f); }, 37);
            var snow  = ErtugrulRealismPass.NoiseTex("t_snow", 512, (u, v, n) => { float g = 0.82f + n * 0.16f; return new Color(g, g, g * 1.02f); }, 41);
            var ash   = ErtugrulRealismPass.NoiseTex("t_ash", 512, (u, v, n) => { float g = 0.10f + n * 0.14f; return new Color(g * 1.1f, g, g * 0.95f); }, 43);
            var steppe= ErtugrulRealismPass.NoiseTex("t_steppe", 512, (u, v, n) => new Color(0.58f + n * 0.22f, 0.48f + n * 0.18f, 0.24f + n * 0.10f), 47);
            td.terrainLayers = new[] {
                ErtugrulRealismPass.Layer("L_grass", grass, 5f, 0.05f), ErtugrulRealismPass.Layer("L_dirt", dirt, 6f, 0.02f),
                ErtugrulRealismPass.Layer("L_rock", rock, 9f, 0.08f), ErtugrulRealismPass.Layer("L_snow", snow, 7f, 0.25f),
                ErtugrulRealismPass.Layer("L_ash", ash, 6f, 0.02f), ErtugrulRealismPass.Layer("L_steppe", steppe, 6f, 0.03f) };
            int R = td.alphamapResolution = 1024;
            var map = new float[R, R, 6];
            for (int y = 0; y < R; ++y)
                for (int x = 0; x < R; ++x)
                {
                    float u = (float)x / (R - 1), v = (float)y / (R - 1);
                    float wx = -WorldSize * 0.5f + u * WorldSize, wz = -WorldSize * 0.5f + v * WorldSize;
                    float h = td.GetInterpolatedHeight(u, v);
                    float slope = td.GetSteepness(u, v) / 90f;
                    Vector2 p = new Vector2(wx, wz);
                    float snowW = Mathf.Clamp01((h - 150f) / 30f);
                    float rockW = Mathf.Clamp01((slope - 0.30f) * 3f) * (1f - snowW) + Mathf.Clamp01((h - 95f) / 40f) * (1f - snowW) * 0.7f;
                    float ashW  = Mask(p, CampCenter, CampR * 0.9f, CampR * 1.6f) * (1f - rockW);
                    float stepW = Mask(p, CityCenter, CityR * 1.2f, 700f) * (0.55f + 0.45f * Fbm(wx * 0.01f, wz * 0.01f, 2)) * (1f - rockW - ashW);
                    float road  = roadMask(wx, wz);
                    float dirtW = Mathf.Max(road, Mathf.Clamp01((Fbm(wx * 0.02f, wz * 0.02f, 2) - 0.5f) * 2.5f) * 0.5f) * (1f - rockW - snowW) * (1f - ashW);
                    float grassW = Mathf.Clamp01(1f - rockW - snowW - ashW - stepW - dirtW);
                    float sum = grassW + dirtW + rockW + snowW + ashW + stepW;
                    map[y, x, 0] = grassW / sum; map[y, x, 1] = dirtW / sum; map[y, x, 2] = rockW / sum;
                    map[y, x, 3] = snowW / sum; map[y, x, 4] = ashW / sum; map[y, x, 5] = stepW / sum;
                }
            td.SetAlphamaps(0, 0, map);
        }

        // ================================================================== YO'LLAR
        static readonly List<Vector2[]> roads = new List<Vector2[]>();
        static void BuildRoads(Terrain t)
        {
            roads.Clear();
            Vector2 obaGate = ObaCenter + new Vector2(0f, -ObaHalf);
            Vector2 cityGate = CityCenter + new Vector2(0f, CityR);
            Vector2 campGate = CampCenter + new Vector2(-CampR, 0f);
            Vector2 fortFoot = FortCenter + new Vector2(-170f, -190f);
            roads.Add(new[] { obaGate, obaGate + new Vector2(20f, -140f), new Vector2(-260f, 260f), Crossroads });
            roads.Add(new[] { Crossroads, new Vector2(-120f, -120f), new Vector2(-300f, -260f), cityGate });
            roads.Add(new[] { Crossroads, new Vector2(220f, -60f), new Vector2(380f, -280f), campGate });
            roads.Add(new[] { Crossroads, new Vector2(230f, 180f), new Vector2(380f, 330f), fortFoot });
            roads.Add(new[] { new Vector2(-260f, 260f), new Vector2(-40f, 420f), new Vector2(230f, 180f) });   // o'rmon yo'li
            // relyefda yo'l izi: balandlik silliqlanadi
            var td = t.terrainData;
            for (int r = 0; r < HeightRes; ++r)
                for (int c = 0; c < HeightRes; ++c)
                {
                    float x = -WorldSize * 0.5f + c * cellM, z = -WorldSize * 0.5f + r * cellM;
                    float d = RoadDist(x, z);
                    if (d > 9f) continue;
                    float k = 1f - Mathf.SmoothStep(0f, 1f, d / 9f);
                    heights[r, c] = Mathf.Lerp(heights[r, c], SmoothH(r, c), k * 0.6f);
                }
            td.SetHeights(0, 0, heights);
            ApplyLayers(t, (x, z) => { float d = RoadDist(x, z); return d < 3f ? 1f : (d < 6f ? 1f - (d - 3f) / 3f : 0f); });
        }
        static float SmoothH(int r, int c)
        {
            float s = 0f; int n = 0;
            for (int dr = -3; dr <= 3; ++dr) for (int dc = -3; dc <= 3; ++dc)
            { int rr = r + dr, cc = c + dc; if (rr < 0 || cc < 0 || rr >= HeightRes || cc >= HeightRes) continue; s += heights[rr, cc]; ++n; }
            return s / n;
        }
        static float RoadDist(float x, float z)
        {
            Vector2 p = new Vector2(x, z); float best = 1e9f;
            foreach (var road in roads)
                for (int i = 0; i + 1 < road.Length; ++i)
                {
                    Vector2 a = road[i], b = road[i + 1], ab = b - a;
                    float t = Mathf.Clamp01(Vector2.Dot(p - a, ab) / ab.sqrMagnitude);
                    best = Mathf.Min(best, Vector2.Distance(p, a + ab * t));
                }
            return best;
        }

        // ================================================================== DARYO
        static float RiverX(float z) => -850f + Mathf.Sin(z * 0.0045f) * 70f + Mathf.Sin(z * 0.013f) * 20f;
        static void BuildRiver(Transform parent, Terrain t)
        {
            var td = t.terrainData;
            for (int r = 0; r < HeightRes; ++r)
                for (int c = 0; c < HeightRes; ++c)
                {
                    float x = -WorldSize * 0.5f + c * cellM, z = -WorldSize * 0.5f + r * cellM;
                    float d = Mathf.Abs(x - RiverX(z));
                    if (d > 40f) continue;
                    float depth = (1f - Mathf.SmoothStep(0f, 1f, d / 40f)) * 7f;
                    heights[r, c] = Mathf.Max(0f, heights[r, c] - depth / td.size.y);
                }
            td.SetHeights(0, 0, heights);
            // suv yuzasi: uzun tekislik segmentlari
            var water = ErtugrulRealismPass.Mat("m_water_river", null, new Color(0.10f, 0.28f, 0.34f, 0.85f), 0.95f, false, false);
            water.SetFloat("_Surface", 1f); water.renderQueue = 3000; water.SetOverrideTag("RenderType", "Transparent");
            water.EnableKeyword("_SURFACE_TYPE_TRANSPARENT"); water.SetInt("_SrcBlend", 5); water.SetInt("_DstBlend", 10); water.SetInt("_ZWrite", 0);
            var wave = ErtugrulRealismPass.NoiseTex("t_wave_n", 256, (u, v, n) => new Color(0.5f + (n - 0.5f) * 0.4f, 0.5f + (Mathf.PerlinNoise(v * 9f, u * 9f) - 0.5f) * 0.4f, 1f), 61);
            water.SetTexture("_BumpMap", wave); water.EnableKeyword("_NORMALMAP"); water.SetFloat("_BumpScale", 0.3f);
            var river = new GameObject("River"); river.transform.SetParent(parent);
            for (float z = -WorldSize * 0.5f; z < WorldSize * 0.5f; z += 60f)
            {
                var seg = GameObject.CreatePrimitive(PrimitiveType.Plane);
                Object.DestroyImmediate(seg.GetComponent<Collider>());
                seg.name = "seg"; seg.transform.SetParent(river.transform);
                float x = RiverX(z + 30f);
                float y = t.SampleHeight(new Vector3(x, 0, z + 30f)) + t.transform.position.y + 3.2f;
                seg.transform.position = new Vector3(x, y, z + 30f);
                seg.transform.localScale = new Vector3(5.6f, 1f, 6.4f);
                seg.GetComponent<MeshRenderer>().sharedMaterial = water;
                seg.AddComponent<ErtugrulWater>();
            }
        }

        // ================================================================== QAYI OBASI (aniq reja)
        static float G(float x, float z) => ErtugrulMedievalProps.Ground(x, z);
        static Vector3 W(Vector2 local, Vector2 center) { float x = center.x + local.x, z = center.y + local.y; return new Vector3(x, G(x, z), z); }

        static void BuildOba(Transform props)
        {
            var oba = new GameObject("KayiOba").transform; oba.SetParent(props);
            var wood = ErtugrulMedievalProps.Mat("m_wood", ErtugrulMedievalProps.PlankTex(), Color.white, 0.10f, Vector2.one);
            var feltW = ErtugrulMedievalProps.Mat("m_felt_white", ErtugrulMedievalProps.FeltTex(), new Color(0.96f, 0.95f, 0.90f), 0.03f, new Vector2(3f, 1f));
            // 1) Bey chodiri — 12 m, markaz (0,0), oq kiyiz, oltin naqsh halqasi
            var bey = (GameObject)PrefabUtility.InstantiatePrefab(ErtugrulMedievalProps.YurtPrefab(0, true), oba);
            bey.name = "BeyTent"; bey.transform.position = W(Vector2.zero, ObaCenter); bey.transform.localScale = new Vector3(6f, 5.2f, 6f);
            foreach (var r in bey.GetComponentsInChildren<MeshRenderer>()) if (r.sharedMaterial != null && r.sharedMaterial.name.StartsWith("m_felt")) r.sharedMaterial = feltW;
            // 2) O'tovlar (5 m) — rejadagi kabi ikki yoy + tashqi halqa
            var yurts = new List<Vector2>();
            for (int i = 0; i < 10; ++i) { float a = Mathf.Deg2Rad * (18f + i * 36f); yurts.Add(new Vector2(Mathf.Cos(a) * 38f, Mathf.Sin(a) * 38f)); }
            for (int i = 0; i < 16; ++i) { float a = Mathf.Deg2Rad * (11f + i * 22.5f); yurts.Add(new Vector2(Mathf.Cos(a) * 70f, Mathf.Sin(a) * 70f)); }
            for (int i = 0; i < 18; ++i) { float a = Mathf.Deg2Rad * (5f + i * 20f); yurts.Add(new Vector2(Mathf.Cos(a) * 100f, Mathf.Sin(a) * 100f)); }
            var rnd = new System.Random(77); int n = 0;
            foreach (var y in yurts)
            {
                if (Mathf.Abs(y.x) < 9f) continue;                                // asosiy yo'l bo'sh
                if (y.x < -35f && y.x > -85f && Mathf.Abs(y.y) < 22f) continue;   // temirchi
                if (y.x > 35f && y.x < 85f && Mathf.Abs(y.y) < 22f) continue;     // mashq maydoni
                var g = (GameObject)PrefabUtility.InstantiatePrefab(ErtugrulMedievalProps.YurtPrefab(rnd.Next(0, 3), rnd.NextDouble() < 0.4), oba);
                g.transform.position = W(y, ObaCenter); g.transform.localScale = new Vector3(2.5f, 2.3f, 2.5f);
                g.transform.rotation = Quaternion.LookRotation(new Vector3(-y.x, 0, -y.y));   // eshik markazga
                g.name = "yurt_" + (n++);
            }
            // 4) Temirchi 15x15 (g'arb): ayvon tom, o'choq, sandon
            var bs = new GameObject("Blacksmith").transform; bs.SetParent(oba); bs.position = W(new Vector2(-60f, 0f), ObaCenter);
            Fence(bs, 15f, 15f, wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, bs, new Vector3(-3f, 3.2f, 0f), new Vector3(8f, 0.3f, 7f), ErtugrulMedievalProps.Mat("m_roof_dark", ErtugrulMedievalProps.PlankTex(), new Color(0.35f, 0.3f, 0.25f), 0.1f, Vector2.one));
            for (int i = 0; i < 4; ++i) ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, bs, new Vector3(-3f + (i % 2 == 0 ? -3.6f : 3.6f), 1.6f, i < 2 ? -3.2f : 3.2f), new Vector3(0.3f, 1.6f, 0.3f), wood);
            var stone = ErtugrulMedievalProps.Mat("m_stone", ErtugrulMedievalProps.StoneTex(), Color.white, 0.06f, Vector2.one);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, bs, new Vector3(-4f, 0.6f, 0f), new Vector3(2.2f, 1.2f, 2.2f), stone).name = "forge";
            Fire(bs, new Vector3(-4f, 1.3f, 0f));
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, bs, new Vector3(-1f, 0.5f, 1.5f), new Vector3(0.9f, 0.5f, 0.5f), ErtugrulRealismPass.Mat("m_iron", null, new Color(0.25f, 0.25f, 0.27f), 0.6f, false, false)).name = "anvil";
            // 5) Mashq maydoni 20x20 (sharq): panjara, nishonlar, jang qo'g'irchoqlari
            var tr = new GameObject("Training").transform; tr.SetParent(oba); tr.position = W(new Vector2(60f, 0f), ObaCenter);
            Fence(tr, 20f, 20f, wood);
            for (int i = 0; i < 3; ++i)
            {
                var tgt = ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, tr, new Vector3(7f, 1.4f, -6f + i * 6f), new Vector3(1.4f, 0.08f, 1.4f), ErtugrulRealismPass.Mat("m_target", null, new Color(0.85f, 0.75f, 0.5f), 0.1f, false, false), new Vector3(0, 0, 90f));
                tgt.name = "target";
                ErtugrulMedievalProps.Prim(PrimitiveType.Cube, tr, new Vector3(-5f + i * 3f, 1.0f, 4f), new Vector3(0.5f, 2.0f, 0.5f), wood).name = "dummy";
            }
            // 6) Janubiy darvoza (0,-125) — ustunlar, to'sin, ikki ko'k bayroq
            var gate = new GameObject("MainGate").transform; gate.SetParent(oba); gate.position = W(new Vector2(0f, -ObaHalf), ObaCenter);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, gate, new Vector3(-4f, 3f, 0f), new Vector3(0.6f, 3f, 0.6f), wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, gate, new Vector3(4f, 3f, 0f), new Vector3(0.6f, 3f, 0.6f), wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, gate, new Vector3(0f, 6f, 0f), new Vector3(9.5f, 0.5f, 0.6f), wood);
            var blue = ErtugrulRealismPass.Mat("m_banner_blue", null, new Color(0.12f, 0.25f, 0.62f), 0.2f, false, true);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, gate, new Vector3(-6.5f, 4.2f, 0f), new Vector3(1.6f, 3.2f, 0.04f), blue);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, gate, new Vector3(6.5f, 4.2f, 0f), new Vector3(1.6f, 3.2f, 0.04f), blue);
            // devor (palisad) — janub yoyi, darvoza ochiq
            for (int i = 0; i < 64; ++i)
            {
                float a = Mathf.Deg2Rad * (200f + i * 2.2f);
                Vector2 pnt = new Vector2(Mathf.Cos(a), Mathf.Sin(a)) * (ObaHalf - 2f);
                if (Mathf.Abs(pnt.x) < 6f) continue;
                var post = ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, oba, Vector3.zero, new Vector3(0.35f, 1.6f, 0.35f), wood);
                post.transform.position = W(pnt, ObaCenter) + Vector3.up * 1.5f;
            }
            // ichkarida siyrak daraxt va toshlar (rejadagi kabi)
            for (int i = 0; i < 26; ++i)
            {
                Vector2 lp = new Vector2((float)(rnd.NextDouble() * 2 - 1) * 112f, (float)(rnd.NextDouble() * 2 - 1) * 112f);
                if (lp.magnitude > 118f || Mathf.Abs(lp.x) < 10f || lp.magnitude < 22f) continue;
                bool nearYurt = false; foreach (var y in yurts) if (Vector2.Distance(y, lp) < 8f) nearYurt = true;
                if (nearYurt) continue;
                var tree = (GameObject)PrefabUtility.InstantiatePrefab(ErtugrulRealismPass.TreePrefab(rnd.Next(0, 3), true), oba);
                tree.transform.position = W(lp, ObaCenter); tree.transform.localScale = Vector3.one * (0.6f + (float)rnd.NextDouble() * 0.4f);
                tree.transform.rotation = Quaternion.Euler(0, (float)rnd.NextDouble() * 360f, 0);
            }
            for (int i = 0; i < 14; ++i)
            {
                Vector2 lp = new Vector2((float)(rnd.NextDouble() * 2 - 1) * 115f, (float)(rnd.NextDouble() * 2 - 1) * 115f);
                if (Mathf.Abs(lp.x) < 8f || lp.magnitude < 20f) continue;
                var rock = (GameObject)PrefabUtility.InstantiatePrefab(ErtugrulRealismPass.RockPrefab(rnd.Next(0, 3)), oba);
                rock.transform.position = W(lp, ObaCenter); float rs = 0.8f + (float)rnd.NextDouble() * 1.5f; rock.transform.localScale = new Vector3(rs, rs * 0.6f, rs);
            }
            // 7) Qorovul minoralari — 4 burchak
            foreach (var c in new[] { new Vector2(-110f, 110f), new Vector2(110f, 110f), new Vector2(-105f, -100f), new Vector2(110f, -75f) })
                WatchTower(oba, W(c, ObaCenter), wood);
        }

        static void Fence(Transform parent, float w, float d, Material wood)
        {
            for (float x = -w * 0.5f; x <= w * 0.5f; x += 2.5f) { ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, parent, new Vector3(x, 0.6f, -d * 0.5f), new Vector3(0.18f, 0.6f, 0.18f), wood); ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, parent, new Vector3(x, 0.6f, d * 0.5f), new Vector3(0.18f, 0.6f, 0.18f), wood); }
            for (float z = -d * 0.5f; z <= d * 0.5f; z += 2.5f) { ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, parent, new Vector3(-w * 0.5f, 0.6f, z), new Vector3(0.18f, 0.6f, 0.18f), wood); ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, parent, new Vector3(w * 0.5f, 0.6f, z), new Vector3(0.18f, 0.6f, 0.18f), wood); }
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, parent, new Vector3(0, 0.95f, -d * 0.5f), new Vector3(w, 0.08f, 0.08f), wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, parent, new Vector3(0, 0.95f, d * 0.5f), new Vector3(w, 0.08f, 0.08f), wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, parent, new Vector3(-w * 0.5f, 0.95f, 0), new Vector3(0.08f, 0.08f, d), wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, parent, new Vector3(w * 0.5f, 0.95f, 0), new Vector3(0.08f, 0.08f, d), wood);
        }
        static void WatchTower(Transform parent, Vector3 pos, Material wood)
        {
            var t = new GameObject("WatchTower").transform; t.SetParent(parent); t.position = pos;
            for (int i = 0; i < 4; ++i) ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, t, new Vector3(i % 2 == 0 ? -1.4f : 1.4f, 4f, i < 2 ? -1.4f : 1.4f), new Vector3(0.28f, 4f, 0.28f), wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, t, new Vector3(0, 8f, 0), new Vector3(3.6f, 0.25f, 3.6f), wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, t, new Vector3(0, 8.6f, -1.8f), new Vector3(3.6f, 1.0f, 0.1f), wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, t, new Vector3(0, 8.6f, 1.8f), new Vector3(3.6f, 1.0f, 0.1f), wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, t, new Vector3(-1.8f, 8.6f, 0), new Vector3(0.1f, 1.0f, 3.6f), wood);
            ErtugrulMedievalProps.Prim(PrimitiveType.Cube, t, new Vector3(1.8f, 8.6f, 0), new Vector3(0.1f, 1.0f, 3.6f), wood);
            var roof = new GameObject("roof"); roof.transform.SetParent(t, false); roof.transform.localPosition = new Vector3(0, 10.5f, 0);
            roof.AddComponent<MeshFilter>().sharedMesh = ErtugrulMedievalProps.ConeMesh(2.8f, 1.8f, 4); roof.AddComponent<MeshRenderer>().sharedMaterial = wood;
            roof.transform.localRotation = Quaternion.Euler(0, 45f, 0);
            var col = t.gameObject.AddComponent<BoxCollider>(); col.center = new Vector3(0, 4f, 0); col.size = new Vector3(3.2f, 8f, 3.2f);
        }
        static void Fire(Transform parent, Vector3 localPos)
        {
            var fire = new GameObject("Fire"); fire.transform.SetParent(parent, false); fire.transform.localPosition = localPos;
            var l = fire.AddComponent<Light>(); l.type = LightType.Point; l.color = new Color(1f, 0.55f, 0.2f); l.intensity = 6f; l.range = 14f;
            var em = ErtugrulRealismPass.Mat("m_ember", null, new Color(1f, 0.35f, 0.08f), 0.3f, false, false);
            em.EnableKeyword("_EMISSION"); em.SetColor("_EmissionColor", new Color(3f, 0.9f, 0.2f));
            ErtugrulMedievalProps.Prim(PrimitiveType.Sphere, fire.transform, Vector3.zero, new Vector3(0.9f, 0.5f, 0.9f), em);
        }

        // ================================================================== SHAHAR (SW)
        static void BuildCity(Transform props)
        {
            var city = new GameObject("City").transform; city.SetParent(props);
            var sand = ErtugrulMedievalProps.Mat("m_sandstone", ErtugrulMedievalProps.StoneTex(), new Color(0.92f, 0.84f, 0.66f), 0.06f, Vector2.one);
            var sandDark = ErtugrulMedievalProps.Mat("m_sandstone_dark", ErtugrulMedievalProps.StoneTex(), new Color(0.72f, 0.64f, 0.48f), 0.06f, Vector2.one);
            var gold = ErtugrulRealismPass.Mat("m_gold", null, new Color(0.95f, 0.75f, 0.25f), 0.85f, false, false); gold.SetFloat("_Metallic", 0.9f);
            var teal = ErtugrulRealismPass.Mat("m_teal", null, new Color(0.15f, 0.45f, 0.50f), 0.5f, false, false);
            Vector3 c = new Vector3(CityCenter.x, G(CityCenter.x, CityCenter.y), CityCenter.y);
            city.position = c;
            // sakkiz burchakli devor + 8 minora, janubda darvoza
            float wallH = 11f;
            for (int i = 0; i < 8; ++i)
            {
                float a0 = Mathf.Deg2Rad * (22.5f + i * 45f), a1 = Mathf.Deg2Rad * (22.5f + (i + 1) * 45f);
                Vector3 p0 = new Vector3(Mathf.Cos(a0), 0, Mathf.Sin(a0)) * CityR, p1 = new Vector3(Mathf.Cos(a1), 0, Mathf.Sin(a1)) * CityR;
                Vector3 mid = (p0 + p1) * 0.5f; float len = Vector3.Distance(p0, p1);
                bool gate = i == 5;   // janub segmenti
                if (!gate)
                {
                    var g = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, city, mid + Vector3.up * (wallH * 0.5f - 3f), new Vector3(len, wallH + 6f, 2.4f), sand);
                    g.transform.rotation = Quaternion.LookRotation(Vector3.Cross(p1 - p0, Vector3.up)); g.AddComponent<BoxCollider>(); g.name = "wall";
                    ErtugrulMedievalProps.SetTiling(g, len / 5f, (wallH + 6f) / 5f);
                    int cnt = Mathf.RoundToInt(len / 3f);
                    for (int k = 0; k < cnt; ++k)
                    {
                        Vector3 p = Vector3.Lerp(p0, p1, (k + 0.5f) / cnt) + Vector3.up * (wallH + 0.7f);
                        var cr = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, city, p, new Vector3(1.4f, 1.4f, 2.6f), sand); cr.transform.rotation = g.transform.rotation;
                    }
                }
                else
                {
                    foreach (float s in new[] { -1f, 1f })
                    {
                        Vector3 gm = Vector3.Lerp(p0, p1, 0.5f + s * 0.28f); float gl = len * 0.44f;
                        var g = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, city, gm + Vector3.up * (wallH * 0.5f - 3f), new Vector3(gl, wallH + 6f, 2.4f), sand);
                        g.transform.rotation = Quaternion.LookRotation(Vector3.Cross(p1 - p0, Vector3.up)); g.AddComponent<BoxCollider>(); g.name = "gatewall";
                    }
                    var lintel = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, city, mid + Vector3.up * (wallH + 1f), new Vector3(len * 0.14f, 4f, 3f), sandDark);
                    lintel.transform.rotation = Quaternion.LookRotation(Vector3.Cross(p1 - p0, Vector3.up));
                }
                // burchak minorasi
                var tw = ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, city, p0 + Vector3.up * (wallH * 0.7f - 3f), new Vector3(9f, wallH * 0.7f + 3f, 9f), sandDark);
                tw.AddComponent<CapsuleCollider>(); tw.name = "tower";
                ErtugrulMedievalProps.Crenels(city, p0 + Vector3.up * (wallH * 1.4f), 4.5f, sand, 12);
            }
            // ichki binolar: yassi tomli uylar (halqalar), markazda saroy + oltin gumbaz + 2 minora
            var rnd = new System.Random(5);
            for (int ring = 0; ring < 4; ++ring)
            {
                float rr = 55f + ring * 32f; int cnt = 14 + ring * 8;
                for (int k = 0; k < cnt; ++k)
                {
                    float a = Mathf.Deg2Rad * (k * 360f / cnt + ring * 7f);
                    if (Mathf.Abs(Mathf.Cos(a + Mathf.PI * 0.5f)) < 0.09f && Mathf.Sin(a) < 0) continue;   // janubiy ko'cha
                    float bw = 6f + (float)rnd.NextDouble() * 6f, bh = 4f + (float)rnd.NextDouble() * 5f, bd = 6f + (float)rnd.NextDouble() * 6f;
                    var b = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, city, new Vector3(Mathf.Cos(a) * rr, bh * 0.5f - 0.5f, Mathf.Sin(a) * rr), new Vector3(bw, bh, bd), rnd.NextDouble() < 0.3 ? sandDark : sand);
                    b.transform.rotation = Quaternion.Euler(0, -a * Mathf.Rad2Deg + 90f, 0); b.AddComponent<BoxCollider>(); b.name = "house";
                    if (rnd.NextDouble() < 0.25) ErtugrulMedievalProps.Prim(PrimitiveType.Cube, b.transform, new Vector3(0, 0.5f, 0), new Vector3(0.6f, 0.15f, 0.6f), teal);
                }
            }
            var palace = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, city, new Vector3(0, 6f, 0), new Vector3(34f, 14f, 34f), sand); palace.AddComponent<BoxCollider>(); palace.name = "palace";
            ErtugrulMedievalProps.SetTiling(palace, 7f, 3f);
            var dome = ErtugrulMedievalProps.Prim(PrimitiveType.Sphere, city, new Vector3(0, 13f + 8f, 0), new Vector3(20f, 16f, 20f), gold); dome.name = "dome";
            foreach (float s in new[] { -1f, 1f })
            {
                ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, city, new Vector3(s * 22f, 17f, 20f), new Vector3(3f, 17f, 3f), sand).name = "minaret";
                ErtugrulMedievalProps.Prim(PrimitiveType.Sphere, city, new Vector3(s * 22f, 35.5f, 20f), new Vector3(3.6f, 3f, 3.6f), gold);
            }
        }

        // ================================================================== MO'G'UL LAGERI (SE)
        static void BuildCamp(Transform props)
        {
            var camp = new GameObject("MongolCamp").transform; camp.SetParent(props);
            var feltDark = ErtugrulMedievalProps.Mat("m_felt_dark", ErtugrulMedievalProps.FeltTex(), new Color(0.32f, 0.28f, 0.24f), 0.03f, new Vector2(3f, 1f));
            var wood = ErtugrulMedievalProps.Mat("m_wood", ErtugrulMedievalProps.PlankTex(), Color.white, 0.10f, Vector2.one);
            var red = ErtugrulRealismPass.Mat("m_banner_red", null, new Color(0.6f, 0.08f, 0.06f), 0.2f, false, true);
            var rnd = new System.Random(13);
            int n = 0;
            for (int ring = 0; ring < 5; ++ring)
            {
                float rr = 22f + ring * 26f; int cnt = 6 + ring * 7;
                for (int k = 0; k < cnt; ++k)
                {
                    float a = Mathf.Deg2Rad * (k * 360f / cnt + ring * 11f);
                    if (Mathf.Abs(Mathf.Sin(a)) < 0.12f && Mathf.Cos(a) < 0) continue;    // g'arbiy kirish
                    Vector2 lp = new Vector2(Mathf.Cos(a) * rr + (float)(rnd.NextDouble() - 0.5) * 8f, Mathf.Sin(a) * rr + (float)(rnd.NextDouble() - 0.5) * 8f);
                    var g = (GameObject)PrefabUtility.InstantiatePrefab(ErtugrulMedievalProps.YurtPrefab(rnd.Next(0, 3), rnd.NextDouble() < 0.5), camp);
                    g.transform.position = W(lp, CampCenter); float s = 1.6f + (float)rnd.NextDouble() * 1.0f;
                    g.transform.localScale = new Vector3(s, s * 0.9f, s); g.transform.rotation = Quaternion.Euler(0, (float)rnd.NextDouble() * 360f, 0);
                    foreach (var r in g.GetComponentsInChildren<MeshRenderer>()) if (r.sharedMaterial != null && r.sharedMaterial.name.StartsWith("m_felt")) r.sharedMaterial = feltDark;
                    g.name = "mtent_" + (n++);
                    if (k % 4 == 0) Fire(camp, W(lp + new Vector2(6f, 3f), CampCenter) + Vector3.up * 0.4f);
                }
            }
            // katta xon chodiri + bayroqlar
            var khan = (GameObject)PrefabUtility.InstantiatePrefab(ErtugrulMedievalProps.YurtPrefab(1, true), camp);
            khan.transform.position = W(Vector2.zero, CampCenter); khan.transform.localScale = new Vector3(5f, 4.2f, 5f); khan.name = "KhanTent";
            foreach (var r in khan.GetComponentsInChildren<MeshRenderer>()) if (r.sharedMaterial != null && r.sharedMaterial.name.StartsWith("m_felt")) r.sharedMaterial = feltDark;
            for (int i = 0; i < 6; ++i)
            {
                float a = Mathf.Deg2Rad * (i * 60f); Vector3 p = W(new Vector2(Mathf.Cos(a) * 12f, Mathf.Sin(a) * 12f), CampCenter);
                ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, camp, p + Vector3.up * 3f, new Vector3(0.15f, 3f, 0.15f), wood);
                ErtugrulMedievalProps.Prim(PrimitiveType.Cube, camp, p + new Vector3(0.7f, 5f, 0f), new Vector3(1.3f, 2.2f, 0.04f), red);
            }
            // qoziqli to'siq (sharq yarmi)
            for (int i = 0; i < 70; ++i)
            {
                float a = Mathf.Deg2Rad * (-80f + i * 2.3f);
                var post = ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, camp, Vector3.zero, new Vector3(0.3f, 1.4f, 0.3f), wood, new Vector3(-12f, 0, 0));
                post.transform.position = W(new Vector2(Mathf.Cos(a), Mathf.Sin(a)) * (CampR - 4f), CampCenter) + Vector3.up * 1.2f;
            }
        }

        // ================================================================== QAL'A (NE, tog' tepasida)
        static void BuildFortress(Transform props)
        {
            var stone = ErtugrulMedievalProps.Mat("m_stone", ErtugrulMedievalProps.StoneTex(), Color.white, 0.06f, Vector2.one);
            var stoneDark = ErtugrulMedievalProps.Mat("m_stone_dark", ErtugrulMedievalProps.StoneTex(), new Color(0.75f, 0.72f, 0.68f), 0.06f, Vector2.one);
            var wood = ErtugrulMedievalProps.Mat("m_wood", ErtugrulMedievalProps.PlankTex(), Color.white, 0.10f, Vector2.one);
            var root = new GameObject("Fortress").transform; root.SetParent(props);
            Vector3 c = new Vector3(FortCenter.x, G(FortCenter.x, FortCenter.y) + 0.3f, FortCenter.y);
            root.position = c;
            float half = FortHalf, wallH = 9f, wallT = 2f, towerR = 4.5f, towerH = 15f;
            for (int w = 0; w < 4; ++w)
            {
                bool ns = w < 2; bool gate = (w == 1);
                Vector3 ctr = w == 0 ? new Vector3(0, 0, half) : w == 1 ? new Vector3(0, 0, -half) : w == 2 ? new Vector3(half, 0, 0) : new Vector3(-half, 0, 0);
                if (!gate)
                {
                    var g = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, root, ctr + Vector3.up * (wallH * 0.5f - 4f), ns ? new Vector3(half * 2f, wallH + 8f, wallT) : new Vector3(wallT, wallH + 8f, half * 2f), stone);
                    g.AddComponent<BoxCollider>(); ErtugrulMedievalProps.SetTiling(g, half * 2f / 4f, (wallH + 8f) / 4f);
                    int count = Mathf.RoundToInt(half * 2f / 2.4f);
                    for (int i = 0; i < count; ++i)
                    {
                        float off = -half + 1.2f + i * 2.4f;
                        ErtugrulMedievalProps.Prim(PrimitiveType.Cube, root, ns ? ctr + new Vector3(off, wallH + 0.6f, 0) : ctr + new Vector3(0, wallH + 0.6f, off), ns ? new Vector3(1.2f, 1.2f, wallT + 0.1f) : new Vector3(wallT + 0.1f, 1.2f, 1.2f), stone);
                    }
                }
                else
                {
                    float segLen = half - 4f;
                    foreach (float sgn in new[] { -1f, 1f })
                    {
                        var g = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, root, ctr + new Vector3(sgn * (4f + segLen * 0.5f), wallH * 0.5f - 4f, 0), new Vector3(segLen, wallH + 8f, wallT), stone);
                        g.AddComponent<BoxCollider>(); ErtugrulMedievalProps.SetTiling(g, segLen / 4f, (wallH + 8f) / 4f);
                        var tw = ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, root, ctr + new Vector3(sgn * 5.5f, towerH * 0.5f - 4f, 0), new Vector3(3.2f * 2f, (towerH + 8f) * 0.5f, 3.2f * 2f), stoneDark);
                        tw.AddComponent<CapsuleCollider>();
                        ErtugrulMedievalProps.Crenels(root, ctr + new Vector3(sgn * 5.5f, towerH, 0), 3.2f, stone, 10);
                    }
                    ErtugrulMedievalProps.Prim(PrimitiveType.Cube, root, ctr + new Vector3(0, wallH - 0.8f, 0), new Vector3(8.2f, 1.6f, wallT), stone).AddComponent<BoxCollider>();
                }
            }
            foreach (var sx in new[] { -1f, 1f }) foreach (var sz in new[] { -1f, 1f })
            {
                var tw = ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, root, new Vector3(sx * half, towerH * 0.5f - 4f, sz * half), new Vector3(towerR * 2f, (towerH + 8f) * 0.5f, towerR * 2f), stoneDark);
                tw.AddComponent<CapsuleCollider>();
                ErtugrulMedievalProps.Crenels(root, new Vector3(sx * half, towerH, sz * half), towerR, stone, 14);
            }
            var keep = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, root, new Vector3(0, 8f - 2f, -6f), new Vector3(20f, 20f, 14f), stone); keep.AddComponent<BoxCollider>(); ErtugrulMedievalProps.SetTiling(keep, 5f, 5f);
            ErtugrulMedievalProps.Crenels(root, new Vector3(0, 18f, -6f), 0.1f, stone, 1);
            for (int i = 0; i < 12; ++i) ErtugrulMedievalProps.Prim(PrimitiveType.Cube, root, new Vector3(-9.5f + i * 1.7f, 18.8f, -12.8f), new Vector3(1f, 1.2f, 0.8f), stone);
            var flag = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, root, new Vector3(0.9f, 25f, -6f), new Vector3(2f, 1.2f, 0.04f), ErtugrulRealismPass.Mat("m_banner_red", null, new Color(0.6f, 0.08f, 0.06f), 0.2f, false, true));
            ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, root, new Vector3(0, 22f, -6f), new Vector3(0.15f, 4f, 0.15f), wood);
            // yog'och ko'prik: qal'a darvozasidan yo'l boshigacha (SW)
            Vector3 a = c + new Vector3(0, 0.5f, -half - 3f);
            Vector2 footXZ = FortCenter + new Vector2(-170f, -190f);
            Vector3 b = new Vector3(footXZ.x, G(footXZ.x, footXZ.y) + 0.5f, footXZ.y);
            int segs = 14;
            for (int i = 0; i < segs; ++i)
            {
                Vector3 p0 = Vector3.Lerp(a, b, (float)i / segs), p1 = Vector3.Lerp(a, b, (float)(i + 1) / segs);
                Vector3 mid = (p0 + p1) * 0.5f; float gy = G(mid.x, mid.z);
                if (mid.y < gy + 0.8f) continue;                       // yer ustida — ko'prik kerak emas
                var deck = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, root, Vector3.zero, new Vector3(4f, 0.4f, Vector3.Distance(p0, p1) + 0.2f), wood);
                deck.transform.position = mid; deck.transform.rotation = Quaternion.LookRotation(p1 - p0); deck.AddComponent<BoxCollider>(); deck.name = "bridge";
                foreach (float s in new[] { -1f, 1f })
                {
                    var rail = ErtugrulMedievalProps.Prim(PrimitiveType.Cube, root, Vector3.zero, new Vector3(0.15f, 1.1f, Vector3.Distance(p0, p1)), wood);
                    rail.transform.position = mid + deck.transform.right * s * 1.9f + Vector3.up * 0.7f; rail.transform.rotation = deck.transform.rotation;
                }
                if (mid.y - gy > 2f) { var pillar = ErtugrulMedievalProps.Prim(PrimitiveType.Cylinder, root, Vector3.zero, new Vector3(0.5f, (mid.y - gy) * 0.5f + 0.5f, 0.5f), wood); pillar.transform.position = new Vector3(mid.x, (mid.y + gy) * 0.5f, mid.z); }
            }
        }

        // ================================================================== DARAXTLAR / QOYALAR
        static void PlaceTrees(Transform props)
        {
            var rnd = new System.Random(21);
            var pine = new[] { ErtugrulRealismPass.TreePrefab(0, true), ErtugrulRealismPass.TreePrefab(1, true), ErtugrulRealismPass.TreePrefab(2, true) };
            var oak  = new[] { ErtugrulRealismPass.TreePrefab(0, false), ErtugrulRealismPass.TreePrefab(1, false), ErtugrulRealismPass.TreePrefab(2, false) };
            var forest = new GameObject("Forest").transform; forest.SetParent(props);
            int placed = 0, tries = 0;
            while (placed < 3200 && tries++ < 60000)
            {
                float x = (float)(rnd.NextDouble() * 2 - 1) * WorldSize * 0.48f, z = (float)(rnd.NextDouble() * 2 - 1) * WorldSize * 0.48f;
                Vector2 p = new Vector2(x, z);
                float dens = 0.04f;
                dens += 0.9f * Mask(p, ObaCenter + new Vector2(60f, -60f), 200f, 720f) * (x < 250f ? 1f : 0.2f);   // NW o'rmon
                dens += 0.25f * Mask(p, new Vector2(150f, 350f), 100f, 400f);                                  // markaziy tepaliklar
                dens *= 1f - Mask(p, CityCenter, CityR * 1.15f, CityR * 1.9f);                                  // dasht — kam
                if (Vector2.Distance(p, ObaCenter) < ObaHalf * 1.08f) continue;
                if (Vector2.Distance(p, CampCenter) < CampR * 1.1f) continue;
                if (Vector2.Distance(p, CityCenter) < CityR * 1.15f) continue;
                if (RoadDist(x, z) < 7f) continue;
                float h = G(x, z);
                if (h > 140f) continue;                                          // qorda daraxt yo'q
                if (Mathf.Abs(x - RiverX(z)) < 30f) continue;
                if (rnd.NextDouble() > dens) continue;
                bool isPine = h > 60f || Mask(p, ObaCenter, 100f, 600f) > 0.4f || rnd.NextDouble() < 0.4;
                var prefab = (isPine ? pine : oak)[rnd.Next(0, 3)];
                var g = (GameObject)PrefabUtility.InstantiatePrefab(prefab, forest);
                g.transform.position = new Vector3(x, h - 0.2f, z);
                g.transform.rotation = Quaternion.Euler(0, (float)rnd.NextDouble() * 360f, 0);
                float s = 0.8f + (float)rnd.NextDouble() * 0.7f;
                g.transform.localScale = Vector3.one * s;
                placed++;
            }
            Debug.Log("Ertugrul: dunyo — " + placed + " daraxt");
        }
        static void PlaceRocks(Transform props)
        {
            var rnd = new System.Random(31);
            var rocks = new GameObject("Rocks").transform; rocks.SetParent(props);
            int placed = 0, tries = 0;
            while (placed < 900 && tries++ < 20000)
            {
                float x = (float)(rnd.NextDouble() * 2 - 1) * WorldSize * 0.48f, z = (float)(rnd.NextDouble() * 2 - 1) * WorldSize * 0.48f;
                float h = G(x, z);
                Vector2 p = new Vector2(x, z);
                float dens = 0.15f + (h > 70f ? 0.6f : 0f) + 0.3f * Mask(p, CampCenter, 100f, 400f);
                if (Vector2.Distance(p, ObaCenter) < ObaHalf * 1.05f || Vector2.Distance(p, CityCenter) < CityR * 1.1f) continue;
                if (RoadDist(x, z) < 6f || rnd.NextDouble() > dens) continue;
                var g = (GameObject)PrefabUtility.InstantiatePrefab(ErtugrulRealismPass.RockPrefab(rnd.Next(0, 3)), rocks);
                float s = 1f + (float)rnd.NextDouble() * (h > 70f ? 6f : 2.5f);
                g.transform.position = new Vector3(x, h, z); g.transform.localScale = new Vector3(s, s * 0.7f, s);
                g.transform.rotation = Quaternion.Euler(0, (float)rnd.NextDouble() * 360f, 0);
                placed++;
            }
        }

        static void Spawns(Transform root)
        {
            var sp = new GameObject("Spawns").transform; sp.SetParent(root);
            void Add(string id, Vector2 xz, float yaw) { var g = new GameObject("Spawn_" + id); g.transform.SetParent(sp); g.transform.position = new Vector3(xz.x, G(xz.x, xz.y) + 0.1f, xz.y); g.transform.rotation = Quaternion.Euler(0, yaw, 0); }
            Add("player", ObaCenter + new Vector2(0f, -ObaHalf - 12f), 0f);      // darvoza oldida, shimolga (obaga) qaraydi
            Add("oba", ObaCenter + new Vector2(0f, -30f), 0f);
            Add("crossroads", Crossroads, 0f);
            Add("city", CityCenter + new Vector2(0f, CityR + 15f), 180f);
            Add("camp", CampCenter + new Vector2(-CampR - 15f, 0f), 90f);
            Add("fortress", FortCenter + new Vector2(0f, -FortHalf - 20f), 0f);
            Add("forest", new Vector2(-40f, 420f), 0f);
        }
    }
}
