using UnityEngine;
using UnityEngine.InputSystem;

namespace Ertugrul.Player
{
    /// <summary>
    /// Uchinchi shaxs harakat boshqaruvi.
    /// Talab: shu GameObject'da CharacterController komponenti bo'lishi kerak.
    ///
    /// Boshqaruv (GDD 07_SETTINGS_HOTKEYS.md ga mos):
    ///   W A S D   — harakat
    ///   Shift     — yugurish (nafas sarflaydi)
    ///   Ctrl      — cho'kkalash
    ///   Space     — sakrash
    ///   Sichqoncha— kamera
    /// </summary>
    [RequireComponent(typeof(CharacterController))]
    public class ErtPlayerController : MonoBehaviour
    {
        [Header("Harakat tezligi (m/s)")]
        public float walkSpeed   = 2.2f;
        public float runSpeed    = 5.4f;
        public float crouchSpeed = 1.1f;
        public float rotationSpeed = 12f;

        [Header("Fizika")]
        public float gravity   = -18f;
        public float jumpHeight = 1.1f;

        [Header("Nafas (Stamina)")]
        public float maxStamina = 100f;
        public float staminaDrainPerSecond = 12f;
        public float staminaRegenPerSecond = 18f;
        [Tooltip("Nafas shu qiymatdan past bo'lsa yugura olmaysiz")]
        public float staminaRunThreshold = 8f;

        [Header("Kamera")]
        [Tooltip("Bo'sh qoldirsangiz Camera.main olinadi")]
        public Transform cameraTransform;

        // ── Holat (boshqa skriptlar o'qiydi) ────────────────────────
        public float Stamina { get; private set; }
        public bool  IsRunning { get; private set; }
        public bool  IsCrouching { get; private set; }
        public bool  IsGrounded => _cc.isGrounded;
        /// <summary>Dialog paytida harakatni bloklash uchun</summary>
        public bool  InputLocked { get; set; }

        private CharacterController _cc;
        private Vector3 _velocity;

        private void Awake()
        {
            _cc = GetComponent<CharacterController>();
            Stamina = maxStamina;

            if (cameraTransform == null && Camera.main != null)
                cameraTransform = Camera.main.transform;
        }

        private void Update()
        {
            // ⚠️ Input System paketi kerak. Klaviatura ulanmagan bo'lsa — chiqamiz.
            var kb = Keyboard.current;
            if (kb == null) return;

            HandleGravity();

            if (InputLocked)
            {
                // Dialog paytida ham gravitatsiya ishlaydi, lekin harakat yo'q
                _cc.Move(_velocity * Time.deltaTime);
                RegenStamina();
                return;
            }

            // ── Kirish ──────────────────────────────────────────────
            float h = (kb.dKey.isPressed ? 1f : 0f) - (kb.aKey.isPressed ? 1f : 0f);
            float v = (kb.wKey.isPressed ? 1f : 0f) - (kb.sKey.isPressed ? 1f : 0f);
            Vector2 input = Vector2.ClampMagnitude(new Vector2(h, v), 1f);

            IsCrouching = kb.leftCtrlKey.isPressed;
            bool wantsRun = kb.leftShiftKey.isPressed && !IsCrouching && input.sqrMagnitude > 0.01f;
            IsRunning = wantsRun && Stamina > staminaRunThreshold;

            // ── Kamera bo'yicha yo'nalish ───────────────────────────
            Vector3 forward = Vector3.forward, right = Vector3.right;
            if (cameraTransform != null)
            {
                forward = Vector3.ProjectOnPlane(cameraTransform.forward, Vector3.up).normalized;
                right   = Vector3.ProjectOnPlane(cameraTransform.right,   Vector3.up).normalized;
            }
            Vector3 moveDir = (forward * input.y + right * input.x);

            // ── Harakat ─────────────────────────────────────────────
            float speed = IsCrouching ? crouchSpeed : (IsRunning ? runSpeed : walkSpeed);
            _cc.Move(moveDir * speed * Time.deltaTime);

            // ── Burilish (harakat yo'nalishiga qarab) ───────────────
            if (moveDir.sqrMagnitude > 0.001f)
            {
                Quaternion target = Quaternion.LookRotation(moveDir, Vector3.up);
                transform.rotation = Quaternion.Slerp(
                    transform.rotation, target, rotationSpeed * Time.deltaTime);
            }

            // ── Sakrash ─────────────────────────────────────────────
            if (kb.spaceKey.wasPressedThisFrame && _cc.isGrounded && !IsCrouching)
                _velocity.y = Mathf.Sqrt(jumpHeight * -2f * gravity);

            _cc.Move(_velocity * Time.deltaTime);

            // ── Nafas ───────────────────────────────────────────────
            if (IsRunning) Stamina = Mathf.Max(0f, Stamina - staminaDrainPerSecond * Time.deltaTime);
            else           RegenStamina();
        }

        private void HandleGravity()
        {
            if (_cc.isGrounded && _velocity.y < 0f)
                _velocity.y = -2f;              // yerga bosib turish
            _velocity.y += gravity * Time.deltaTime;
        }

        private void RegenStamina()
            => Stamina = Mathf.Min(maxStamina, Stamina + staminaRegenPerSecond * Time.deltaTime);
    }
}
