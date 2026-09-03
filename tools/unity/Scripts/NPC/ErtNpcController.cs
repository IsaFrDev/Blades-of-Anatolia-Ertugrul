using System.Collections;
using UnityEngine;
using UnityEngine.AI;
using Ertugrul.Dialogue;

namespace Ertugrul.NPC
{
    /// <summary>
    /// NPC: NavMesh bo'ylab yuradi, o'yinchi kelganda to'xtaydi va gaplashadi.
    ///
    /// TALAB:
    ///   1. Sahnada NavMesh baked bo'lishi kerak (NavMeshSurface → Bake)
    ///   2. Bu GameObject'da NavMeshAgent komponenti
    ///   3. Collider (odatda CapsuleCollider) — interaksiya uchun
    ///
    /// Rejimlar:
    ///   Idle    — joyida turadi, atrofga qaraydi
    ///   Patrol  — waypoint'lar bo'ylab yuradi
    ///   Wander  — tasodifiy nuqtalarga boradi (bozor, oba uchun)
    /// </summary>
    [RequireComponent(typeof(NavMeshAgent))]
    public class ErtNpcController : MonoBehaviour, IErtInteractable
    {
        public enum Mode { Idle, Patrol, Wander }

        [Header("Kimligi")]
        public string npcId = "CHR_TURGUT";
        public string displayName = "Turgut";

        [Header("Harakat")]
        public Mode mode = Mode.Patrol;
        public float walkSpeed = 1.6f;

        [Tooltip("Patrol rejimi uchun: bo'sh GameObject'lar yarating va shu yerga qo'ying")]
        public Transform[] waypoints;

        [Tooltip("Har waypoint'da necha soniya kutadi")]
        public Vector2 waitAtPointRange = new Vector2(2f, 5f);

        [Tooltip("Wander rejimi uchun: markazdan necha metr radiusda yuradi")]
        public float wanderRadius = 12f;

        [Header("Dialog")]
        [Tooltip("Bu NPC gapiradigan dialog (ScriptableObject). Bo'sh bo'lsa gaplashmaydi.")]
        public ErtDialogueData dialogue;

        [Tooltip("Gaplashish uchun kerakli flag (bo'sh — har doim mumkin)")]
        public string requiredFlag = "";

        [Tooltip("Bu flag qo'yilgan bo'lsa NPC boshqa gaplashmaydi")]
        public string blockedByFlag = "";

        [Header("Prompt")]
        [Tooltip("Prompt qayerda chiqadi. Bo'sh — boshning tepasi avtomatik hisoblanadi.")]
        public Transform promptAnchor;

        private NavMeshAgent _agent;
        private int _waypointIndex;
        private bool _busy;                 // dialog paytida true
        private Vector3 _homePosition;
        private Coroutine _routine;

        private void Awake()
        {
            _agent = GetComponent<NavMeshAgent>();
            _agent.speed = walkSpeed;
            _homePosition = transform.position;

            if (promptAnchor == null)
            {
                // Boshning tepasida avtomatik nuqta yaratamiz
                var go = new GameObject("PromptAnchor");
                go.transform.SetParent(transform);
                go.transform.localPosition = new Vector3(0f, 2.1f, 0f);
                promptAnchor = go.transform;
            }
        }

        private void Start()
        {
            // ⚠️ NavMesh baked emasmi — tekshiramiz, aks holda NPC qimirlamaydi
            if (!_agent.isOnNavMesh)
            {
                Debug.LogError(
                    $"[{displayName}] NavMesh ustida emas! " +
                    $"Sahnada NavMeshSurface qo'shib Bake qiling " +
                    $"(Window > AI > Navigation).", this);
                enabled = false;
                return;
            }

            _routine = StartCoroutine(Behaviour());
        }

        // ═══════════════════════════════════════════════════════════
        //  XATTI-HARAKAT
        // ═══════════════════════════════════════════════════════════

        private IEnumerator Behaviour()
        {
            while (true)
            {
                if (_busy || mode == Mode.Idle)
                {
                    yield return null;
                    continue;
                }

                Vector3 target = mode == Mode.Patrol ? NextWaypoint() : RandomPointNearHome();

                if (NavMesh.SamplePosition(target, out var hit, 4f, NavMesh.AllAreas))
                {
                    _agent.isStopped = false;
                    _agent.SetDestination(hit.position);

                    // Yetib borguncha kutamiz
                    while (!_busy &&
                           (_agent.pathPending ||
                            _agent.remainingDistance > _agent.stoppingDistance + 0.1f))
                        yield return null;
                }

                // Joyida kutish
                float wait = Random.Range(waitAtPointRange.x, waitAtPointRange.y);
                float t = 0f;
                while (t < wait && !_busy) { t += Time.deltaTime; yield return null; }
            }
        }

        private Vector3 NextWaypoint()
        {
            if (waypoints == null || waypoints.Length == 0) return _homePosition;
            Vector3 p = waypoints[_waypointIndex].position;
            _waypointIndex = (_waypointIndex + 1) % waypoints.Length;
            return p;
        }

        private Vector3 RandomPointNearHome()
            => _homePosition + new Vector3(
                   Random.Range(-wanderRadius, wanderRadius), 0f,
                   Random.Range(-wanderRadius, wanderRadius));

        // ═══════════════════════════════════════════════════════════
        //  IErtInteractable
        // ═══════════════════════════════════════════════════════════

        public string GetPrompt() => $"{displayName} bilan gaplashish";

        public bool CanInteract()
        {
            if (_busy || dialogue == null) return false;
            if (!string.IsNullOrEmpty(requiredFlag) && !Core.ErtGameManager.Flag(requiredFlag))
                return false;
            if (!string.IsNullOrEmpty(blockedByFlag) && Core.ErtGameManager.Flag(blockedByFlag))
                return false;
            return true;
        }

        public Transform GetTransform() => promptAnchor != null ? promptAnchor : transform;

        public void Interact(GameObject interactor)
        {
            if (ErtDialogueRunner.Instance == null)
            {
                Debug.LogError("[NPC] Sahnada ErtDialogueRunner yo'q!", this);
                return;
            }

            _busy = true;
            _agent.isStopped = true;
            StartCoroutine(FacePlayer(interactor.transform));

            ErtDialogueRunner.Instance.StartDialogue(dialogue, this, () =>
            {
                _busy = false;
                if (_agent.isOnNavMesh) _agent.isStopped = false;
            });
        }

        /// <summary>Gaplashganda NPC o'yinchiga qaraydi — kichik detal, katta farq.</summary>
        private IEnumerator FacePlayer(Transform player)
        {
            float t = 0f;
            Vector3 dir = player.position - transform.position;
            dir.y = 0f;
            if (dir.sqrMagnitude < 0.01f) yield break;

            Quaternion from = transform.rotation;
            Quaternion to = Quaternion.LookRotation(dir);
            while (t < 1f)
            {
                t += Time.deltaTime * 3.5f;
                transform.rotation = Quaternion.Slerp(from, to, t);
                yield return null;
            }
        }

        // Sahnada patrul yo'lini ko'rsatish
        private void OnDrawGizmosSelected()
        {
            if (waypoints == null || waypoints.Length < 2) return;
            Gizmos.color = new Color(0.28f, 0.66f, 0.71f);
            for (int i = 0; i < waypoints.Length; i++)
            {
                if (waypoints[i] == null) continue;
                var next = waypoints[(i + 1) % waypoints.Length];
                if (next == null) continue;
                Gizmos.DrawLine(waypoints[i].position, next.position);
                Gizmos.DrawWireSphere(waypoints[i].position, 0.35f);
            }
        }
    }
}
