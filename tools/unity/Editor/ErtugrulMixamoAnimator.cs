// Humanoid Animator quruvchi — Mixamo yoki Unity Starter Assets kliplaridan.
//
// Kliplar KALIT SO'Z bo'yicha qidiriladi (fayl nomi katta-kichik harfsiz):
//   idle | walk | run | sprint | jump | crouch idle | crouch walk | mantle/climb
// Qidiruv papkalari: Assets/Ertugrul/Characters (Mixamo), keyin
// Assets/SourceFiles/StarterAssets (Unity mannequin animatsiyalari — humanoid
// retargeting tufayli Mixamo rigda ham ishlaydi).
//
// Mixamo (mixamo.com, Adobe hisobi): Upload character -> ottoman.obj -> autorig;
// "FBX for Unity": ertugrul.fbx (With Skin) + animatsiyalar (Without Skin) ->
// Assets/Ertugrul/Characters/. Keyin: Ertugrul > Build Animator, Ertugrul > Setup Gameplay.
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.Animations;
using UnityEngine;

namespace Ertugrul.EditorTools
{
    public static class ErtugrulMixamoAnimator
    {
        const string Dir = "Assets/Ertugrul/Characters";
        const string Ctrl = Dir + "/Ertugrul.controller";
        static readonly string[] SearchDirs = { Dir, "Assets/SourceFiles/StarterAssets" };

        [MenuItem("Ertugrul/Build Animator (Mixamo / Starter Assets)")]
        public static void Build()
        {
            Directory.CreateDirectory(Dir);
            // Characters dagi FBX'lar: humanoid, loop
            foreach (var f in Directory.GetFiles(Dir, "*.fbx"))
            {
                string p = f.Replace('\\', '/');
                var imp = AssetImporter.GetAtPath(p) as ModelImporter;
                if (imp == null) continue;
                bool changed = imp.animationType != ModelImporterAnimationType.Human;
                imp.animationType = ModelImporterAnimationType.Human;
                imp.importAnimation = true;
                var clips = imp.defaultClipAnimations;
                string n = Path.GetFileNameWithoutExtension(p).ToLower();
                foreach (var c in clips)
                {
                    c.loopTime = !(n.Contains("jump") || n.Contains("mantle") || n.Contains("climb"));
                    c.lockRootHeightY = true; c.lockRootRotation = true; c.keepOriginalPositionXZ = false;
                }
                imp.clipAnimations = clips;
                if (changed || clips.Length > 0) imp.SaveAndReimport();
            }

            var idle = Find("idle", "stand");
            var walk = Find("walk");
            var run = Find("run", "jog");
            var sprint = Find("sprint") ?? run;
            var jump = Find("jump");
            var cIdle = Find("crouch idle", "crouch_idle", "crouchidle") ?? idle;
            var cWalk = Find("crouch walk", "crouch_walk", "crouchwalk", "sneak") ?? walk;
            var mantle = Find("mantle", "climb", "vault") ?? jump;
            Debug.Log("Ertugrul: kliplar — idle:" + N(idle) + " walk:" + N(walk) + " run:" + N(run) + " sprint:" + N(sprint) +
                      " jump:" + N(jump) + " crouchIdle:" + N(cIdle) + " crouchWalk:" + N(cWalk) + " mantle:" + N(mantle));
            if (idle == null || walk == null || run == null)
            {
                Debug.LogWarning("Ertugrul: idle/walk/run kliplari topilmadi — Animator qurilmadi. FBX'larni " + Dir + " ga qo'ying.");
                return;
            }

            var ctrl = AnimatorController.CreateAnimatorControllerAtPath(Ctrl);
            ctrl.AddParameter("Speed", AnimatorControllerParameterType.Float);
            ctrl.AddParameter("Grounded", AnimatorControllerParameterType.Bool);
            ctrl.AddParameter("Crouch", AnimatorControllerParameterType.Bool);
            ctrl.AddParameter("Jump", AnimatorControllerParameterType.Trigger);
            ctrl.AddParameter("Mantle", AnimatorControllerParameterType.Trigger);
            var sm = ctrl.layers[0].stateMachine;

            var loco = sm.AddState("Locomotion");
            var tree = new BlendTree { name = "Loco", blendType = BlendTreeType.Simple1D, blendParameter = "Speed", useAutomaticThresholds = false };
            AssetDatabase.AddObjectToAsset(tree, ctrl);
            tree.AddChild(idle, 0f); tree.AddChild(walk, 1.35f); tree.AddChild(run, 3.3f); tree.AddChild(sprint, 6.2f);
            loco.motion = tree;
            sm.defaultState = loco;

            var crouch = sm.AddState("Crouch");
            var ctree = new BlendTree { name = "CrouchLoco", blendType = BlendTreeType.Simple1D, blendParameter = "Speed", useAutomaticThresholds = false };
            AssetDatabase.AddObjectToAsset(ctree, ctrl);
            ctree.AddChild(cIdle, 0f); ctree.AddChild(cWalk, 1.1f);
            crouch.motion = ctree;
            var toC = loco.AddTransition(crouch); toC.AddCondition(AnimatorConditionMode.If, 0, "Crouch"); toC.hasExitTime = false; toC.duration = 0.15f;
            var fromC = crouch.AddTransition(loco); fromC.AddCondition(AnimatorConditionMode.IfNot, 0, "Crouch"); fromC.hasExitTime = false; fromC.duration = 0.15f;

            if (jump != null)
            {
                var js = sm.AddState("Jump"); js.motion = jump;
                var tj = sm.AddAnyStateTransition(js); tj.AddCondition(AnimatorConditionMode.If, 0, "Jump"); tj.duration = 0.05f; tj.canTransitionToSelf = false;
                var back = js.AddTransition(loco); back.hasExitTime = true; back.exitTime = 0.85f; back.duration = 0.15f;
            }
            if (mantle != null)
            {
                var ms = sm.AddState("Mantle"); ms.motion = mantle;
                var tm = sm.AddAnyStateTransition(ms); tm.AddCondition(AnimatorConditionMode.If, 0, "Mantle"); tm.duration = 0.05f; tm.canTransitionToSelf = false;
                var back2 = ms.AddTransition(loco); back2.hasExitTime = true; back2.exitTime = 0.9f; back2.duration = 0.1f;
            }
            AssetDatabase.SaveAssets();
            Debug.Log("Ertugrul: Animator yaratildi: " + Ctrl);
        }

        static string N(AnimationClip c) => c != null ? c.name : "-";

        // Kalit so'z bo'yicha klip: avval Characters (Mixamo), keyin Starter Assets
        static AnimationClip Find(params string[] keys)
        {
            foreach (var dir in SearchDirs)
            {
                if (!Directory.Exists(dir)) continue;
                var files = new List<string>();
                foreach (var f in Directory.GetFiles(dir, "*.fbx", SearchOption.AllDirectories)) files.Add(f.Replace('\\', '/'));
                foreach (var f in Directory.GetFiles(dir, "*.anim", SearchOption.AllDirectories)) files.Add(f.Replace('\\', '/'));
                files.Sort();
                foreach (var f in files)
                {
                    string n = Path.GetFileNameWithoutExtension(f).ToLower().Replace("--", " ").Replace("_", " ").Replace(".anim", "");
                    bool ok = false;
                    foreach (var k in keys) if (n.Contains(k)) { ok = true; break; }
                    if (!ok) continue;
                    // "walk n land" / "run n land" — qo'nish kliplari emas
                    if (n.Contains("land") && !HasKey(keys, "land")) continue;
                    if (n.Contains("inair") || n.Contains("in air")) { if (!HasKey(keys, "inair")) continue; }
                    if (n.Contains("crouch") && !HasKey(keys, "crouch")) continue;
                    foreach (var o in AssetDatabase.LoadAllAssetsAtPath(f))
                        if (o is AnimationClip c && !c.name.StartsWith("__preview")) return c;
                }
            }
            return null;
        }
        static bool HasKey(string[] keys, string k) { foreach (var x in keys) if (x.Contains(k)) return true; return false; }
    }
}
