// Lokalizatsiya: C++ dvijok bilan bir xil CSV'lar ("keys","uz","tr","en").
// Fayllar Assets/Ertugrul/Localization/*.csv (Editor/Play rejimi) yoki
// StreamingAssets/Ertugrul/Localization (build) dan o'qiladi.
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnityEngine;

namespace Ertugrul
{
    public static class ErtugrulLoc
    {
        public static string Lang = "uz";
        static readonly Dictionary<string, string[]> table = new Dictionary<string, string[]>();
        static bool loaded;

        public static string Root
        {
            get
            {
                string a = Path.Combine(Application.dataPath, "Ertugrul");
                if (Directory.Exists(a)) return a;
                return Path.Combine(Application.streamingAssetsPath, "Ertugrul");
            }
        }

        public static void Load()
        {
            if (loaded) return;
            loaded = true;
            string dir = Path.Combine(Root, "Localization");
            if (!Directory.Exists(dir)) { Debug.LogWarning("Ertugrul: Localization papkasi yo'q: " + dir); return; }
            foreach (var f in Directory.GetFiles(dir, "*.csv"))
                foreach (var row in ParseCsv(File.ReadAllText(f, Encoding.UTF8)))
                    if (row.Count >= 4 && row[0] != "keys" && !table.ContainsKey(row[0]))
                        table[row[0]] = new[] { row[1], row[2], row[3] };
            Debug.Log("Ertugrul: lokalizatsiya " + table.Count + " kalit");
        }

        public static string T(string key)
        {
            Load();
            if (string.IsNullOrEmpty(key) || !table.TryGetValue(key, out var v)) return key;
            int i = Lang == "tr" ? 1 : (Lang == "en" ? 2 : 0);
            return string.IsNullOrEmpty(v[i]) ? v[0] : v[i];
        }

        // Oddiy CSV: qo'shtirnoq ichida vergul va "" qochish
        static List<List<string>> ParseCsv(string s)
        {
            var rows = new List<List<string>>();
            var row = new List<string>();
            var cell = new StringBuilder();
            bool q = false;
            for (int i = 0; i < s.Length; ++i)
            {
                char c = s[i];
                if (q)
                {
                    if (c == '"') { if (i + 1 < s.Length && s[i + 1] == '"') { cell.Append('"'); ++i; } else q = false; }
                    else cell.Append(c);
                }
                else if (c == '"') q = true;
                else if (c == ',') { row.Add(cell.ToString()); cell.Clear(); }
                else if (c == '\n' || c == '\r')
                {
                    if (cell.Length > 0 || row.Count > 0) { row.Add(cell.ToString()); rows.Add(row); row = new List<string>(); cell.Clear(); }
                }
                else cell.Append(c);
            }
            if (cell.Length > 0 || row.Count > 0) { row.Add(cell.ToString()); rows.Add(row); }
            return rows;
        }
    }
}
