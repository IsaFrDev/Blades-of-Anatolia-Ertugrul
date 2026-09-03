#if UNITY_EDITOR
using System;
using System.IO;
using UnityEditor;
using UnityEngine;
using Ertugrul.NPC;
using Ertugrul.Dialogue;
using Ertugrul.Quest;
using Ertugrul.Core;

namespace Ertugrul.EditorTools
{
    /// <summary>
    /// Namuna kontent yaratadi: bitta quest, bitta dialog, bitta yuradigan NPC.
    /// Menyu: Ertugrul → Namuna NPC va missiya yaratish
    ///
    /// Mazmun EP039 «Söğüt» epizodidan olingan (GDD 03B_NARRATIVE_S3_S4.md).
    /// </summary>
    public static class ErtSampleContent
    {
        const string DataFolder = "Assets/Ertugrul/Data";
        const string DlgFolder  = DataFolder + "/Dialogue";
        const string QstFolder  = DataFolder + "/Quests";

        [MenuItem("Ertugrul/Namuna NPC va missiya yaratish", false, 2)]
        public static void CreateSample()
        {
            EnsureFolder(DataFolder);
            EnsureFolder(DlgFolder);
            EnsureFolder(QstFolder);

            var quest    = CreateQuest();
            var dialogue = CreateDialogue(quest);
            var npc      = CreateNpc(dialogue);
            CreateTrigger();

            // Questni boshlanishda beramiz
            var qm = UnityEngine.Object.FindFirstObjectByType<ErtQuestManager>();
            if (qm != null && qm.startingQuest == null)
            {
                qm.startingQuest = quest;
                EditorUtility.SetDirty(qm);
            }

            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
            Selection.activeGameObject = npc;
            EditorGUIUtility.PingObject(npc);

            EditorUtility.DisplayDialog(
                "Namuna yaratildi",
                "✅ Missiya: Söğütga o'rnashish (3 bosqich)\n" +
                "✅ Dialog: keksa yunon (tanlov bilan)\n" +
                "✅ NPC: patrul qiladi va gaplashadi\n" +
                "✅ Trigger: qishloq markaziga kirish\n\n" +
                "MUHIM: NPC hozir (0,0,0) da. Uni Scene oynasida\n" +
                "qishloqqa, YER USTIGA suring — aks holda yurmaydi.\n\n" +
                "Va NavMesh'ni BAKE qilishni unutmang!",
                "Tushundim");
        }

        // ═══════════════════════════════════════════════════════════

        static ErtQuestData CreateQuest()
        {
            string path = $"{QstFolder}/QST_EP039_SOGUT.asset";
            var existing = AssetDatabase.LoadAssetAtPath<ErtQuestData>(path);
            if (existing != null) return existing;

            var q = ScriptableObject.CreateInstance<ErtQuestData>();
            q.questId    = "QST_EP039_SOGUT";
            q.title      = "Söğütga o'rnashish";
            q.episodeId  = "EP039";
            q.description= "Bo'sh qishloqni ko'rib chiq. Kim yashayotganini bil.";
            q.imanReward = 5f;
            q.dirhamReward = 40;
            q.setFlagsOnComplete = new[] { "EP039.Arrived" };
            q.codexUnlocks = new[] { "CDX_SOGUT_HISTORY", "CDX_BYZANTINE_VILLAGERS" };
            q.objectives = new[]
            {
                new ErtObjective {
                    objectiveId = "OBJ_REACH_VILLAGE",
                    text = "Qishloq markaziga yet" },
                new ErtObjective {
                    objectiveId = "OBJ_TALK_TO_ELDER",
                    text = "Keksa yunon bilan gaplash" },
                new ErtObjective {
                    objectiveId = "OBJ_FIND_WELL",
                    text = "Quduqni top", optional = true },
            };

            AssetDatabase.CreateAsset(q, path);
            return q;
        }

        static ErtDialogueData CreateDialogue(ErtQuestData quest)
        {
            string path = $"{DlgFolder}/DLG_GREEK_ELDER.asset";
            var existing = AssetDatabase.LoadAssetAtPath<ErtDialogueData>(path);
            if (existing != null) return existing;

            var d = ScriptableObject.CreateInstance<ErtDialogueData>();
            d.dialogueId = "DLG_GREEK_ELDER";
            d.playOnce = false;
            d.completeObjectiveId = "OBJ_TALK_TO_ELDER";
            d.setFlagsOnComplete = new[] { "EP039.MetElder" };

            d.lines = new[]
            {
                // 0
                new ErtDialogueLine {
                    speaker = "Keksa yunon",
                    text = "Siz kimsiz? Bu yerda nima qilyapsiz?",
                    nextLineIndex = 1 },
                // 1 — tanlov
                new ErtDialogueLine {
                    speaker = "Ertug'rul",
                    text = "...",
                    nextLineIndex = -1,
                    choices = new[]
                    {
                        new ErtDialogueChoice {
                            text = "Biz bu yerda yashamoqchimiz.",
                            gotoLineIndex = 2, setFlag = "EP039.Honest", imanDelta = 3f },
                        new ErtDialogueChoice {
                            text = "Bu yer endi bizniki.",
                            gotoLineIndex = 3, setFlag = "EP039.Harsh", imanDelta = -5f },
                        new ErtDialogueChoice {
                            text = "(Jim turish)",
                            gotoLineIndex = 4 },
                    }},
                // 2 — halol javob
                new ErtDialogueLine {
                    speaker = "Keksa yunon",
                    text = "Bu yerda oldin ham kelganlar. Hammasi ketgan.\n" +
                           "Siz ham ketasizmi?",
                    nextLineIndex = 5 },
                // 3 — qattiq javob
                new ErtDialogueLine {
                    speaker = "Keksa yunon",
                    text = "Demak, yana bir xo'jayin.\n" +
                           "Mayli. Biz xo'jayinlarni sanashni bilamiz.",
                    nextLineIndex = -1 },
                // 4 — jimlik
                new ErtDialogueLine {
                    speaker = "Keksa yunon",
                    text = "Gapirmaysizmi? Yaxshi.\n" +
                           "Ko'p gapiradiganlar tez ketadi.",
                    nextLineIndex = 5 },
                // 5 — yakun
                new ErtDialogueLine {
                    speaker = "Ertug'rul",
                    text = "Biz ketmaymiz.",
                    nextLineIndex = -1 },
            };

            AssetDatabase.CreateAsset(d, path);
            return d;
        }

        static GameObject CreateNpc(ErtDialogueData dialogue)
        {
            var existing = GameObject.Find("NPC_GreekElder");
            if (existing != null) return existing;

            var go = new GameObject("NPC_GreekElder");
            Undo.RegisterCreatedObjectUndo(go, "NPC");
            go.transform.position = new Vector3(3f, 0f, 3f);

            // Vaqtinchalik ko'rinish
            var body = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            body.name = "TEMP_Body";
            body.transform.SetParent(go.transform, false);
            body.transform.localPosition = new Vector3(0f, 0.85f, 0f);
            body.transform.localScale = new Vector3(0.6f, 0.85f, 0.6f);
            UnityEngine.Object.DestroyImmediate(body.GetComponent<Collider>());
            var mr = body.GetComponent<MeshRenderer>();
            if (mr != null && mr.sharedMaterial != null)
            {
                var mat = new Material(mr.sharedMaterial) { color = new Color32(0x48,0xA9,0xB5,255) };
                mr.sharedMaterial = mat;
            }

            // Interaksiya uchun collider
            var col = go.AddComponent<CapsuleCollider>();
            col.height = 1.7f; col.radius = 0.4f;
            col.center = new Vector3(0f, 0.85f, 0f);

            // NavMeshAgent — YURISH SHU YERDAN KELADI
            var agent = go.AddComponent<UnityEngine.AI.NavMeshAgent>();
            agent.speed = 1.4f;
            agent.angularSpeed = 240f;
            agent.acceleration = 6f;
            agent.stoppingDistance = 0.4f;
            agent.radius = 0.35f;
            agent.height = 1.7f;

            var npc = go.AddComponent<ErtNpcController>();
            npc.npcId = "CHR_GREEK_ELDER";
            npc.displayName = "Keksa yunon";
            npc.dialogue = dialogue;
            npc.mode = ErtNpcController.Mode.Wander;
            npc.wanderRadius = 8f;
            npc.walkSpeed = 1.4f;

            return go;
        }

        static void CreateTrigger()
        {
            if (GameObject.Find("Trigger_VillageCenter") != null) return;

            var go = new GameObject("Trigger_VillageCenter");
            Undo.RegisterCreatedObjectUndo(go, "Trigger");
            go.transform.position = new Vector3(0f, 1f, 8f);

            var box = go.AddComponent<BoxCollider>();
            box.isTrigger = true;
            box.size = new Vector3(8f, 4f, 8f);

            var t = go.AddComponent<ErtObjectiveTrigger>();
            t.objectiveId = "OBJ_REACH_VILLAGE";
            t.setFlag = "EP039.EnteredVillage";
        }

        static void EnsureFolder(string path)
        {
            if (AssetDatabase.IsValidFolder(path)) return;
            string parent = Path.GetDirectoryName(path)!.Replace('\\', '/');
            string leaf = Path.GetFileName(path);
            EnsureFolder(parent);
            AssetDatabase.CreateFolder(parent, leaf);
        }

        // ═══════════════════════════════════════════════════════════

        [MenuItem("Ertugrul/Tekshirish: nima yetishmayapti?", false, 20)]
        public static void Diagnose()
        {
            var sb = new System.Text.StringBuilder();
            void Check(bool ok, string label, string fix)
                => sb.AppendLine(ok ? $"✅  {label}" : $"❌  {label}\n     → {fix}");

            Check(UnityEngine.Object.FindFirstObjectByType<ErtGameManager>() != null,
                "GameManager", "Ertugrul > Sahnani sozlash");
            Check(GameObject.FindGameObjectWithTag("Player") != null,
                "Player (tag 'Player')", "Ertugrul > Sahnani sozlash");
            Check(UnityEngine.Object.FindFirstObjectByType<ErtDialogueRunner>() != null,
                "DialogueRunner", "Ertugrul > Sahnani sozlash");
            Check(UnityEngine.Object.FindFirstObjectByType<ErtDialogueUI>(FindObjectsInactive.Include) != null,
                "Dialog UI", "Ertugrul > Sahnani sozlash");

            bool navPkg = Type.GetType("Unity.AI.Navigation.NavMeshSurface, Unity.AI.Navigation") != null;
            Check(navPkg, "AI Navigation paketi",
                "Window > Package Manager > Unity Registry > AI Navigation > Install");

            var tri = UnityEngine.AI.NavMesh.CalculateTriangulation();
            Check(tri.vertices != null && tri.vertices.Length > 0,
                $"NavMesh baked ({(tri.vertices?.Length ?? 0)} vertex)",
                "Level obyektiga NavMeshSurface qo'shing va BAKE bosing");

            int agents = UnityEngine.Object.FindObjectsByType<UnityEngine.AI.NavMeshAgent>(
                FindObjectsSortMode.None).Length;
            Check(agents > 0, $"NavMeshAgent ({agents} ta)",
                "Ertugrul > Namuna NPC va missiya yaratish");

            Debug.Log("═══ ERTUGRUL DIAGNOSTIKA ═══\n" + sb);
            EditorUtility.DisplayDialog("Diagnostika", sb.ToString(), "Yopish");
        }
    }
}
#endif
