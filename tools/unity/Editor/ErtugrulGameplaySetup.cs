// Menyu: Ertugrul > Setup Gameplay (current scene) / (ALL scenes)
// Sahnaga qo'shadi: Player (CharacterController + ErtugrulPlayer + OBJ model),
// Cinemachine (Brain + orbital kamera), CutscenePlayer (cast kutubxonasi bilan),
// UI Canvas, Episode oqimi. Mixamo FBX bo'lsa (Assets/Ertugrul/Characters/ertugrul.fbx)
// o'sha model + Animator ishlatiladi, aks holda ottoman.obj placeholder.
using System.IO;
using Unity.Cinemachine;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using Ertugrul;

namespace Ertugrul.EditorTools
{
    public static class ErtugrulGameplaySetup
    {
        const string ModelsRoot = "Assets/Ertugrul/Models/";
        const string MixamoModel = "Assets/Ertugrul/Characters/ertugrul.fbx";
        const string AnimatorPath = "Assets/Ertugrul/Characters/Ertugrul.controller";

        [MenuItem("Ertugrul/Setup Gameplay (current scene)")]
        public static void SetupCurrent() { Setup(); EditorSceneManager.MarkSceneDirty(EditorSceneManager.GetActiveScene()); }

        [MenuItem("Ertugrul/Setup Gameplay (ALL scenes)")]
        public static void SetupAll()
        {
            foreach (var f in Directory.GetFiles("Assets/Ertugrul/Scenes", "*.unity"))
            {
                var sc = EditorSceneManager.OpenScene(f.Replace('\\', '/'), OpenSceneMode.Single);
                Setup();
                EditorSceneManager.SaveScene(sc);
                Debug.Log("Ertugrul: gameplay o'rnatildi " + f);
            }
        }

        public static void Setup()
        {
            // --- eski o'rnatmani tozalash ---
            foreach (var n in new[] { "Player", "PlayerCamera", "CutscenePlayer", "ErtugrulUI", "Episode" })
            { var old = GameObject.Find(n); if (old != null) Object.DestroyImmediate(old); }

            var spawn = GameObject.Find("Spawn_player");
            Vector3 sp = spawn != null ? spawn.transform.position : Vector3.zero;
            float yaw = spawn != null ? spawn.transform.eulerAngles.y : 0f;
            string levelId = "";
            foreach (var go in Object.FindObjectsByType<GameObject>(FindObjectsSortMode.None))
                if (go.name.StartsWith("Level_")) { levelId = go.name.Substring(6); break; }

            // --- Player ---
            var player = new GameObject("Player");
            player.transform.position = sp + Vector3.up * 0.1f;
            player.transform.rotation = Quaternion.Euler(0f, yaw, 0f);
            var cc = player.AddComponent<CharacterController>();
            cc.height = 1.8f; cc.radius = 0.35f; cc.center = new Vector3(0f, 0.95f, 0f);
            cc.slopeLimit = 48f; cc.stepOffset = 0.45f;
            var pl = player.AddComponent<ErtugrulPlayer>();

            // Rigli model: Characters/ertugrul.fbx yoki papkadagi SkinnedMeshRenderer'li birinchi FBX
            GameObject modelPrefab = AssetDatabase.LoadAssetAtPath<GameObject>(MixamoModel);
            if (modelPrefab == null && Directory.Exists("Assets/Ertugrul/Characters"))
                foreach (var f in Directory.GetFiles("Assets/Ertugrul/Characters", "*.fbx"))
                {
                    var g = AssetDatabase.LoadAssetAtPath<GameObject>(f.Replace('\\', '/'));
                    if (g != null && g.GetComponentInChildren<SkinnedMeshRenderer>() != null) { modelPrefab = g; break; }
                }
            bool rigged = modelPrefab != null;
            if (!rigged) modelPrefab = AssetDatabase.LoadAssetAtPath<GameObject>(ModelsRoot + "ottoman/ottoman.obj");
            if (modelPrefab != null)
            {
                // Pivot: model ofseti shu yerda; Animator (rigli model) o'z ildizini yozsa ham pivotga tegmaydi
                var pivot = new GameObject("Model").transform;
                pivot.SetParent(player.transform, false);
                var m = (GameObject)PrefabUtility.InstantiatePrefab(modelPrefab, pivot);
                m.name = rigged ? "Rig" : "Mesh";
                if (!rigged) ErtugrulCutscenePlayer.FitToHeight(m.transform, 1.82f);
                else
                {
                    var anim = m.GetComponent<Animator>() ?? m.AddComponent<Animator>();
                    var ctrl = AssetDatabase.LoadAssetAtPath<RuntimeAnimatorController>(AnimatorPath);
                    if (ctrl != null) anim.runtimeAnimatorController = ctrl;
                    anim.applyRootMotion = false;
                    pl.animator = anim;
                    if (m.GetComponent<ErtugrulFootsteps>() == null) m.AddComponent<ErtugrulFootsteps>();
                }
                pl.model = pivot;
            }

            // --- Kamera ---
            var mainCam = Camera.main;
            if (mainCam == null)
            {
                var cgo = new GameObject("Main Camera", typeof(Camera), typeof(AudioListener));
                cgo.tag = "MainCamera"; mainCam = cgo.GetComponent<Camera>();
            }
            mainCam.farClipPlane = 900f;
            if (mainCam.GetComponent<CinemachineBrain>() == null) mainCam.gameObject.AddComponent<CinemachineBrain>();

            var camGo = new GameObject("PlayerCamera");
            var vcam = camGo.AddComponent<CinemachineCamera>();
            vcam.Priority = 10;
            vcam.Lens.FieldOfView = 46f;
            var target = new GameObject("CameraTarget");
            target.transform.SetParent(player.transform, false);
            target.transform.localPosition = new Vector3(0f, 1.5f, 0f);
            vcam.Follow = target.transform;
            vcam.LookAt = target.transform;
            var orbital = camGo.AddComponent<CinemachineOrbitalFollow>();
            orbital.OrbitStyle = CinemachineOrbitalFollow.OrbitStyles.Sphere;
            // DUNYO fazosi: aks holda kamera o'yinchi burilishi bilan birga aylanadi va
            // kamera-nisbiy kiritish bilan cheksiz aylanish (qiyshiq yurish) paydo bo'ladi
            orbital.TrackerSettings.BindingMode = Unity.Cinemachine.TargetTracking.BindingMode.WorldSpace;
            orbital.Radius = 5.2f;
            orbital.HorizontalAxis.Value = yaw;
            orbital.VerticalAxis.Value = 12f;
            orbital.VerticalAxis.Range = new Vector2(-30f, 70f);
            camGo.AddComponent<CinemachineRotationComposer>();
            var decol = camGo.AddComponent<CinemachineDecollider>();
            decol.CameraRadius = 0.3f;
            camGo.AddComponent<ErtugrulCamera>();
            pl.cameraTransform = mainCam.transform;

            // --- Cutscene ---
            var cut = new GameObject("CutscenePlayer").AddComponent<ErtugrulCutscenePlayer>();
            foreach (var guid in AssetDatabase.FindAssets("t:Model", new[] { "Assets/Ertugrul/Models" }))
            {
                string p = AssetDatabase.GUIDToAssetPath(guid);
                if (!p.EndsWith(".obj")) continue;
                var pf = AssetDatabase.LoadAssetAtPath<GameObject>(p);
                if (pf != null) cut.cast.Add(new CastEntry { path = p.Substring(ModelsRoot.Length), prefab = pf });
            }

            // --- UI + Episode ---
            new GameObject("ErtugrulUI", typeof(Canvas)).AddComponent<ErtugrulUI>();
            if (Object.FindFirstObjectByType<UnityEngine.EventSystems.EventSystem>() == null)
                new GameObject("EventSystem", typeof(UnityEngine.EventSystems.EventSystem),
                               typeof(UnityEngine.InputSystem.UI.InputSystemUIInputModule));
            var ep = new GameObject("Episode").AddComponent<ErtugrulEpisode>();
            ep.levelId = levelId;

            Debug.Log("Ertugrul: gameplay o'rnatildi — daraja " + levelId + ", model " + (rigged ? "Mixamo rig" : "OBJ placeholder") +
                      ", cast " + cut.cast.Count + " model");
        }
    }
}
