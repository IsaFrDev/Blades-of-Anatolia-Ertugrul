using UnityEngine;
using Ertugrul.Quest;

namespace Ertugrul.Core
{
    /// <summary>
    /// O'yinning markaziy boshqaruvchisi (singleton).
    /// Sahnada BITTA bo'lishi kerak — "_GameManager" nomli bo'sh GameObject'ga qo'ying.
    ///
    /// Bu obyekt sahnalar orasida saqlanadi (DontDestroyOnLoad), shuning uchun
    /// Söğütdan Konyaga o'tganda dunyo holati yo'qolmaydi.
    /// </summary>
    [DefaultExecutionOrder(-100)]   // ⚠️ Boshqa skriptlardan OLDIN ishga tushadi
    public class ErtGameManager : MonoBehaviour
    {
        public static ErtGameManager Instance { get; private set; }

        [Header("Joriy epizod")]
        [Tooltip("episodes_v2.json dagi ID — masalan EP039")]
        public string currentEpisodeId = "EP039";

        [Tooltip("Hijriy sana — HUD'da ko'rsatiladi")]
        public string hijriDate = "642 Ramazon";

        [Tooltip("Milodiy sana")]
        public string gregorianDate = "1245 Fevral";

        [Tooltip("Joy nomi")]
        public string placeName = "SÖĞÜT";

        [Header("Tizimlar")]
        public ErtQuestManager questManager;

        /// <summary>Dunyo holati — flag'lar, iymon, obro'.</summary>
        public ErtWorldState World { get; private set; } = new ErtWorldState();

        private void Awake()
        {
            // Singleton: agar allaqachon bor bo'lsa — bu nusxani o'chir
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
            DontDestroyOnLoad(gameObject);

            if (questManager == null)
                questManager = GetComponentInChildren<ErtQuestManager>();

            // Boshlang'ich qiymatlar (GDD: Iymon 50 dan boshlanadi)
            if (World.GetScalar("iman", -1f) < 0f)
            {
                World.SetScalar("iman", 50f);
                World.SetScalar("sabr", 50f);
            }
        }

        private void Start()
        {
            Debug.Log($"[Ertugrul] {currentEpisodeId} boshlandi — " +
                      $"{hijriDate} · {gregorianDate} · {placeName}");
        }

        // ═══════════════════════════════════════════════════════════
        //  Qulay yordamchilar — boshqa skriptlar shulardan foydalanadi
        // ═══════════════════════════════════════════════════════════

        public static bool Flag(string f)
            => Instance != null && Instance.World.HasFlag(f);

        public static void SetFlag(string f, bool v = true)
            => Instance?.World.SetFlag(f, v);

        public static void AddIman(float delta)
        {
            if (Instance == null) return;
            float v = Mathf.Clamp(Instance.World.GetScalar("iman") + delta, 0f, 100f);
            Instance.World.SetScalar("iman", v);
            Debug.Log($"[Iymon] {delta:+0;-0} → {v:0}");
        }

        public static float Iman => Instance != null ? Instance.World.GetScalar("iman") : 50f;
    }
}
