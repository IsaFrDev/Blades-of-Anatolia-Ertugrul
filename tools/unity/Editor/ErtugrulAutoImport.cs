// Avtomatik import: Assets/Ertugrul/Levels/AUTOIMPORT fayli bo'lsa, muharrir
// skriptlarni qayta yuklagan zahoti ImportAll() ishga tushadi va fayl o'chiriladi.
// Tashqaridan (CLI siz) importni boshlash uchun: faylni yarating, Unity oynasiga
// bir marta bosing (yoki Ctrl+R).
using System.IO;
using UnityEditor;
using UnityEngine;

namespace Ertugrul.EditorTools
{
    [InitializeOnLoad]
    public static class ErtugrulAutoImport
    {
        const string Flag = "Assets/Ertugrul/Levels/AUTOIMPORT";

        static ErtugrulAutoImport()
        {
            if (!File.Exists(Flag)) return;
            // Import tugaguncha kutamiz — assetlar hali yuklanayotgan bo'lishi mumkin
            EditorApplication.delayCall += () =>
            {
                if (!File.Exists(Flag)) return;
                if (EditorApplication.isCompiling || EditorApplication.isUpdating)
                {
                    EditorApplication.delayCall += Retry;
                    return;
                }
                Run();
            };
        }

        static void Retry()
        {
            if (!File.Exists(Flag)) return;
            if (EditorApplication.isCompiling || EditorApplication.isUpdating) { EditorApplication.delayCall += Retry; return; }
            Run();
        }

        static void Run()
        {
            // Fayl ichida "Namespace.Type.Method" bo'lsa — o'sha statik metod, bo'sh bo'lsa ImportAll
            string method = "";
            try { method = File.ReadAllText(Flag).Trim(); File.Delete(Flag); } catch { }
            if (string.IsNullOrEmpty(method))
            {
                Debug.Log("Ertugrul: AUTOIMPORT bayrog'i topildi — barcha darajalar import qilinmoqda");
                ErtugrulLevelImporter.ImportAll();
                return;
            }
            int dot = method.LastIndexOf('.');
            var type = System.Type.GetType(method.Substring(0, dot) + ", Assembly-CSharp-Editor");
            var mi = type != null ? type.GetMethod(method.Substring(dot + 1)) : null;
            if (mi == null) { Debug.LogError("Ertugrul: AUTOIMPORT metodi topilmadi: " + method); return; }
            Debug.Log("Ertugrul: AUTOIMPORT -> " + method);
            mi.Invoke(null, null);
        }
    }
}
