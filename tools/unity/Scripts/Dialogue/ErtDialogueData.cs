using System;
using UnityEngine;

namespace Ertugrul.Dialogue
{
    /// <summary>
    /// Dialog ma'lumoti — ScriptableObject.
    /// Yaratish: Project oynasida o'ng tugma → Create → Ertugrul → Dialogue
    ///
    /// Bu GRAF, chiziqli ro'yxat emas: har qator keyingi qatorga yoki
    /// tanlovga olib boradi. GDD dagi quest grafi bilan bir xil printsip.
    /// </summary>
    [CreateAssetMenu(fileName = "DLG_New", menuName = "Ertugrul/Dialogue", order = 10)]
    public class ErtDialogueData : ScriptableObject
    {
        [Header("Kimligi")]
        public string dialogueId = "DLG_TURGUT_01";

        [Tooltip("Bir marta o'ynalgandan keyin qayta o'ynalmaydimi?")]
        public bool playOnce = false;

        [Header("Qatorlar")]
        public ErtDialogueLine[] lines;

        [Header("Tugagandan keyin")]
        [Tooltip("Dialog tugagach qo'yiladigan flag'lar")]
        public string[] setFlagsOnComplete;

        [Tooltip("Dialog tugagach beriladigan quest (bo'sh qoldirish mumkin)")]
        public Quest.ErtQuestData grantQuest;

        [Tooltip("Dialog tugagach bajarilgan deb belgilanadigan quest bosqichi")]
        public string completeObjectiveId = "";

        [Tooltip("Iymon o'zgarishi (GDD: kechirish +15, va'da buzish -10)")]
        public float imanDelta = 0f;
    }

    [Serializable]
    public class ErtDialogueLine
    {
        [Tooltip("Kim gapiryapti — HUD'da ko'rsatiladi")]
        public string speaker = "Turgut";

        [TextArea(2, 5)]
        public string text = "...";

        [Tooltip("Bu qator ko'rinishi uchun kerakli flag (bo'sh — har doim)")]
        public string requiredFlag = "";

        [Tooltip("Ovoz fayli (ixtiyoriy)")]
        public AudioClip voiceOver;

        [Tooltip("Bu qatordan keyin tanlov chiqadimi?")]
        public ErtDialogueChoice[] choices;

        [Tooltip("Tanlov yo'q bo'lsa: keyingi qator indeksi. -1 = dialog tugadi.")]
        public int nextLineIndex = -1;
    }

    [Serializable]
    public class ErtDialogueChoice
    {
        [TextArea(1, 3)]
        public string text = "Javob varianti";

        [Tooltip("Tanlanganda o'tiladigan qator indeksi. -1 = dialog tugadi.")]
        public int gotoLineIndex = -1;

        [Tooltip("Tanlanganda qo'yiladigan flag")]
        public string setFlag = "";

        [Tooltip("Iymon o'zgarishi")]
        public float imanDelta = 0f;

        [Tooltip("Bu variant ko'rinishi uchun kerakli flag")]
        public string requiredFlag = "";
    }
}
