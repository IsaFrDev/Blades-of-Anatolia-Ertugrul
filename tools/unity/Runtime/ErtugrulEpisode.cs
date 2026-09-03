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

        // Avto-sinov: Assets/Ertugrul/SELFTEST fayli bo'lsa intro o'tkaziladi, (editorda)
        // mannequin + Animator qo'yiladi va ErtugrulPlayer.DebugRunTest() ishga tushadi;
        // hisobot Temp/ertugrul_selftest.txt ga yoziladi.
        void SelfTestIfRequested()
        {
            string flag = System.IO.Path.Combine(Application.dataPath, "Ertugrul", "SELFTEST");
            if (!System.IO.File.Exists(flag)) return;
            playIntro = false;
            var pl = FindFirstObjectByType<ErtugrulPlayer>();
            if (pl == null) return;
#if UNITY_EDITOR
            string mode = System.IO.File.ReadAllText(flag).Trim();
            // "armature" yoki "armature:-90" (model ofseti sinov uchun)
            if (mode.StartsWith("armature") && pl.animator == null)
            {
                int colon = mode.IndexOf(':');
                if (colon > 0) float.TryParse(mode.Substring(colon + 1), System.Globalization.NumberStyles.Float,
                                              System.Globalization.CultureInfo.InvariantCulture, out pl.modelYawOffset);
                var arm = UnityEditor.AssetDatabase.LoadAssetAtPath<GameObject>("Assets/SourceFiles/StarterAssets/ThirdPersonController/Character/Models/Armature.fbx");
                var ctrl = UnityEditor.AssetDatabase.LoadAssetAtPath<RuntimeAnimatorController>("Assets/Ertugrul/Characters/Ertugrul.controller");
                if (arm != null && ctrl != null)
                {
                    if (pl.model != null) pl.model.gameObject.SetActive(false);
                    // PIVOT: Animator o'z GameObject'ining burilishini yozib qo'yadi, shuning uchun
                    // ofset alohida ota obyektda — Animator unga tegmaydi
                    var pivot = new GameObject("ModelPivot").transform;
                    pivot.SetParent(pl.transform, false);
                    pivot.localRotation = Quaternion.Euler(0f, pl.modelYawOffset, 0f);
                    var m = Instantiate(arm, pivot);
                    m.name = "ModelTest"; m.transform.localPosition = Vector3.zero; m.transform.localRotation = Quaternion.identity;
                    var an = m.GetComponent<Animator>() ?? m.AddComponent<Animator>();
                    an.runtimeAnimatorController = ctrl; an.applyRootMotion = false;
                    m.AddComponent<ErtugrulFootsteps>();
                    pl.animator = an; pl.model = pivot;
                }
            }
#endif
            Debug.Log("Ertugrul: SELFTEST boshlandi");
            StartCoroutine(SelfTestLater(pl));
        }
        System.Collections.IEnumerator SelfTestLater(ErtugrulPlayer pl)
        {
            yield return new WaitForSeconds(1.5f);
            pl.DebugRunTest();
        }

        void Start()
        {
            ErtugrulLoc.Lang = language;
            ErtugrulLoc.Load();
            SelfTestIfRequested();
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
