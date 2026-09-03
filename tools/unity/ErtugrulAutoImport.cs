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
            try { File.Delete(Flag); } catch { }
            Debug.Log("Ertugrul: AUTOIMPORT bayrog'i topildi — barcha darajalar import qilinmoqda");
            ErtugrulLevelImporter.ImportAll();
        }
    }
}
