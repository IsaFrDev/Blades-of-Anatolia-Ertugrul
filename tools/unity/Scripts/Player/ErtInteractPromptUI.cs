using UnityEngine;
using TMPro;

namespace Ertugrul.Player
{
    /// <summary>
    /// "[E] Turgut bilan gaplashish" — obyekt tepasida chiqadigan matn.
    /// Dunyodagi nuqtani ekran koordinatasiga o'giradi (world → screen).
    /// </summary>
    public class ErtInteractPromptUI : MonoBehaviour
    {
        [Header("Bog'lanishlar")]
        public RectTransform panel;
        public TextMeshProUGUI label;

        [Header("Ko'rinish")]
        public Vector2 screenOffset = new Vector2(0f, 24f);
        [Tooltip("Shu masofadan uzoqda prompt kichrayadi")]
        public float fadeStartDistance = 6f;

        private Camera _cam;

        private void Awake()
        {
            if (panel == null) panel = GetComponent<RectTransform>();
            Hide();
        }

        public void Show(string text, Transform worldAnchor)
        {
            if (_cam == null) _cam = Camera.main;
            if (_cam == null || worldAnchor == null) { Hide(); return; }

            // Kamera orqasida bo'lsa ko'rsatmaymiz
            Vector3 viewport = _cam.WorldToViewportPoint(worldAnchor.position);
            if (viewport.z <= 0f) { Hide(); return; }

            panel.gameObject.SetActive(true);
            if (label != null) label.text = $"[E]  {text}";

            Vector3 screen = _cam.WorldToScreenPoint(worldAnchor.position);
            panel.position = screen + (Vector3)screenOffset;

            // Uzoqlashganda kichrayadi — nozik detal
            float dist = Vector3.Distance(_cam.transform.position, worldAnchor.position);
            float scale = Mathf.Lerp(1f, 0.78f, Mathf.Clamp01(dist / fadeStartDistance));
            panel.localScale = Vector3.one * scale;
        }

        public void Hide()
        {
            if (panel != null) panel.gameObject.SetActive(false);
        }
    }
}
