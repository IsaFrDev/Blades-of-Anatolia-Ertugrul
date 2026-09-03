// Ertug'rul — uchinchi shaxs harakat mexanikasi (C++ Character.cpp ning Unity ko'chirmasi).
//   yurish 1.35 m/s · yugurish 3.3 · sprint 6.2 (Shift) · sakrash (Space) · cho'kkalash (Ctrl)
//   qiyalikda sirpanish, chekkaga chiqish (mantle: past to'siq oldida Space)
//   kamera-nisbiy harakat, tana burilish inersiyasi
// Animator bo'lsa (Mixamo rig): Speed / Jump / Crouch / Grounded parametrlari.
// Animator bo'lmasa (OBJ placeholder): oddiy protsedural tebranish.
using UnityEngine;
using UnityEngine.InputSystem;

namespace Ertugrul
{
    [RequireComponent(typeof(CharacterController))]
    public class ErtugrulPlayer : MonoBehaviour
    {
        [Header("Tezliklar (m/s)")]
        public float walkSpeed = 1.35f;
        public float jogSpeed = 3.3f;
        public float sprintSpeed = 6.2f;
        public float crouchSpeed = 1.1f;
        public float accel = 14f;
        public float jumpHeight = 1.25f;
        public float gravity = -19.6f;
        public float turnSharpness = 11f;

        [Header("Chekkaga chiqish")]
        public float mantleMaxHeight = 1.9f;
        public float mantleReach = 1.1f;

        [Header("Kamera")]
        public Transform cameraTransform;     // yo'nalish shu kameradan olinadi
        public Transform model;               // aylantiriladigan vizual model
        public Animator animator;             // ixtiyoriy (Mixamo)

        // Model "old" tomoni bilan harakat yo'nalishi orasidagi ofset. Tripo OBJ (ottoman)
        // +Z ga qaraydi — 0 (sinovda 0/90/180 solishtirildi). Boshqa model uchun sozlang.
        public float modelYawOffset = 0f;

        public bool inputEnabled = true;      // cutscene paytida o'chiriladi
        public Vector2 debugMove;             // sinov: kiritish o'rniga (CLI dan)
        public bool debugSprint;

        CharacterController cc;
        Vector3 velocity;
        float speed;                          // gorizontal tezlik
        float yaw;
        bool crouch;
        bool grounded;
        float bob;
        float mantleT;
        Vector3 mantleFrom, mantleTo;
        float defaultHeight, defaultCenterY;

        public float Speed => speed;
        public bool Grounded => grounded;

        void Awake()
        {
            cc = GetComponent<CharacterController>();
            defaultHeight = cc.height;
            defaultCenterY = cc.center.y;
            yaw = transform.eulerAngles.y;
            if (cameraTransform == null && Camera.main != null) cameraTransform = Camera.main.transform;
            if (model == null && transform.childCount > 0) model = transform.GetChild(0);
            if (model != null) model.localRotation = Quaternion.Euler(0f, modelYawOffset, 0f);
        }

        void Update()
        {
            float dt = Time.deltaTime;
            if (dt <= 0f) return;

            // ---------- chekkaga chiqish animatsiyasi ----------
            if (mantleT > 0f)
            {
                mantleT -= dt;
                float u = 1f - Mathf.Clamp01(mantleT / 0.55f);
                Vector3 p = Vector3.Lerp(mantleFrom, mantleTo, u);
                p.y = Mathf.Lerp(mantleFrom.y, mantleTo.y, Mathf.Sqrt(u));   // avval yuqoriga
                cc.enabled = false; transform.position = p; cc.enabled = true;
                UpdateAnimation(dt, 0f);
                return;
            }

            // ---------- kiritish ----------
            var kb = Keyboard.current;
            Vector2 mv = debugMove;
            bool sprint = debugSprint, jump = false, wantCrouch = crouch;
            if (inputEnabled && kb != null && debugMove.sqrMagnitude < 0.01f)
            {
                mv = new Vector2((kb.dKey.isPressed ? 1 : 0) - (kb.aKey.isPressed ? 1 : 0),
                                 (kb.wKey.isPressed ? 1 : 0) - (kb.sKey.isPressed ? 1 : 0));
                sprint = kb.leftShiftKey.isPressed;
                jump = kb.spaceKey.wasPressedThisFrame;
                if (kb.leftCtrlKey.wasPressedThisFrame || kb.cKey.wasPressedThisFrame) wantCrouch = !crouch;
            }
            if (!inputEnabled) { mv = Vector2.zero; sprint = false; }
            if (mv.sqrMagnitude > 1f) mv.Normalize();

            // ---------- kamera-nisbiy yo'nalish ----------
            Vector3 fwd = Vector3.forward, right = Vector3.right;
            if (cameraTransform != null)
            {
                fwd = cameraTransform.forward; fwd.y = 0f; fwd.Normalize();
                right = cameraTransform.right; right.y = 0f; right.Normalize();
            }
            Vector3 wish = fwd * mv.y + right * mv.x;
            float wishLen = Mathf.Clamp01(wish.magnitude);

            // ---------- cho'kkalash ----------
            if (wantCrouch != crouch)
            {
                crouch = wantCrouch;
                cc.height = crouch ? defaultHeight * 0.62f : defaultHeight;
                cc.center = new Vector3(0f, crouch ? defaultCenterY * 0.62f : defaultCenterY, 0f);
            }

            // ---------- tezlik profili (AC uslubi: analog kuch -> yurish/yugurish/sprint) ----------
            float target = 0f;
            if (wishLen > 0.05f)
            {
                if (crouch) target = crouchSpeed;
                else if (sprint) target = sprintSpeed;
                else target = wishLen < 0.5f ? walkSpeed : jogSpeed;
            }
            speed = Mathf.MoveTowards(speed, target, accel * dt * (target < speed ? 1.6f : 1f));

            // ---------- burilish ----------
            if (wishLen > 0.05f)
            {
                float targetYaw = Mathf.Atan2(wish.x, wish.z) * Mathf.Rad2Deg;
                yaw = Mathf.LerpAngle(yaw, targetYaw, 1f - Mathf.Exp(-turnSharpness * dt));
            }
            Vector3 moveDir = Quaternion.Euler(0f, yaw, 0f) * Vector3.forward;

            // ---------- vertikal ----------
            grounded = cc.isGrounded;
            if (grounded && velocity.y < 0f) velocity.y = -2f;
            if (grounded && jump && !crouch)
            {
                // Oldinda past to'siq bo'lsa — chiqish, aks holda sakrash
                if (TryMantle(moveDir)) { UpdateAnimation(dt, 0f); return; }
                velocity.y = Mathf.Sqrt(jumpHeight * -2f * gravity);
                if (animator != null) animator.SetTrigger("Jump");
            }
            velocity.y += gravity * dt;

            // ---------- harakat ----------
            Vector3 h = moveDir * speed;
            // Qiyalik: tik nishabda pastga sirpanish
            if (grounded && Physics.Raycast(transform.position + Vector3.up * 0.3f, Vector3.down, out var hit, 1.2f))
            {
                float slope = Vector3.Angle(hit.normal, Vector3.up);
                if (slope > cc.slopeLimit)
                {
                    Vector3 slide = Vector3.ProjectOnPlane(Vector3.down, hit.normal).normalized;
                    h += slide * 4.5f;
                }
            }
            cc.Move((h + Vector3.up * velocity.y) * dt);

            // ---------- burilish: o'yinchi ILDIZI aylanadi ----------
            // Model ildizi emas: humanoid Animator o'z GameObject'ining burilishini
            // yozib qo'yishi mumkin (root rotation) — bu "qiyshiq yurish" bergan edi.
            transform.rotation = Quaternion.Slerp(transform.rotation, Quaternion.Euler(0f, yaw, 0f), 1f - Mathf.Exp(-14f * dt));
            if (model != null && animator != null) model.localRotation = Quaternion.Euler(0f, modelYawOffset, 0f);
            UpdateAnimation(dt, speed);
        }

        bool TryMantle(Vector3 dir)
        {
            Vector3 feet = transform.position;
            // To'siq bormi (bel balandligida)?
            if (!Physics.Raycast(feet + Vector3.up * 0.7f, dir, out var wall, mantleReach)) return false;
            // Uning tepasi bo'shmi?
            Vector3 top = wall.point + dir * 0.45f + Vector3.up * (mantleMaxHeight + 0.3f);
            if (!Physics.Raycast(top, Vector3.down, out var topHit, mantleMaxHeight + 0.3f)) return false;
            float h = topHit.point.y - feet.y;
            if (h < 0.5f || h > mantleMaxHeight) return false;
            if (Physics.CheckSphere(topHit.point + Vector3.up * 1.0f, 0.35f)) return false;   // turish joyi yo'q
            mantleFrom = feet;
            mantleTo = topHit.point + Vector3.up * 0.05f;
            mantleT = 0.55f;
            velocity = Vector3.zero;
            if (animator != null) animator.SetTrigger("Mantle");
            return true;
        }

        void UpdateAnimation(float dt, float spd)
        {
            if (animator != null)
            {
                animator.SetFloat("Speed", spd, 0.08f, dt);
                animator.SetBool("Grounded", grounded);
                animator.SetBool("Crouch", crouch);
                return;
            }
            // OBJ placeholder: qadam tebranishi (Mixamo rig kelguncha)
            if (model == null) return;
            bob += dt * (2.0f + spd * 1.6f);
            float amp = Mathf.Clamp01(spd / jogSpeed) * 0.05f;
            var lp = model.localPosition;
            lp.y = Mathf.Abs(Mathf.Sin(bob * Mathf.PI)) * amp + (crouch ? -0.25f : 0f);
            model.localPosition = lp;
            model.localRotation = Quaternion.Euler(Mathf.Clamp(spd, 0f, 7f) * 1.2f, modelYawOffset, Mathf.Sin(bob * Mathf.PI * 0.5f) * amp * 40f);
        }

        // Sinov uchun: Vector2 yo'nalishda N sekund yurish (Unity CLI eval orqali chaqiriladi)
        public void DebugWalk(float x, float y, bool sprint, float seconds)
        {
            debugMove = new Vector2(x, y); debugSprint = sprint;
            CancelInvoke(nameof(DebugStop));
            Invoke(nameof(DebugStop), seconds);
        }
        void DebugStop() { debugMove = Vector2.zero; debugSprint = false; }

        // O'z-o'zini sinash: 4 yo'nalish + sprint, har birida 1.6 s dan keyin o'lchov.
        // Natija debugReport da (CLI dan bir marta o'qiladi — eval kechikishiga bog'liq emas).
        public string debugReport = "";
        public void DebugRunTest() { StartCoroutine(RunTest()); }
        System.Collections.IEnumerator RunTest()
        {
            debugReport = "";
            var dirs = new[] { new Vector2(0, 1), new Vector2(1, 0), new Vector2(0, -1), new Vector2(-1, 0), new Vector2(0, 1) };
            for (int i = 0; i < dirs.Length; ++i)
            {
                bool sprint = i == dirs.Length - 1;
                debugMove = dirs[i]; debugSprint = sprint;
                yield return new WaitForSeconds(1.6f);
                Vector3 v = cc.velocity; v.y = 0f;
                Vector3 f = transform.forward;
                string hipsTxt = "-";
                string clip = "-";
                if (animator != null)
                {
                    var hb = animator.GetBoneTransform(HumanBodyBones.Hips);
                    if (hb != null) { Vector3 hf = hb.forward; hf.y = 0f; hipsTxt = Vector3.SignedAngle(hf.normalized, v.normalized, Vector3.up).ToString("F0"); }
                    var ci = animator.GetCurrentAnimatorClipInfo(0);
                    if (ci.Length > 0) clip = ci[0].clip.name;
                }
                debugReport += string.Format("[{0}{1}] speed={2:F2} rootVsMove={3:F0} hipsVsMove={4} clip={5} tilt={6:F0}/{7:F0}\n",
                    dirs[i], sprint ? " sprint" : "", v.magnitude, Vector3.SignedAngle(f, v.normalized, Vector3.up), hipsTxt, clip,
                    model != null ? model.eulerAngles.x : 0f, model != null ? model.eulerAngles.z : 0f);
            }
            debugMove = Vector2.zero; debugSprint = false;
            debugReport += "DONE";
            try { System.IO.File.WriteAllText(System.IO.Path.Combine(Application.dataPath, "..", "Temp", "ertugrul_selftest.txt"), debugReport); }
            catch (System.Exception e) { Debug.LogWarning("selftest yozilmadi: " + e.Message); }
        }
        public void DebugJump() { velocity.y = Mathf.Sqrt(jumpHeight * -2f * gravity); }
    }
}
