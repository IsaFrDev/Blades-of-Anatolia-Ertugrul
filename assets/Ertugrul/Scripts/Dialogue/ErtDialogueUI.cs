using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace Ertugrul.Dialogue
{
    /// <summary>
    /// Dialog oynasi (uGUI). Ekranning pastida panel: ism + matn + tanlovlar.
    /// Ranglar GDD «Temir va Firuze» palitrasidan.
    ///
    /// Bu skriptni Canvas ostidagi "DialoguePanel" ga qo'ying va
    /// Inspector'da maydonlarni bog'lang. Yoki Ertugrul > Sahnani sozlash
    /// menyusidan avtomatik yaratishingiz mumkin.
    /// </summary>
    public class ErtDialogueUI : MonoBehaviour
    {
        [Header("Panel")]
        public GameObject root;

        [Header("Matn")]
        public TextMeshProUGUI speakerText;
        public TextMeshProUGUI bodyText;
        public GameObject continueHint;      // "[E] davom etish"

        [Header("Tanlovlar")]
        public RectTransform choiceContainer;
        public Button choiceButtonPrefab;

        private readonly List<Button> _spawned = new();

        private void Awake()
        {
            if (root == null) root = gameObject;
            root.SetActive(false);
        }

        public void Open()
        {
            root.SetActive(true);
            if (continueHint != null) continueHint.SetActive(true);
            HideChoices();
        }

        public void Close()
        {
            HideChoices();
            root.SetActive(false);
        }

        public void SetLine(string speaker, string body)
        {
            if (speakerText != null) speakerText.text = speaker;
            if (bodyText != null)    bodyText.text = body;
        }

        public void ShowChoices(List<ErtDialogueChoice> choices, Action<int> onPicked)
        {
            HideChoices();
            if (continueHint != null) continueHint.SetActive(false);
            if (choiceContainer == null || choiceButtonPrefab == null)
            {
                // Prefab yo'q bo'lsa — birinchi variantni avtomatik tanlaymiz,
                // shunda o'yin qotib qolmaydi
                Debug.LogWarning("[DialogueUI] choiceButtonPrefab yo'q — 1-variant tanlandi.");
                onPicked?.Invoke(0);
                return;
            }

            for (int i = 0; i < choices.Count; i++)
            {
                int index = i;                                  // ⚠️ closure uchun nusxa
                var btn = Instantiate(choiceButtonPrefab, choiceContainer);
                btn.gameObject.SetActive(true);

                var label = btn.GetComponentInChildren<TextMeshProUGUI>();
                if (label != null) label.text = $"{i + 1}. {choices[i].text}";

                btn.onClick.AddListener(() => onPicked?.Invoke(index));
                _spawned.Add(btn);
            }

            // Birinchi tugmani fokusga olamiz (gamepad/klaviatura uchun)
            if (_spawned.Count > 0)
                UnityEngine.EventSystems.EventSystem.current?
                    .SetSelectedGameObject(_spawned[0].gameObject);
        }

        public void HideChoices()
        {
            foreach (var b in _spawned)
                if (b != null) Destroy(b.gameObject);
            _spawned.Clear();
            if (continueHint != null) continueHint.SetActive(true);
        }
    }
}
