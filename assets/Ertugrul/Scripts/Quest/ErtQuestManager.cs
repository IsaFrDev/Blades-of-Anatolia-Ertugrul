using System;
using System.Collections.Generic;
using UnityEngine;
using Ertugrul.Core;

namespace Ertugrul.Quest
{
    /// <summary>
    /// Missiyalarni boshqaruvchi. ErtGameManager bilan bitta GameObject'da bo'lsin.
    ///
    /// Ishlatish:
    ///   questManager.StartQuest(quest);                    // boshlash
    ///   questManager.CompleteObjective("OBJ_TALK_ELDER");  // bosqichni yopish
    /// Bosqichlarning hammasi (ixtiyoriylardan tashqari) bajarilsa —
    /// quest avtomatik tugaydi.
    /// </summary>
    public class ErtQuestManager : MonoBehaviour
    {
        [Header("Boshlanishda beriladigan quest (ixtiyoriy)")]
        public ErtQuestData startingQuest;

        public event Action<ErtActiveQuest> OnQuestStarted;
        public event Action<ErtActiveQuest> OnQuestUpdated;
        public event Action<ErtActiveQuest> OnQuestCompleted;

        private readonly List<ErtActiveQuest> _active = new();
        private readonly HashSet<string> _completed = new();

        public IReadOnlyList<ErtActiveQuest> Active => _active;

        private void Start()
        {
            if (startingQuest != null) StartQuest(startingQuest);
        }

        // ═══════════════════════════════════════════════════════════

        public void StartQuest(ErtQuestData data)
        {
            if (data == null) return;
            if (_completed.Contains(data.questId)) return;
            if (_active.Exists(q => q.data.questId == data.questId)) return;

            var aq = new ErtActiveQuest(data);
            _active.Add(aq);
            OnQuestStarted?.Invoke(aq);
            Debug.Log($"[Quest] Boshlandi: {data.title}");
        }

        /// <summary>Bosqichni bajarilgan deb belgilaydi. ID butun o'yin bo'ylab noyob.</summary>
        public void CompleteObjective(string objectiveId)
        {
            if (string.IsNullOrEmpty(objectiveId)) return;

            for (int i = _active.Count - 1; i >= 0; i--)
            {
                var q = _active[i];
                if (!q.CompleteObjective(objectiveId)) continue;

                Debug.Log($"[Quest] Bosqich bajarildi: {objectiveId}");
                OnQuestUpdated?.Invoke(q);

                if (q.IsComplete) FinishQuest(q);
            }
        }

        public bool IsQuestActive(string questId)
            => _active.Exists(q => q.data.questId == questId);

        public bool IsQuestCompleted(string questId) => _completed.Contains(questId);

        private void FinishQuest(ErtActiveQuest q)
        {
            _active.Remove(q);
            _completed.Add(q.data.questId);

            if (!Mathf.Approximately(q.data.imanReward, 0f))
                ErtGameManager.AddIman(q.data.imanReward);

            if (q.data.dirhamReward != 0 && ErtGameManager.Instance != null)
                ErtGameManager.Instance.World.AddScalar("dirham", q.data.dirhamReward);

            if (q.data.setFlagsOnComplete != null)
                foreach (var f in q.data.setFlagsOnComplete)
                    if (!string.IsNullOrEmpty(f)) ErtGameManager.SetFlag(f);

            if (q.data.codexUnlocks != null)
                foreach (var c in q.data.codexUnlocks)
                    if (!string.IsNullOrEmpty(c)) ErtGameManager.SetFlag($"Codex.{c}");

            OnQuestCompleted?.Invoke(q);
            Debug.Log($"[Quest] ✅ TUGADI: {q.data.title}");
        }
    }

    /// <summary>Runtime'dagi faol quest holati.</summary>
    public class ErtActiveQuest
    {
        public readonly ErtQuestData data;
        public readonly bool[] done;

        public ErtActiveQuest(ErtQuestData d)
        {
            data = d;
            done = new bool[d.objectives != null ? d.objectives.Length : 0];
        }

        public bool CompleteObjective(string id)
        {
            if (data.objectives == null) return false;
            for (int i = 0; i < data.objectives.Length; i++)
            {
                if (data.objectives[i].objectiveId != id || done[i]) continue;
                done[i] = true;
                return true;
            }
            return false;
        }

        /// <summary>Barcha majburiy bosqichlar bajarilganmi?</summary>
        public bool IsComplete
        {
            get
            {
                if (data.objectives == null) return true;
                for (int i = 0; i < data.objectives.Length; i++)
                    if (!done[i] && !data.objectives[i].optional) return false;
                return true;
            }
        }

        public int DoneCount
        {
            get { int n = 0; foreach (var d in done) if (d) n++; return n; }
        }
    }
}
