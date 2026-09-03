// Epizod oqimi: sahna ochilganda daraja uchun intro cutscene o'ynaydi, keyin
// o'yinchiga boshqaruv beriladi va birinchi maqsad ko'rsatiladi.
using UnityEngine;

namespace Ertugrul
{
    public class ErtugrulEpisode : MonoBehaviour
    {
        public string levelId;
        public string episodeId;          // bo'sh bo'lsa daraja bo'yicha birinchi intro
        public bool playIntro = true;
        public string language = "uz";

        void Start()
        {
            ErtugrulLoc.Lang = language;
            ErtugrulLoc.Load();
            var cut = FindFirstObjectByType<ErtugrulCutscenePlayer>();
            var ui = ErtugrulUI.Instance;
            string id = null;
            if (playIntro && cut != null)
                id = string.IsNullOrEmpty(episodeId) ? ErtugrulCutscenePlayer.FindForLevel(levelId)
                                                     : episodeId.ToLower() + "_intro";
            if (id != null)
            {
                Debug.Log("Ertugrul: intro cutscene " + id);
                cut.Play(id, OnIntroDone);
            }
            else OnIntroDone();
        }

        void OnIntroDone()
        {
            var ui = ErtugrulUI.Instance;
            ui?.SetObjective(ErtugrulLoc.T("ui.obj.title") + "\n" + ErtugrulLoc.T("ui.obj.reach_point"));
        }

        public void DebugPlay(string cutsceneId)
        {
            var cut = FindFirstObjectByType<ErtugrulCutscenePlayer>();
            cut?.Play(cutsceneId, OnIntroDone);
        }
    }
}
