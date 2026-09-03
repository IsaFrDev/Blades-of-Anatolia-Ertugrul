// Mixamo rig bilan ishlash.
//
// Mixamo (mixamo.com) ga kirish Adobe hisobi talab qiladi, shuning uchun yuklab olish
// qo'lda: 1) assets/models/ottoman/ottoman.obj ni Mixamo ga yuklang ("Upload character"),
// autorig; 2) quyidagi animatsiyalarni "FBX for Unity, Without Skin" (birinchisini
// "With Skin") formatida yuklab oling va Assets/Ertugrul/Characters/ papkasiga qo'ying:
//      ertugrul.fbx        — Idle (With Skin)  -> model + skelet
//      anim_idle.fbx, anim_walk.fbx, anim_run.fbx, anim_sprint.fbx,
//      anim_jump.fbx, anim_crouch_idle.fbx, anim_crouch_walk.fbx, anim_mantle.fbx (Climb)
// 3) Menyu: Ertugrul > Build Animator from Mixamo. Keyin "Setup Gameplay" avtomatik
//    rigli modelni ishlatadi (Speed/Grounded/Crouch/Jump/Mantle parametrlari).
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

        [MenuItem("Ertugrul/Build Animator from Mixamo")]
        public static void Build()
        {
            Directory.CreateDirectory(Dir);
            var files = Directory.GetFiles(Dir, "*.fbx");
            if (files.Length == 0)
            {
                Debug.LogWarning("Ertugrul: " + Dir + " da FBX yo'q. Mixamo dan yuklab oling (skript boshidagi izoh).");
                return;
            }
            // Barcha FBX: Humanoid rig, loop
            foreach (var f in files)
            {
                var imp = AssetImporter.GetAtPath(f.Replace('\\', '/')) as ModelImporter;
                if (imp == null) continue;
                imp.animationType = ModelImporterAnimationType.Human;
                imp.importAnimation = true;
                var clips = imp.defaultClipAnimations;
                foreach (var c in clips)
                {
                    string n = Path.GetFileNameWithoutExtension(f).ToLower();
                    c.loopTime = !(n.Contains("jump") || n.Contains("mantle") || n.Contains("climb"));
                    c.lockRootHeightY = true; c.lockRootRotation = true; c.keepOriginalPositionXZ = false;
                }
                imp.clipAnimations = clips;
                imp.SaveAndReimport();
            }

            var ctrl = AnimatorController.CreateAnimatorControllerAtPath(Ctrl);
            ctrl.AddParameter("Speed", AnimatorControllerParameterType.Float);
            ctrl.AddParameter("Grounded", AnimatorControllerParameterType.Bool);
            ctrl.AddParameter("Crouch", AnimatorControllerParameterType.Bool);
            ctrl.AddParameter("Jump", AnimatorControllerParameterType.Trigger);
            ctrl.AddParameter("Mantle", AnimatorControllerParameterType.Trigger);
            var sm = ctrl.layers[0].stateMachine;

            // Locomotion blend tree: Speed bo'yicha idle -> walk -> run -> sprint
            var loco = sm.AddState("Locomotion");
            var tree = new BlendTree { name = "Loco", blendType = BlendTreeType.Simple1D, blendParameter = "Speed", useAutomaticThresholds = false };
            AssetDatabase.AddObjectToAsset(tree, ctrl);
            AddMotion(tree, "anim_idle", 0f); AddMotion(tree, "anim_walk", 1.35f);
            AddMotion(tree, "anim_run", 3.3f); AddMotion(tree, "anim_sprint", 6.2f);
            loco.motion = tree;
            sm.defaultState = loco;

            var crouchState = sm.AddState("Crouch");
            var ctree = new BlendTree { name = "CrouchLoco", blendType = BlendTreeType.Simple1D, blendParameter = "Speed", useAutomaticThresholds = false };
            AssetDatabase.AddObjectToAsset(ctree, ctrl);
            AddMotion(ctree, "anim_crouch_idle", 0f); AddMotion(ctree, "anim_crouch_walk", 1.1f);
            crouchState.motion = ctree;
            var toCrouch = loco.AddTransition(crouchState); toCrouch.AddCondition(AnimatorConditionMode.If, 0, "Crouch"); toCrouch.hasExitTime = false; toCrouch.duration = 0.15f;
            var fromCrouch = crouchState.AddTransition(loco); fromCrouch.AddCondition(AnimatorConditionMode.IfNot, 0, "Crouch"); fromCrouch.hasExitTime = false; fromCrouch.duration = 0.15f;

            var jump = sm.AddState("Jump"); jump.motion = FindClip("anim_jump");
            var tj = sm.AddAnyStateTransition(jump); tj.AddCondition(AnimatorConditionMode.If, 0, "Jump"); tj.duration = 0.05f; tj.canTransitionToSelf = false;
            var back = jump.AddTransition(loco); back.hasExitTime = true; back.exitTime = 0.85f; back.duration = 0.15f;

            var mantle = sm.AddState("Mantle"); mantle.motion = FindClip("anim_mantle");
            var tm = sm.AddAnyStateTransition(mantle); tm.AddCondition(AnimatorConditionMode.If, 0, "Mantle"); tm.duration = 0.05f; tm.canTransitionToSelf = false;
            var back2 = mantle.AddTransition(loco); back2.hasExitTime = true; back2.exitTime = 0.9f; back2.duration = 0.1f;

            AssetDatabase.SaveAssets();
            Debug.Log("Ertugrul: Animator yaratildi: " + Ctrl);
        }

        static void AddMotion(BlendTree t, string name, float threshold)
        {
            var c = FindClip(name);
            if (c != null) t.AddChild(c, threshold);
            else Debug.LogWarning("Ertugrul: klip topilmadi: " + name + ".fbx");
        }

        static AnimationClip FindClip(string name)
        {
            string p = Dir + "/" + name + ".fbx";
            foreach (var o in AssetDatabase.LoadAllAssetsAtPath(p))
                if (o is AnimationClip c && !c.name.StartsWith("__preview")) return c;
            return null;
        }
    }
}
