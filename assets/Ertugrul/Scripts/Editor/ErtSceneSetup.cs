#if UNITY_EDITOR
using System;
using UnityEditor;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem.UI;
using TMPro;
using Ertugrul.Core;
using Ertugrul.Player;
using Ertugrul.Dialogue;
using Ertugrul.Quest;

namespace Ertugrul.EditorTools
{
    /// <summary>
    /// BITTA TUGMA BILAN SAHNANI SOZLASH.
    ///
    /// Menyu: Ertugrul → Sahnani sozlash (Setup Scene)
    ///
    /// Yaratadigan narsalar:
    ///   • _Ertugrul_Systems — GameManager, QuestManager, DialogueRunner
    ///   • Player — CharacterController + harakat + interaksiya
    ///   • Kamera — uchinchi shaxs orbital
    ///   • Canvas — dialog oynasi, interaksiya prompti, missiya HUD'i, sana kartasi
    ///   • EventSystem
    ///   • NavMeshSurface (agar AI Navigation paketi o'rnatilgan bo'lsa)
    ///
    /// Xavfsiz: allaqachon mavjud obyektlarni qayta yaratmaydi.
    /// </summary>
    public static class ErtSceneSetup
    {
        // GDD «Temir va Firuze» palitrasi
        static readonly Color Ground   = new Color32(0x0E, 0x13, 0x16, 235);
        static readonly Color Panel    = new Color32(0x16, 0x1D, 0x21, 245);
        static readonly Color Line     = new Color32(0x2A, 0x35, 0x3A, 255);
        static readonly Color Ink      = new Color32(0xE4, 0xEA, 0xEA, 255);
        static readonly Color Dim      = new Color32(0x7C, 0x8B, 0x8F, 255);
        static readonly Color Accent   = new Color32(0x48, 0xA9, 0xB5, 255);   // feruza
        static readonly Color Gold     = new Color32(0xC0, 0x95, 0x50, 255);   // zarhal

        [MenuItem("Ertugrul/Sahnani sozlash (Setup Scene)", false, 1)]
        public static void SetupScene()
        {
            Undo.SetCurrentGroupName("Ertugrul: Sahnani sozlash");
            int group = Undo.GetCurrentGroup();

            var systems = CreateSystems();
            var player  = CreatePlayer();
            var cam     = SetupCamera(player.transform);
            CreateUI(player);
            EnsureEventSystem();
            TryAddNavMeshSurface();

            Undo.CollapseUndoOperations(group);
            Selection.activeGameObject = player;

            EditorUtility.DisplayDialog(
                "Ertugrul — sahna sozlandi",
                "Yaratildi:\n" +
                "  • _Ertugrul_Systems\n" +
                "  • Player (CharacterController)\n" +
                "  • Kamera (uchinchi shaxs)\n" +
                "  • Canvas (dialog, prompt, missiya HUD)\n\n" +
                "KEYINGI QADAM:\n" +
                "  1. Player'ni yerga qo'ying (Scene oynasida)\n" +
                "  2. Level_sogut_village'ga NavMeshSurface qo'shib BAKE qiling\n" +
                "  3. Ertugrul → Namuna NPC yaratish\n" +
                "  4. Play bosing",
                "Tushundim");

            Debug.Log("[Ertugrul] Sahna sozlandi ✅");
        }

        // ═══════════════════════════════════════════════════════════

        static GameObject CreateSystems()
        {
            var existing = GameObject.Find("_Ertugrul_Systems");
            if (existing != null) return existing;

            var go = new GameObject("_Ertugrul_Systems");
            Undo.RegisterCreatedObjectUndo(go, "Systems");

            var gm = go.AddComponent<ErtGameManager>();
            var qm = go.AddComponent<ErtQuestManager>();
            go.AddComponent<ErtDialogueRunner>();
            go.AddComponent<AudioSource>();

            gm.questManager = qm;
            gm.currentEpisodeId = "EP039";
            gm.hijriDate = "642 Ramazon";
            gm.gregorianDate = "1245 Fevral";
            gm.placeName = "SÖĞÜT";
            return go;
        }

        static GameObject CreatePlayer()
        {
            var existing = GameObject.FindGameObjectWithTag("Player");
            if (existing != null) return existing;

            var go = new GameObject("Player");
            Undo.RegisterCreatedObjectUndo(go, "Player");
            go.tag = "Player";
            go.transform.position = new Vector3(0f, 1f, 0f);

            var cc = go.AddComponent<CharacterController>();
            cc.height = 1.85f;                     // GDD: Ertug'rul bo'yi 1.85m
            cc.radius = 0.32f;
            cc.center = new Vector3(0f, 0.925f, 0f);
            cc.slopeLimit = 50f;
            cc.stepOffset = 0.4f;

            go.AddComponent<ErtPlayerController>();
            go.AddComponent<ErtPlayerInteraction>();

            // Vaqtinchalik ko'rinish — keyin haqiqiy model bilan almashtiring
            var body = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            body.name = "TEMP_Body (modelingiz bilan almashtiring)";
            body.transform.SetParent(go.transform, false);
            body.transform.localPosition = new Vector3(0f, 0.925f, 0f);
            body.transform.localScale = new Vector3(0.64f, 0.925f, 0.64f);
            UnityEngine.Object.DestroyImmediate(body.GetComponent<Collider>());

            // Old tomonni ko'rsatuvchi belgi
            var nose = GameObject.CreatePrimitive(PrimitiveType.Cube);
            nose.name = "TEMP_Forward";
            nose.transform.SetParent(go.transform, false);
            nose.transform.localPosition = new Vector3(0f, 1.5f, 0.34f);
            nose.transform.localScale = Vector3.one * 0.14f;
            UnityEngine.Object.DestroyImmediate(nose.GetComponent<Collider>());

            return go;
        }

        static Camera SetupCamera(Transform target)
        {
            var cam = Camera.main;
            if (cam == null)
            {
                var go = new GameObject("Main Camera");
                Undo.RegisterCreatedObjectUndo(go, "Camera");
                go.tag = "MainCamera";
                cam = go.AddComponent<Camera>();
                go.AddComponent<AudioListener>();
            }

            var follow = cam.GetComponent<ErtThirdPersonCamera>();
            if (follow == null)
                follow = Undo.AddComponent<ErtThirdPersonCamera>(cam.gameObject);
            follow.target = target;
            return cam;
        }

        // ═══════════════════════════════════════════════════════════
        //  UI
        // ═══════════════════════════════════════════════════════════

        static void CreateUI(GameObject player)
        {
            if (GameObject.Find("ErtugrulCanvas") != null) return;

            var canvasGo = new GameObject("ErtugrulCanvas",
                typeof(Canvas), typeof(CanvasScaler), typeof(GraphicRaycaster));
            Undo.RegisterCreatedObjectUndo(canvasGo, "Canvas");

            var canvas = canvasGo.GetComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;

            var scaler = canvasGo.GetComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1920f, 1080f);
            scaler.matchWidthOrHeight = 0.5f;

            BuildDialoguePanel(canvasGo.transform);
            BuildInteractPrompt(canvasGo.transform, player);
            BuildQuestHud(canvasGo.transform);
        }

        static void BuildDialoguePanel(Transform parent)
        {
            // Panel — ekranning pastki qismida
            var panel = NewUI("DialoguePanel", parent);
            var rt = panel.GetComponent<RectTransform>();
            rt.anchorMin = new Vector2(0.5f, 0f);
            rt.anchorMax = new Vector2(0.5f, 0f);
            rt.pivot = new Vector2(0.5f, 0f);
            rt.anchoredPosition = new Vector2(0f, 48f);
            rt.sizeDelta = new Vector2(1180f, 260f);
            AddImage(panel, Panel);

            // Chap qirra — feruza (GDD uslubi)
            var edge = NewUI("Edge", panel.transform);
            var ert = edge.GetComponent<RectTransform>();
            ert.anchorMin = new Vector2(0f, 0f); ert.anchorMax = new Vector2(0f, 1f);
            ert.pivot = new Vector2(0f, 0.5f);
            ert.sizeDelta = new Vector2(2f, 0f); ert.anchoredPosition = Vector2.zero;
            AddImage(edge, Accent);

            var speaker = AddText(panel.transform, "Speaker", "TURGUT", 15, Accent,
                new Vector2(0f, 1f), new Vector2(1f, 1f), new Vector2(0f, 1f),
                new Vector2(28f, -20f), new Vector2(-56f, 24f));
            speaker.fontStyle = FontStyles.UpperCase;
            speaker.characterSpacing = 8f;

            var body = AddText(parent: panel.transform, name: "Body",
                text: "...", size: 22, color: Ink,
                aMin: new Vector2(0f, 0f), aMax: new Vector2(1f, 1f), pivot: new Vector2(0f, 1f),
                pos: new Vector2(28f, -54f), size2: new Vector2(-56f, -96f));
            body.alignment = TextAlignmentOptions.TopLeft;

            var hint = AddText(panel.transform, "ContinueHint", "[E]  davom etish", 13, Dim,
                new Vector2(1f, 0f), new Vector2(1f, 0f), new Vector2(1f, 0f),
                new Vector2(-28f, 18f), new Vector2(260f, 22f));
            hint.alignment = TextAlignmentOptions.BottomRight;

            // Tanlovlar konteyneri
            var choices = NewUI("Choices", panel.transform);
            var crt = choices.GetComponent<RectTransform>();
            crt.anchorMin = new Vector2(0f, 0f); crt.anchorMax = new Vector2(1f, 0f);
            crt.pivot = new Vector2(0.5f, 0f);
            crt.anchoredPosition = new Vector2(0f, 16f);
            crt.sizeDelta = new Vector2(-56f, 120f);
            var vlg = choices.AddComponent<VerticalLayoutGroup>();
            vlg.spacing = 6f; vlg.childForceExpandHeight = false;
            vlg.childControlHeight = true; vlg.childControlWidth = true;

            // Tanlov tugmasi shabloni
            var btnGo = NewUI("ChoiceButtonPrefab", choices.transform);
            var brt = btnGo.GetComponent<RectTransform>();
            brt.sizeDelta = new Vector2(0f, 34f);
            var img = AddImage(btnGo, new Color32(0x1E, 0x27, 0x2B, 255));
            var btn = btnGo.AddComponent<Button>();
            btn.targetGraphic = img;
            var colors = btn.colors;
            colors.highlightedColor = new Color32(0x48, 0xA9, 0xB5, 60);
            colors.selectedColor = new Color32(0x48, 0xA9, 0xB5, 60);
            btn.colors = colors;

            var lbl = AddText(btnGo.transform, "Label", "1. Variant", 16, Ink,
                Vector2.zero, Vector2.one, new Vector2(0.5f, 0.5f),
                Vector2.zero, Vector2.zero);
            lbl.alignment = TextAlignmentOptions.Left;
            lbl.margin = new Vector4(14f, 0f, 8f, 0f);
            btnGo.SetActive(false);

            // Skriptni bog'laymiz
            var ui = panel.AddComponent<ErtDialogueUI>();
            ui.root = panel;
            ui.speakerText = speaker;
            ui.bodyText = body;
            ui.continueHint = hint.gameObject;
            ui.choiceContainer = crt;
            ui.choiceButtonPrefab = btn;

            var runner = UnityEngine.Object.FindFirstObjectByType<ErtDialogueRunner>();
            if (runner != null)
            {
                runner.ui = ui;
                runner.voiceSource = runner.GetComponent<AudioSource>();
            }
            panel.SetActive(false);
        }

        static void BuildInteractPrompt(Transform parent, GameObject player)
        {
            var go = NewUI("InteractPrompt", parent);
            var rt = go.GetComponent<RectTransform>();
            rt.sizeDelta = new Vector2(340f, 32f);
            AddImage(go, new Color32(0x0E, 0x13, 0x16, 200));

            var label = AddText(go.transform, "Label", "[E]  gaplashish", 15, Ink,
                Vector2.zero, Vector2.one, new Vector2(0.5f, 0.5f), Vector2.zero, Vector2.zero);
            label.alignment = TextAlignmentOptions.Center;

            var prompt = go.AddComponent<ErtInteractPromptUI>();
            prompt.panel = rt;
            prompt.label = label;

            var interaction = player.GetComponent<ErtPlayerInteraction>();
            if (interaction != null) interaction.promptUI = prompt;

            go.SetActive(false);
        }

        static void BuildQuestHud(Transform parent)
        {
            var hud = NewUI("QuestHUD", parent);
            var hrt = hud.GetComponent<RectTransform>();
            hrt.anchorMin = Vector2.zero; hrt.anchorMax = Vector2.one;
            hrt.offsetMin = Vector2.zero; hrt.offsetMax = Vector2.zero;

            // ── Missiya — yuqori chap ──────────────────────────────
            var title = AddText(hud.transform, "QuestTitle", "SÖĞÜTGA O'RNASHISH", 14, Accent,
                new Vector2(0f, 1f), new Vector2(0f, 1f), new Vector2(0f, 1f),
                new Vector2(36f, -36f), new Vector2(520f, 22f));
            title.characterSpacing = 8f;

            var objectives = AddText(hud.transform, "QuestObjectives", "", 16, Ink,
                new Vector2(0f, 1f), new Vector2(0f, 1f), new Vector2(0f, 1f),
                new Vector2(36f, -62f), new Vector2(520f, 140f));
            objectives.alignment = TextAlignmentOptions.TopLeft;

            // ── Sana kartasi — pastki chap (GDD qoidasi) ───────────
            var hijri = AddText(hud.transform, "HijriDate", "٦٤٢ رمضان", 15, Gold,
                new Vector2(0f, 0f), new Vector2(0f, 0f), new Vector2(0f, 0f),
                new Vector2(36f, 96f), new Vector2(420f, 22f));

            var greg = AddText(hud.transform, "GregorianDate", "1245 FEVRAL", 14, Gold,
                new Vector2(0f, 0f), new Vector2(0f, 0f), new Vector2(0f, 0f),
                new Vector2(36f, 72f), new Vector2(420f, 20f));
            greg.characterSpacing = 6f;

            var place = AddText(hud.transform, "PlaceName", "SÖĞÜT", 20, Ink,
                new Vector2(0f, 0f), new Vector2(0f, 0f), new Vector2(0f, 0f),
                new Vector2(36f, 44f), new Vector2(420f, 26f));

            var ui = hud.AddComponent<ErtQuestUI>();
            ui.questTitle = title;
            ui.questObjectives = objectives;
            ui.hijriText = hijri;
            ui.gregorianText = greg;
            ui.placeText = place;
        }

        // ═══════════════════════════════════════════════════════════
        //  Yordamchilar
        // ═══════════════════════════════════════════════════════════

        static GameObject NewUI(string name, Transform parent)
        {
            var go = new GameObject(name, typeof(RectTransform));
            go.transform.SetParent(parent, false);
            return go;
        }

        static Image AddImage(GameObject go, Color c)
        {
            var img = go.AddComponent<Image>();
            img.color = c;
            img.raycastTarget = false;
            return img;
        }

        static TextMeshProUGUI AddText(Transform parent, string name, string text,
            float size, Color color, Vector2 aMin, Vector2 aMax, Vector2 pivot,
            Vector2 pos, Vector2 size2)
        {
            var go = NewUI(name, parent);
            var rt = go.GetComponent<RectTransform>();
            rt.anchorMin = aMin; rt.anchorMax = aMax; rt.pivot = pivot;
            rt.anchoredPosition = pos; rt.sizeDelta = size2;

            var t = go.AddComponent<TextMeshProUGUI>();
            t.text = text;
            t.fontSize = size;
            t.color = color;
            t.raycastTarget = false;
            return t;
        }

        static void EnsureEventSystem()
        {
            if (UnityEngine.Object.FindFirstObjectByType<EventSystem>() != null) return;
            var go = new GameObject("EventSystem", typeof(EventSystem),
                                    typeof(InputSystemUIInputModule));
            Undo.RegisterCreatedObjectUndo(go, "EventSystem");
        }

        /// <summary>
        /// AI Navigation paketi o'rnatilgan bo'lsa NavMeshSurface qo'shadi.
        /// Refleksiya orqali — paket yo'q bo'lsa ham skript kompilyatsiya bo'ladi.
        /// </summary>
        static void TryAddNavMeshSurface()
        {
            Type surfaceType = Type.GetType(
                "Unity.AI.Navigation.NavMeshSurface, Unity.AI.Navigation");

            if (surfaceType == null)
            {
                Debug.LogWarning(
                    "[Ertugrul] AI Navigation paketi topilmadi.\n" +
                    "Window > Package Manager > Unity Registry > 'AI Navigation' → Install\n" +
                    "Keyin Ertugrul > Sahnani sozlash ni qayta ishga tushiring.");
                return;
            }

            var levelRoot = GameObject.Find("Level_sogut_village")
                         ?? GameObject.Find("Level")
                         ?? GameObject.Find("Environment");

            if (levelRoot == null)
            {
                Debug.LogWarning(
                    "[Ertugrul] Level obyekti topilmadi. NavMeshSurface'ni qo'lda qo'shing: " +
                    "Level_sogut_village → Add Component → NavMesh Surface → Bake");
                return;
            }

            if (levelRoot.GetComponent(surfaceType) != null) return;

            Undo.AddComponent(levelRoot, surfaceType);
            Debug.Log($"[Ertugrul] NavMeshSurface qo'shildi: {levelRoot.name}. " +
                      $"Endi Inspector'da BAKE tugmasini bosing.");
        }
    }
}
#endif
