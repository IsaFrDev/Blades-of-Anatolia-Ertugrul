// Cinemachine 3 uchinchi shaxs kamerasi: orbital kuzatuv + sichqoncha bilan aylantirish.
// CinemachineInputAxisController o'rniga o'qlar shu skriptdan boshqariladi — Input
// Actions asset kerak emas. Cutscene paytida ErtugrulCutscenePlayer o'z kamerasini
// yuqori ustuvorlik bilan yoqadi, bu kamera avtomatik pasayadi.
using Unity.Cinemachine;
using UnityEngine;
using UnityEngine.InputSystem;

namespace Ertugrul
{
    public class ErtugrulCamera : MonoBehaviour
    {
        public float sensitivity = 0.16f;
        public float minPitch = -22f, maxPitch = 62f;
        public bool inputEnabled = true;

        CinemachineOrbitalFollow orbital;

        void Awake()
        {
            orbital = GetComponent<CinemachineOrbitalFollow>();
            Cursor.lockState = CursorLockMode.Locked;
            Cursor.visible = false;
        }

        void Update()
        {
            if (orbital == null || !inputEnabled) return;
            var m = Mouse.current;
            if (m == null) return;
            Vector2 d = m.delta.ReadValue();
            orbital.HorizontalAxis.Value = Mathf.Repeat(orbital.HorizontalAxis.Value + d.x * sensitivity + 180f, 360f) - 180f;
            orbital.VerticalAxis.Value = Mathf.Clamp(orbital.VerticalAxis.Value - d.y * sensitivity, minPitch, maxPitch);
            if (Keyboard.current != null && Keyboard.current.escapeKey.wasPressedThisFrame)
            {
                Cursor.lockState = Cursor.lockState == CursorLockMode.Locked ? CursorLockMode.None : CursorLockMode.Locked;
                Cursor.visible = Cursor.lockState != CursorLockMode.Locked;
            }
        }

        public void DebugSetYaw(float yaw) { if (orbital != null) orbital.HorizontalAxis.Value = yaw; }
    }
}
