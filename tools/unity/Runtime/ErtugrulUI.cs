// Kod bilan quriladigan HUD: subtitr, gapiruvchi nomi, letterbox, qora so'nish, maqsad matni.
// Prefab yo'q — setup skripti Canvas'ni yaratadi, bu komponent uni to'ldiradi.
using UnityEngine;
using UnityEngine.UI;

namespace Ertugrul
{
    public class ErtugrulUI : MonoBehaviour
    {
        public static ErtugrulUI Instance { get; private set; }

        Text subtitle, speaker, objective, hint;
        Image top, bottom, fade;
        float letterbox, letterboxTarget, fadeA, fadeTarget;
        float subtitleT;

        void Awake()
        {
            Instance = this;
            var canvas = GetComponent<Canvas>();
            if (canvas == null) canvas = gameObject.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 100;
            if (GetComponent<CanvasScaler>() == null)
            {
                var sc = gameObject.AddComponent<CanvasScaler>();
                sc.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
                sc.referenceResolution = new Vector2(1920, 1080);
            }
            Font font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");

            top = MakeBar("Letterbox_Top", new Vector2(0, 1), new Vector2(1, 1), new Vector2(0.5f, 1));
            bottom = MakeBar("Letterbox_Bottom", new Vector2(0, 0), new Vector2(1, 0), new Vector2(0.5f, 0));
            fade = MakeBar("Fade", Vector2.zero, Vector2.one, new Vector2(0.5f, 0.5f));
            fade.rectTransform.offsetMin = Vector2.zero; fade.rectTransform.offsetMax = Vector2.zero;
            fade.color = new Color(0, 0, 0, 0);

            speaker = MakeText("Speaker", font, 30, new Color(0.31f, 0.80f, 0.77f), new Vector2(0.5f, 0.17f), 700, 40, TextAnchor.MiddleCenter);
            subtitle = MakeText("Subtitle", font, 34, Color.white, new Vector2(0.5f, 0.12f), 1400, 110, TextAnchor.UpperCenter);
            objective = MakeText("Objective", font, 26, new Color(0.31f, 0.80f, 0.77f), new Vector2(0.86f, 0.92f), 520, 120, TextAnchor.UpperRight);
            hint = MakeText("Hint", font, 22, new Color(0.85f, 0.83f, 0.78f), new Vector2(0.5f, 0.03f), 1400, 30, TextAnchor.MiddleCenter);
            hint.text = "W/A/S/D — harakat  ·  Shift — sprint  ·  Space — sakrash / chiqish  ·  Ctrl — cho'kkalash  ·  Esc — sichqoncha";
        }

        Image MakeBar(string name, Vector2 amin, Vector2 amax, Vector2 pivot)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(Image));
            go.transform.SetParent(transform, false);
            var img = go.GetComponent<Image>();
            img.color = Color.black; img.raycastTarget = false;
            var rt = img.rectTransform;
            rt.anchorMin = amin; rt.anchorMax = amax; rt.pivot = pivot;
            rt.sizeDelta = new Vector2(0, 0);
            return img;
        }

        Text MakeText(string name, Font font, int size, Color col, Vector2 anchor, float w, float h, TextAnchor align)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(Text), typeof(Shadow));
            go.transform.SetParent(transform, false);
            var t = go.GetComponent<Text>();
            t.font = font; t.fontSize = size; t.color = col; t.alignment = align; t.raycastTarget = false;
            t.horizontalOverflow = HorizontalWrapMode.Wrap; t.verticalOverflow = VerticalWrapMode.Overflow;
            var rt = t.rectTransform;
            rt.anchorMin = rt.anchorMax = anchor; rt.pivot = new Vector2(0.5f, 0.5f);
            rt.sizeDelta = new Vector2(w, h);
            go.GetComponent<Shadow>().effectDistance = new Vector2(1.5f, -1.5f);
            return t;
        }

        // FPS ko'rsatkichi — optimizatsiya doim ko'z oldida tursin (Steam sharhlaridagi asosiy shikoyat)
        Text fpsText; float fpsAcc, fpsT; int fpsN;

        void Update()
        {
            float dt = Time.unscaledDeltaTime;
            if (fpsText == null)
            {
                fpsText = MakeText("FPS", Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf"), 20, new Color(0.7f, 0.9f, 0.7f),
                                   new Vector2(0.06f, 0.95f), 260, 30, TextAnchor.MiddleLeft);
            }
            fpsAcc += dt; ++fpsN; fpsT += dt;
            if (fpsT > 0.5f)
            {
                float ms = fpsAcc / fpsN * 1000f;
                fpsText.text = string.Format("{0:0} FPS  {1:0.0} ms", 1000f / ms, ms);
                fpsText.color = ms > 33f ? new Color(0.95f, 0.45f, 0.35f) : (ms > 20f ? new Color(0.95f, 0.85f, 0.4f) : new Color(0.7f, 0.9f, 0.7f));
                fpsAcc = 0f; fpsN = 0; fpsT = 0f;
            }
            letterbox = Mathf.MoveTowards(letterbox, letterboxTarget, dt * 1.6f);
            float barH = Screen.height * 0.12f * letterbox;
            top.rectTransform.sizeDelta = new Vector2(0, barH);
            bottom.rectTransform.sizeDelta = new Vector2(0, barH);
            fadeA = Mathf.MoveTowards(fadeA, fadeTarget, dt * 1.2f);
            fade.color = new Color(0, 0, 0, fadeA);
            if (subtitleT > 0f) { subtitleT -= dt; if (subtitleT <= 0f) { subtitle.text = ""; speaker.text = ""; } }
        }

        public void SetLetterbox(bool on) => letterboxTarget = on ? 1f : 0f;
        public void SetFade(float a) => fadeTarget = Mathf.Clamp01(a);
        public void SetFadeImmediate(float a) { fadeA = fadeTarget = a; }
        public void ShowSubtitle(string who, string text, float seconds)
        {
            speaker.text = who; subtitle.text = text; subtitleT = Mathf.Max(0.5f, seconds);
        }
        public void SetObjective(string text) => objective.text = text;
        public void SetHint(string text) => hint.text = text;
    }
}
