using UnityEngine;
using UnityEngine.InputSystem;

namespace Ertugrul.Player
{
    /// <summary>
    /// Oddiy uchinchi shaxs orbital kamerasi.
    /// Sichqoncha bilan aylantiriladi, g'ildirak bilan masofa o'zgaradi.
    ///
    /// ⚠️ Keyinchalik buni CINEMACHINE ga almashtirishingiz kerak —
    /// u devor orqasiga o'tmaslik (occlusion), silliq o'tishlar va
    /// jang kamerasi uchun ancha yaxshi. Hozircha bu yetarli.
    /// </summary>
    public class ErtThirdPersonCamera : MonoBehaviour
    {
        [Header("Nishon")]
        public Transform target;
        [Tooltip("Nishonning qayeriga qaraydi (bo'y balandligi)")]
        public Vector3 targetOffset = new Vector3(0f, 1.55f, 0f);

        [Header("Masofa")]
        public float distance = 4.2f;
        public float minDistance = 1.8f;
        public float maxDistance = 8f;
        public float zoomSpeed = 2.5f;

        [Header("Aylanish")]
        public float sensitivity = 0.14f;
        public float minPitch = -25f;
        public float maxPitch = 65f;
        public float smoothing = 14f;

        [Header("To'siq")]
        [Tooltip("Devor orqasiga o'tmaslik uchun")]
        public LayerMask collisionLayers = ~0;
        public float collisionPadding = 0.25f;

        private float _yaw;
        private float _pitch = 18f;
        private Vector3 _currentPos;

        private void Start()
        {
            if (target == null)
            {
                var p = FindFirstObjectByType<ErtPlayerController>();
                if (p != null) target = p.transform;
            }

            Vector3 e = transform.eulerAngles;
            _yaw = e.y; _pitch = e.x;
            _currentPos = transform.position;

            Cursor.lockState = CursorLockMode.Locked;
            Cursor.visible = false;
        }

        private void LateUpdate()
        {
            if (target == null) return;

            var mouse = Mouse.current;
            var kb = Keyboard.current;

            // Esc — kursorni chiqarish (Editor'da testlash uchun kerak)
            if (kb != null && kb.escapeKey.wasPressedThisFrame)
            {
                bool locked = Cursor.lockState == CursorLockMode.Locked;
                Cursor.lockState = locked ? CursorLockMode.None : CursorLockMode.Locked;
                Cursor.visible = locked;
            }

            if (mouse != null && Cursor.lockState == CursorLockMode.Locked)
            {
                Vector2 delta = mouse.delta.ReadValue();
                _yaw   += delta.x * sensitivity;
                _pitch -= delta.y * sensitivity;
                _pitch  = Mathf.Clamp(_pitch, minPitch, maxPitch);

                float scroll = mouse.scroll.ReadValue().y;
                if (Mathf.Abs(scroll) > 0.01f)
                    distance = Mathf.Clamp(
                        distance - scroll * zoomSpeed * 0.01f, minDistance, maxDistance);
            }

            Quaternion rot = Quaternion.Euler(_pitch, _yaw, 0f);
            Vector3 pivot = target.position + targetOffset;
            Vector3 desired = pivot - rot * Vector3.forward * distance;

            // Devor orqasiga o'tib ketmasin
            if (Physics.SphereCast(pivot, collisionPadding, (desired - pivot).normalized,
                                   out var hit, distance, collisionLayers,
                                   QueryTriggerInteraction.Ignore))
                desired = pivot + (desired - pivot).normalized * (hit.distance - 0.05f);

            _currentPos = Vector3.Lerp(_currentPos, desired, smoothing * Time.deltaTime);
            transform.position = _currentPos;
            transform.rotation = rot;
        }
    }
}
