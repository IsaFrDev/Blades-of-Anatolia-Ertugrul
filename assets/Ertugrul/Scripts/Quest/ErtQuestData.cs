using System;
using UnityEngine;

namespace Ertugrul.Quest
{
    /// <summary>
    /// Missiya (quest) ma'lumoti — ScriptableObject.
    /// Yaratish: Project oynasida o'ng tugma → Create → Ertugrul → Quest
    ///
    /// GDD dagi `UErtQuestGraph` ning soddalashtirilgan Unity versiyasi:
    /// bosqichlar ketma-ket, lekin ba'zilari ixtiyoriy bo'lishi mumkin.
    /// </summary>
    [CreateAssetMenu(fileName = "QST_New", menuName = "Ertugrul/Quest", order = 11)]
    public class ErtQuestData : ScriptableObject
    {
        [Header("Kimligi")]
        public string questId = "QST_EP039_SOGUT";
        public string title = "Söğütga o'rnashish";

        [TextArea(2, 4)]
        public string description =
            "Bo'sh qishloqni ko'rib chiq. Kim yashayotganini bil.";

        [Tooltip("Qaysi epizodga tegishli — episodes_v2.json bilan mos")]
        public string episodeId = "EP039";

        [Header("Bosqichlar")]
        public ErtObjective[] objectives;

        [Header("Mukofot")]
        public float imanReward = 0f;
        public int dirhamReward = 0;

        [Tooltip("Quest tugagach qo'yiladigan flag'lar")]
        public string[] setFlagsOnComplete;

        [Tooltip("Quest tugagach ochiladigan kodeks yozuvlari (CDX_...)")]
        public string[] codexUnlocks;
    }

    [Serializable]
    public class ErtObjective
    {
        [Tooltip("Noyob ID — dialog va trigger'lar shu ID bo'yicha bajarilgan deb belgilaydi")]
        public string objectiveId = "OBJ_TALK_TO_ELDER";

        [TextArea(1, 3)]
        public string text = "Keksa yunon bilan gaplash";

        [Tooltip("Ixtiyoriy bosqich — bajarilmasa ham quest tugaydi")]
        public bool optional = false;

        [Tooltip("Xaritada/dunyoda ko'rsatiladigan nuqta (ixtiyoriy)")]
        public Vector3 markerPosition;
        public bool hasMarker = false;
    }
}
