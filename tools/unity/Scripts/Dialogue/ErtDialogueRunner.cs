using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;
using Ertugrul.Core;

namespace Ertugrul.Dialogue
{
    /// <summary>
    /// Dialogni o'ynatuvchi. Sahnada BITTA bo'lishi kerak.
    /// UI ni o'zi boshqarmaydi — ErtDialogueUI ga buyruq beradi.
    ///
    /// Oqim:
    ///   StartDialogue() → qator ko'rsat → [E] → keyingi qator →
    ///   tanlov bo'lsa → o'yinchi tanlaydi → o'sha qatorga o'tadi → ... → tugash
    /// </summary>
    public class ErtDialogueRunner : MonoBehaviour
    {
        public static ErtDialogueRunner Instance { get; private set; }

        [Header("Bog'lanishlar")]
        public ErtDialogueUI ui;
        public AudioSource voiceSource;

        [Header("Yozuv tezligi")]
        [Tooltip("Harflar soniyasiga — 0 bo'lsa darhol chiqadi")]
        public float charsPerSecond = 45f;

        public bool IsRunning { get; private set; }

        private readonly HashSet<string> _playedOnce = new();
        private Ertugrul.ErtugrulPlayer _player;   // ErtugrulPlayer (Runtime/) bilan bog'landi

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;

            if (ui == null) ui = FindFirstObjectByType<ErtDialogueUI>(FindObjectsInactive.Include);
        }

        public void StartDialogue(ErtDialogueData data, NPC.ErtNpcController speaker, Action onComplete)
        {
            if (IsRunning || data == null || data.lines == null || data.lines.Length == 0)
            {
                onComplete?.Invoke();
                return;
            }
            if (data.playOnce && _playedOnce.Contains(data.dialogueId))
            {
                onComplete?.Invoke();
                return;
            }

            StartCoroutine(Run(data, onComplete));
        }

        private IEnumerator Run(ErtDialogueData data, Action onComplete)
        {
            IsRunning = true;
            LockPlayer(true);
            ui?.Open();

            int index = 0;
            int safety = 0;   // ⚠️ cheksiz sikldan himoya

            while (index >= 0 && index < data.lines.Length && safety++ < 500)
            {
                var line = data.lines[index];

                // Shartga mos kelmasa — keyingisiga o'tamiz
                if (!string.IsNullOrEmpty(line.requiredFlag) &&
                    !ErtGameManager.Flag(line.requiredFlag))
                {
                    index = line.nextLineIndex;
                    continue;
                }

                // ── Qatorni yozib chiqamiz ──────────────────────────
                if (voiceSource != null && line.voiceOver != null)
                    voiceSource.PlayOneShot(line.voiceOver);

                yield return TypeLine(line);

                // ── Tanlov bormi? ───────────────────────────────────
                if (line.choices != null && line.choices.Length > 0)
                {
                    var valid = new List<ErtDialogueChoice>();
                    foreach (var c in line.choices)
                        if (string.IsNullOrEmpty(c.requiredFlag) || ErtGameManager.Flag(c.requiredFlag))
                            valid.Add(c);

                    if (valid.Count > 0)
                    {
                        int picked = -1;
                        ui?.ShowChoices(valid, i => picked = i);
                        while (picked < 0) yield return null;
                        ui?.HideChoices();

                        var choice = valid[picked];
                        if (!string.IsNullOrEmpty(choice.setFlag))
                            ErtGameManager.SetFlag(choice.setFlag);
                        if (!Mathf.Approximately(choice.imanDelta, 0f))
                            ErtGameManager.AddIman(choice.imanDelta);

                        index = choice.gotoLineIndex;
                        continue;
                    }
                }

                // ── Tanlov yo'q: [E] kutamiz ────────────────────────
                yield return WaitForAdvance();
                index = line.nextLineIndex;
            }

            // ── Yakuniy ta'sirlar ───────────────────────────────────
            if (data.setFlagsOnComplete != null)
                foreach (var f in data.setFlagsOnComplete)
                    if (!string.IsNullOrEmpty(f)) ErtGameManager.SetFlag(f);

            if (!Mathf.Approximately(data.imanDelta, 0f))
                ErtGameManager.AddIman(data.imanDelta);

            var qm = ErtGameManager.Instance != null ? ErtGameManager.Instance.questManager : null;
            if (qm != null)
            {
                if (data.grantQuest != null) qm.StartQuest(data.grantQuest);
                if (!string.IsNullOrEmpty(data.completeObjectiveId))
                    qm.CompleteObjective(data.completeObjectiveId);
            }

            if (data.playOnce) _playedOnce.Add(data.dialogueId);

            ui?.Close();
            LockPlayer(false);
            IsRunning = false;
            onComplete?.Invoke();
        }

        private IEnumerator TypeLine(ErtDialogueLine line)
        {
            if (ui == null) yield break;

            if (charsPerSecond <= 0f)
            {
                ui.SetLine(line.speaker, line.text);
                yield break;
            }

            ui.SetLine(line.speaker, "");
            float shown = 0f;
            var kb = Keyboard.current;

            while (shown < line.text.Length)
            {
                // [E] bosilsa — qolgan matnni darhol ko'rsatamiz
                if (kb != null && kb.eKey.wasPressedThisFrame) break;

                shown += charsPerSecond * Time.deltaTime;
                ui.SetLine(line.speaker, line.text[..Mathf.Min((int)shown, line.text.Length)]);
                yield return null;
            }
            ui.SetLine(line.speaker, line.text);
            yield return null;   // shu kadrdagi bosishni "yeb qo'ymaslik" uchun
        }

        private IEnumerator WaitForAdvance()
        {
            var kb = Keyboard.current;
            while (kb == null || !kb.eKey.wasPressedThisFrame)
            {
                kb = Keyboard.current;
                yield return null;
            }
        }

        private void LockPlayer(bool locked)
        {
            if (_player == null)
                _player = FindFirstObjectByType<Ertugrul.ErtugrulPlayer>();
            if (_player != null) _player.inputEnabled = !locked;
        }
    }
}
