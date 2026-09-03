// Cutscene o'ynatuvchi — C++ dvijokning data/cutscenes/*.json formatini o'qiydi:
//   actors[] { id, char, model, loc_name, scale, tint, keys[] {t, pos, yaw, clip} }
//   camera[] { t, pos, look, fov }
//   lines[]  { t, actor, loc, vo, dur }
// Aktyorlar CastLibrary dan (OBJ prefablari) yaratiladi, kalitlar orasida interpolyatsiya,
// kamera alohida CinemachineCamera (ustuvorlik 50), replikalar: subtitr + WAV (Audio/vo/<til>/).
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using Unity.Cinemachine;
using UnityEngine;
using UnityEngine.Networking;

namespace Ertugrul
{
    [Serializable] public class CutKey { public float t; public float[] pos; public float yaw; public string clip; }
    [Serializable] public class CutActor { public string id; public string @char; public string model; public string loc_name; public float scale = 1.8f; public float[] tint; public string proportions; public List<CutKey> keys; }
    [Serializable] public class CutCam { public float t; public float[] pos; public float[] look; public float fov = 46f; }
    [Serializable] public class CutLine { public float t; public string actor; public string loc; public string vo; public float dur; }
    [Serializable] public class CutsceneData
    {
        public string id; public string episode; public string level; public bool letterbox = true;
        public float fade_in = 1f, fade_out = 1f, duration;
        public List<CutActor> actors; public List<CutCam> camera; public List<CutLine> lines;
    }

    [Serializable] public class CastEntry { public string path; public GameObject prefab; }

    public class ErtugrulCutscenePlayer : MonoBehaviour
    {
        public List<CastEntry> cast = new List<CastEntry>();   // setup skripti to'ldiradi
        public bool flipZ = true;
        public float modelYawOffset = 0f;     // Tripo OBJ modellari +Z ga qaraydi (ErtugrulPlayer ga qarang)
        public bool IsPlaying { get; private set; }
        public float Time01 { get; private set; }

        CinemachineCamera cam;
        AudioSource audioSrc;
        float skipHold;
        readonly List<GameObject> spawned = new List<GameObject>();

        public static string CutsceneDir => Path.Combine(ErtugrulLoc.Root, "Data", "cutscenes");
        public static string VoDir(string lang) => Path.Combine(ErtugrulLoc.Root, "Audio", "vo", lang);

        void Awake()
        {
            var go = new GameObject("CutsceneCamera");
            go.transform.SetParent(transform);
            cam = go.AddComponent<CinemachineCamera>();
            cam.Priority = 0;
            audioSrc = gameObject.AddComponent<AudioSource>();
            audioSrc.spatialBlend = 0f;
        }

        public static CutsceneData LoadById(string id)
        {
            string p = Path.Combine(CutsceneDir, id + ".json");
            if (!File.Exists(p)) return null;
            return JsonUtility.FromJson<CutsceneData>(File.ReadAllText(p));
        }

        // Berilgan daraja uchun birinchi mos cutscene (episode intro)
        public static string FindForLevel(string levelId, string preferEpisode = null)
        {
            if (!Directory.Exists(CutsceneDir)) return null;
            string first = null;
            foreach (var f in Directory.GetFiles(CutsceneDir, "ep*_intro.json"))
            {
                var d = JsonUtility.FromJson<CutsceneData>(File.ReadAllText(f));
                if (d == null || d.level != levelId) continue;
                if (preferEpisode != null && d.episode == preferEpisode) return d.id;
                if (first == null) first = d.id;
            }
            return first;
        }

        public void Play(string id, Action onDone = null)
        {
            var d = LoadById(id);
            if (d == null) { Debug.LogWarning("Ertugrul: cutscene topilmadi: " + id); onDone?.Invoke(); return; }
            StartCoroutine(Run(d, onDone));
        }

        Vector3 W(float[] v) => new Vector3(v[0], v[1], flipZ ? -v[2] : v[2]);
        float Yaw(float y) => flipZ ? 180f - y : y;

        GameObject Spawn(CutActor a)
        {
            string rel = (a.model ?? "").Replace("assets/models/", "");
            GameObject prefab = null;
            foreach (var e in cast) if (e.path == rel) { prefab = e.prefab; break; }
            var root = new GameObject("Actor_" + a.id);
            if (prefab != null)
            {
                var m = Instantiate(prefab, root.transform);
                FitToHeight(m.transform, a.scale, a.proportions == "child" ? 0.72f : 1f);
                if (a.tint != null && a.tint.Length >= 3)
                {
                    var mpb = new MaterialPropertyBlock();
                    foreach (var r in m.GetComponentsInChildren<Renderer>())
                    {
                        r.GetPropertyBlock(mpb);
                        var c = new Color(a.tint[0], a.tint[1], a.tint[2], 1f);
                        Color baseCol = Color.white;
                        if (r.sharedMaterial != null && r.sharedMaterial.HasProperty("_BaseColor")) baseCol = r.sharedMaterial.GetColor("_BaseColor");
                        mpb.SetColor("_BaseColor", baseCol * c);
                        r.SetPropertyBlock(mpb);
                    }
                }
            }
            else Debug.LogWarning("Ertugrul: aktyor modeli yo'q: " + rel);
            spawned.Add(root);
            return root;
        }

        // OBJ modelini berilgan bo'yga (m) keltiradi, oyoq ostini y=0 ga qo'yadi
        public static void FitToHeight(Transform m, float heightM, float legScale = 1f)
        {
            var rs = m.GetComponentsInChildren<Renderer>();
            if (rs.Length == 0) return;
            var b = rs[0].bounds;
            foreach (var r in rs) b.Encapsulate(r.bounds);
            float h = Mathf.Max(0.01f, b.size.y);
            float s = heightM / h;
            m.localScale = Vector3.one * s * (legScale < 1f ? 0.85f : 1f);
            // oyoq osti -> ota koordinata boshi, markaz -> 0
            var b2 = rs[0].bounds; foreach (var r in rs) b2.Encapsulate(r.bounds);
            Vector3 feet = new Vector3(b2.center.x, b2.min.y, b2.center.z);
            m.position += m.parent.position - feet;
        }

        static float GroundY(Vector3 p)
        {
            if (Physics.Raycast(p + Vector3.up * 60f, Vector3.down, out var hit, 200f)) return hit.point.y;
            var t = Terrain.activeTerrain;
            return t != null ? t.SampleHeight(p) + t.transform.position.y : p.y;
        }

        IEnumerator Run(CutsceneData d, Action onDone)
        {
            IsPlaying = true;
            var ui = ErtugrulUI.Instance;
            var player = FindFirstObjectByType<ErtugrulPlayer>();
            var pcam = FindFirstObjectByType<ErtugrulCamera>();
            if (player != null) player.inputEnabled = false;
            if (pcam != null) pcam.inputEnabled = false;
            // O'yinchi modeli yashiriladi — cutscene'da Ertug'rul aktyor sifatida o'zi bor
            if (player != null && player.model != null) player.model.gameObject.SetActive(false);
            if (ui != null) { ui.SetLetterbox(d.letterbox); ui.SetFadeImmediate(1f); ui.SetFade(0f); }

            // aktyorlar
            var actors = new List<(CutActor a, GameObject go)>();
            foreach (var a in d.actors)
                if (a.keys != null && a.keys.Count > 0) actors.Add((a, Spawn(a)));

            // davomiylik
            float dur = d.duration;
            foreach (var a in d.actors) foreach (var k in a.keys) dur = Mathf.Max(dur, k.t);
            foreach (var c in d.camera) dur = Mathf.Max(dur, c.t);
            foreach (var l in d.lines) dur = Mathf.Max(dur, l.t + Mathf.Max(2f, l.dur));
            dur += 1.0f;

            cam.Priority = 50;
            int nextLine = 0;
            float t = 0f;
            var bob = new Dictionary<string, float>();
            while (t < dur)
            {
                float dt = Time.deltaTime;
                t += dt;
                Time01 = t / dur;

                // kamera
                if (d.camera != null && d.camera.Count > 0)
                {
                    EvalCam(d.camera, t, out var pos, out var look, out var fov);
                    cam.transform.position = pos;
                    cam.transform.rotation = Quaternion.LookRotation((look - pos).normalized, Vector3.up);
                    cam.Lens.FieldOfView = fov;
                }
                // aktyorlar
                foreach (var (a, go) in actors)
                {
                    EvalActor(a.keys, t, out var pos, out var yaw, out var clip, out var moving);
                    pos.y = GroundY(pos);
                    go.transform.position = pos;
                    go.transform.rotation = Quaternion.Euler(0f, yaw + modelYawOffset, 0f);
                    // Rig yo'q: yurishda tebranish, gapirishda yengil bosh harakati
                    if (go.transform.childCount > 0)
                    {
                        float b = bob.TryGetValue(a.id, out var v) ? v : 0f;
                        b += dt * (moving ? 5.5f : (clip == "Talk" ? 1.8f : 0.7f));
                        bob[a.id] = b;
                        var m = go.transform.GetChild(0);
                        float amp = moving ? 0.04f : 0.008f;
                        var lp = m.localPosition; lp.y = Mathf.Abs(Mathf.Sin(b)) * amp; m.localPosition = lp;
                    }
                }
                // replikalar
                while (d.lines != null && nextLine < d.lines.Count && d.lines[nextLine].t <= t)
                {
                    var l = d.lines[nextLine++];
                    string who = "";
                    foreach (var a in d.actors) if (a.id == l.actor) who = ErtugrulLoc.T(a.loc_name);
                    string text = ErtugrulLoc.T(l.loc);
                    float sec = l.dur > 0.1f ? l.dur : Mathf.Clamp(text.Length * 0.06f, 2f, 7f);
                    ui?.ShowSubtitle(who, text, sec);
                    StartCoroutine(PlayVo(l.vo));
                }
                if (t > dur - d.fade_out) ui?.SetFade(1f);
                // O'tkazib yuborish: Esc ni 0.5 s ushlab turing (tasodifiy bosishdan himoya)
                var kb = UnityEngine.InputSystem.Keyboard.current;
                skipHold = (kb != null && kb.escapeKey.isPressed) ? skipHold + dt : 0f;
                if (skipHold > 0.5f) break;
                yield return null;
            }

            foreach (var g in spawned) Destroy(g);
            spawned.Clear();
            cam.Priority = 0;
            audioSrc.Stop();
            if (ui != null) { ui.SetLetterbox(false); ui.SetFade(0f); }
            if (player != null) player.inputEnabled = true;
            if (player != null && player.model != null) player.model.gameObject.SetActive(true);
            if (pcam != null) pcam.inputEnabled = true;
            IsPlaying = false;
            onDone?.Invoke();
        }

        void EvalCam(List<CutCam> ks, float t, out Vector3 pos, out Vector3 look, out float fov)
        {
            int i = 0;
            while (i + 1 < ks.Count && ks[i + 1].t <= t) ++i;
            var a = ks[i];
            var b = (i + 1 < ks.Count) ? ks[i + 1] : a;
            float u = (b.t > a.t) ? Mathf.Clamp01((t - a.t) / (b.t - a.t)) : 0f;
            u = u * u * (3f - 2f * u);
            pos = Vector3.Lerp(W(a.pos), W(b.pos), u);
            look = Vector3.Lerp(W(a.look), W(b.look), u);
            fov = Mathf.Lerp(a.fov, b.fov, u);
        }

        void EvalActor(List<CutKey> ks, float t, out Vector3 pos, out float yaw, out string clip, out bool moving)
        {
            int i = 0;
            while (i + 1 < ks.Count && ks[i + 1].t <= t) ++i;
            var a = ks[i];
            var b = (i + 1 < ks.Count) ? ks[i + 1] : a;
            float u = (b.t > a.t) ? Mathf.Clamp01((t - a.t) / (b.t - a.t)) : 0f;
            Vector3 pa = W(a.pos), pb = W(b.pos);
            pos = Vector3.Lerp(pa, pb, u);
            yaw = Mathf.LerpAngle(Yaw(a.yaw), Yaw(b.yaw), u);
            clip = a.clip ?? "Idle";
            moving = (pb - pa).sqrMagnitude > 0.05f && u < 1f;
        }

        IEnumerator PlayVo(string vo)
        {
            if (string.IsNullOrEmpty(vo)) yield break;
            string p = Path.Combine(VoDir(ErtugrulLoc.Lang), vo + ".wav");
            if (!File.Exists(p)) yield break;
            using (var req = UnityWebRequestMultimedia.GetAudioClip("file:///" + p.Replace('\\', '/'), AudioType.WAV))
            {
                yield return req.SendWebRequest();
                if (req.result != UnityWebRequest.Result.Success) yield break;
                var clip = DownloadHandlerAudioClip.GetContent(req);
                audioSrc.Stop();
                audioSrc.clip = clip;
                audioSrc.Play();
            }
        }
    }
}
