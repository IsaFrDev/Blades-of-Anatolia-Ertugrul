using UnityEngine;
using UnityEngine.InputSystem;
using Ertugrul.NPC;

namespace Ertugrul.Player
{
    /// <summary>
    /// O'yinchi atrofidagi eng yaqin interaktiv obyektni topadi va [E] bilan ishlatadi.
    ///
    /// Ishlash printsipi: har kadrda o'yinchi atrofida sfera tekshiriladi
    /// (Physics.OverlapSphere), topilganlar orasidan eng yaqini va
    /// eng OLDIDA turgani tanlanadi.
    /// </summary>
    public class ErtPlayerInteraction : MonoBehaviour
    {
        [Header("Aniqlash")]
        [Tooltip("Necha metr masofadan interaksiya mumkin")]
        public float interactRadius = 2.6f;

        [Tooltip("Qaysi layer'dagi obyektlar tekshiriladi. 'Interactable' layer yarating.")]
        public LayerMask interactLayers = ~0;

        [Tooltip("O'yinchi qanchalik obyekt tomonga qarashi kerak (1 = to'g'ri qarash)")]
        [Range(0f, 1f)] public float minFacingDot = 0.25f;

        [Header("Bog'lanishlar")]
        [Tooltip("UI: 'E — gaplashish' matni. Bo'sh qoldirsangiz ishlaydi, lekin ko'rinmaydi.")]
        public ErtInteractPromptUI promptUI;

        private IErtInteractable _current;
        private readonly Collider[] _hits = new Collider[16];
        private ErtPlayerController _controller;

        private void Awake() => _controller = GetComponent<ErtPlayerController>();

        private void Update()
        {
            FindNearest();

            if (promptUI != null)
            {
                if (_current != null && _current.CanInteract())
                    promptUI.Show(_current.GetPrompt(), _current.GetTransform());
                else
                    promptUI.Hide();
            }

            var kb = Keyboard.current;
            if (kb == null) return;

            // Dialog paytida interaksiya bloklanadi (dialog o'zi E ni oladi)
            if (_controller != null && _controller.InputLocked) return;

            if (kb.eKey.wasPressedThisFrame && _current != null && _current.CanInteract())
                _current.Interact(gameObject);
        }

        private void FindNearest()
        {
            _current = null;
            float bestScore = float.MaxValue;

            int count = Physics.OverlapSphereNonAlloc(
                transform.position, interactRadius, _hits, interactLayers,
                QueryTriggerInteraction.Collide);

            for (int i = 0; i < count; i++)
            {
                var candidate = _hits[i].GetComponentInParent<IErtInteractable>();
                if (candidate == null || !candidate.CanInteract()) continue;

                Transform t = candidate.GetTransform();
                if (t == null) continue;

                Vector3 toTarget = t.position - transform.position;
                toTarget.y = 0f;
                float dist = toTarget.magnitude;
                if (dist < 0.01f) { _current = candidate; return; }

                // O'yinchi shu tomonga qaraydimi?
                float facing = Vector3.Dot(transform.forward, toTarget.normalized);
                if (facing < minFacingDot) continue;

                // Ball: yaqinroq va to'g'riroq qaragan g'olib
                float score = dist - facing * 1.5f;
                if (score < bestScore) { bestScore = score; _current = candidate; }
            }
        }

        // Sahnada interaksiya radiusini ko'rish uchun (faqat Editor'da)
        private void OnDrawGizmosSelected()
        {
            Gizmos.color = new Color(0.28f, 0.66f, 0.71f, 0.35f);   // feruza
            Gizmos.DrawWireSphere(transform.position, interactRadius);
        }
    }
}
