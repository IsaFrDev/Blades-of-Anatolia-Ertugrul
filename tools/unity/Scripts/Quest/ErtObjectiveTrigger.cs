using UnityEngine;
using Ertugrul.Core;

namespace Ertugrul.Quest
{
    /// <summary>
    /// Hududga kirganda missiya bosqichini bajarilgan deb belgilaydi.
    ///
    /// Ishlatish:
    ///   1. Bo'sh GameObject yarating
    ///   2. Box Collider qo'shing va "Is Trigger" ni yoqing
    ///   3. Bu skriptni qo'shing va objectiveId ni yozing
    /// </summary>
    [RequireComponent(typeof(Collider))]
    public class ErtObjectiveTrigger : MonoBehaviour
    {
        [Header("Nima bajariladi")]
        [Tooltip("ErtQuestData dagi objectiveId bilan bir xil bo'lishi kerak")]
        public string objectiveId = "OBJ_REACH_VILLAGE";

        [Tooltip("Qo'shimcha flag (ixtiyoriy)")]
        public string setFlag = "";

        [Header("Shartlar")]
        public string requiredTag = "Player";

        [Tooltip("Bir marta ishlaydimi?")]
        public bool once = true;

        [Tooltip("Ishlagach GameObject o'chirilsinmi?")]
        public bool disableAfter = true;

        private bool _fired;

        private void Reset()
        {
            // Qulaylik: skript qo'shilganda collider avtomatik trigger bo'ladi
            var col = GetComponent<Collider>();
            if (col != null) col.isTrigger = true;
        }

        private void OnTriggerEnter(Collider other)
        {
            if (_fired && once) return;
            if (!string.IsNullOrEmpty(requiredTag) && !other.CompareTag(requiredTag)) return;

            _fired = true;

            var qm = ErtGameManager.Instance != null
                ? ErtGameManager.Instance.questManager
                : FindFirstObjectByType<ErtQuestManager>();

            qm?.CompleteObjective(objectiveId);

            if (!string.IsNullOrEmpty(setFlag))
                ErtGameManager.SetFlag(setFlag);

            if (disableAfter) gameObject.SetActive(false);
        }

        private void OnDrawGizmos()
        {
            var col = GetComponent<Collider>();
            if (col == null) return;
            Gizmos.color = new Color(0.28f, 0.66f, 0.71f, 0.22f);
            Gizmos.DrawCube(col.bounds.center, col.bounds.size);
            Gizmos.color = new Color(0.28f, 0.66f, 0.71f, 0.8f);
            Gizmos.DrawWireCube(col.bounds.center, col.bounds.size);
        }
    }
}
