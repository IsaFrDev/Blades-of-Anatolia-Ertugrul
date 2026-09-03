using System.Text;
using UnityEngine;
using TMPro;
using Ertugrul.Core;

namespace Ertugrul.Quest
{
    /// <summary>
    /// HUD: joriy missiya va sana kartasi.
    /// GDD qoidasi — sana kartasi HAR DOIM pastki chapda va kichik.
    /// </summary>
    public class ErtQuestUI : MonoBehaviour
    {
        [Header("Missiya (yuqori chap)")]
        public TextMeshProUGUI questTitle;
        public TextMeshProUGUI questObjectives;

        [Header("Sana kartasi (pastki chap)")]
        public TextMeshProUGUI hijriText;
        public TextMeshProUGUI gregorianText;
        public TextMeshProUGUI placeText;

        [Header("Ranglar (Temir va Firuze)")]
        public Color doneColor    = new Color(0.28f, 0.66f, 0.71f);   // #48A9B5 feruza
        public Color pendingColor = new Color(0.89f, 0.91f, 0.91f);   // #E4EAEA
        public Color optionalColor= new Color(0.49f, 0.55f, 0.56f);   // #7C8B8F

        private ErtQuestManager _qm;

        private void Start()
        {
            var gm = ErtGameManager.Instance;
            if (gm != null)
            {
                if (hijriText != null)     hijriText.text     = gm.hijriDate;
                if (gregorianText != null) gregorianText.text = gm.gregorianDate;
                if (placeText != null)     placeText.text     = gm.placeName;

                _qm = gm.questManager;
            }

            if (_qm == null) _qm = FindFirstObjectByType<ErtQuestManager>();
            if (_qm == null) { Refresh(); return; }

            _qm.OnQuestStarted   += _ => Refresh();
            _qm.OnQuestUpdated   += _ => Refresh();
            _qm.OnQuestCompleted += _ => Refresh();
            Refresh();
        }

        private void Refresh()
        {
            if (questTitle == null || questObjectives == null) return;

            if (_qm == null || _qm.Active.Count == 0)
            {
                questTitle.text = "";
                questObjectives.text = "";
                return;
            }

            var q = _qm.Active[0];                 // hozircha birinchi faol quest
            questTitle.text = q.data.title.ToUpperInvariant();

            var sb = new StringBuilder();
            for (int i = 0; i < q.data.objectives.Length; i++)
            {
                var o = q.data.objectives[i];
                bool d = q.done[i];

                // Mix boshi glifi — GDD dagi asosiy motiv (kvadrat, ichida teshik)
                string marker = d ? "▪" : "▫";
                Color c = d ? doneColor : (o.optional ? optionalColor : pendingColor);
                string hex = ColorUtility.ToHtmlStringRGB(c);
                string text = d ? $"<s>{o.text}</s>" : o.text;
                string opt = o.optional ? "  <size=70%>(ixtiyoriy)</size>" : "";

                sb.AppendLine($"<color=#{hex}>{marker}  {text}{opt}</color>");
            }
            questObjectives.text = sb.ToString();
        }
    }
}
