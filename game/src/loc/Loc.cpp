// ertugrul/loc/Loc.h amalga oshirilishi.
// UTF-8 lokalizatsiya jadvali: keys,uz,tr,en ustunli CSV fayllar.
// Butun holat shu .cpp ichida (header'da a'zo maydonlar yo'q).
#include "ertugrul/loc/Loc.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace ert {

namespace {

// Til slotlari: 0 = uz, 1 = tr, 2 = en
enum : int { kUz = 0, kTr = 1, kEn = 2, kLangCount = 3 };

struct Entry {
    std::string v[kLangCount];
};

struct LocState {
    std::map<std::string, Entry> table;
    std::string                  lang     = "uz";
    int                          langSlot = kUz;

    // Topilmagan kalitlar (takrorlanmagan holda, so'ralgan tartibda)
    std::vector<std::string> missing;
    std::set<std::string>    missingSet;

    // tr() 'const std::string&' qaytaradi — shuning uchun topilmagan kalitlar
    // shu keshda DOIMIY saqlanadi (std::map tugunlari ko'chmaydi -> havola xavfsiz).
    std::map<std::string, std::string> missCache;

    const std::string emptyStr;
};

// Yagona global holat (funksiya-lokal static -> initsializatsiya tartibi xavfsiz)
LocState& S() {
    static LocState s;
    return s;
}

// ---------- kichik yordamchi funksiyalar ----------

bool isSpaceCh(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

std::string trimCopy(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isSpaceCh(s[a])) ++a;
    while (b > a && isSpaceCh(s[b - 1])) --b;
    return s.substr(a, b - a);
}

std::string lowerAscii(const std::string& s) {
    std::string r = s;
    for (char& c : r) {
        unsigned char u = static_cast<unsigned char>(c);
        if (u < 128) c = static_cast<char>(std::tolower(u));
    }
    return r;
}

// "\n" ketma-ketligini haqiqiy yangi qatorga, "\\" ni bitta '\' ga aylantiradi.
// (Ko'p qatorli UI matnlarini bitta CSV yacheykasida yozish uchun qulay.)
std::string unescapeText(const std::string& s) {
    if (s.find('\\') == std::string::npos) return s;
    std::string r;
    r.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[i + 1];
            if (n == 'n')       { r.push_back('\n'); ++i; continue; }
            if (n == 't')       { r.push_back('\t'); ++i; continue; }
            if (n == '\\')      { r.push_back('\\'); ++i; continue; }
        }
        r.push_back(s[i]);
    }
    return r;
}

// Faylni to'liq o'qiydi (binar rejim: CRLF buzilmasin). Muvaffaqiyatsiz bo'lsa false.
bool readWholeFile(const std::string& path, std::string& out) {
    std::ifstream f(path.c_str(), std::ios::binary);
    if (!f.is_open()) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    if (f.bad()) return false;
    out = ss.str();
    return true;
}

// Yo'l topilmasa bir necha zaxira prefiks bilan urinadi (build/ ichidan ishga tushirilganda).
bool readFileWithFallback(const std::string& path, std::string& out, std::string& usedPath) {
    if (path.empty()) return false;
    static const char* kPrefix[] = { "", "../", "../../", "../../../" };
    for (const char* p : kPrefix) {
        std::string full = std::string(p) + path;
        if (readWholeFile(full, out)) { usedPath = full; return true; }
    }
    return false;
}

// UTF-8 BOM (EF BB BF) ni olib tashlaydi.
void stripBom(std::string& s) {
    if (s.size() >= 3 &&
        static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
}

// ---------- to'liq CSV tahlilchisi (RFC 4180 uslubida) ----------
// Qo'llab-quvvatlanadi: qo'shtirnoq ichidagi vergul, ikkilangan qo'shtirnoq (""),
// qo'shtirnoq ichidagi yangi qator, CRLF va LF.
std::vector<std::vector<std::string>> parseCsv(const std::string& text) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string>              row;
    std::string                           field;
    bool inQuotes    = false;
    bool quotedField = false;   // maydon qo'shtirnoq bilan boshlangan

    auto endField = [&]() {
        row.push_back(field);
        field.clear();
        quotedField = false;
    };
    auto endRow = [&]() {
        endField();
        rows.push_back(row);
        row.clear();
    };

    const size_t n = text.size();
    size_t i = 0;
    while (i < n) {
        const char c = text[i];

        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < n && text[i + 1] == '"') { field.push_back('"'); i += 2; continue; }
                inQuotes = false; ++i; continue;
            }
            if (c == '\r') {                       // qo'shtirnoq ichidagi CRLF -> LF
                if (i + 1 < n && text[i + 1] == '\n') ++i;
                field.push_back('\n'); ++i; continue;
            }
            field.push_back(c); ++i; continue;
        }

        if (c == '"') {
            if (field.empty() && !quotedField) {   // maydon boshidagi ochuvchi qo'shtirnoq
                inQuotes = true; quotedField = true; ++i; continue;
            }
            if (i + 1 < n && text[i + 1] == '"') { field.push_back('"'); i += 2; continue; }
            inQuotes = true; ++i; continue;        // bag'rikenglik: noto'g'ri joylashgan qo'shtirnoq
        }
        if (c == ',')  { endField(); ++i; continue; }
        if (c == '\r') { if (i + 1 < n && text[i + 1] == '\n') ++i; endRow(); ++i; continue; }
        if (c == '\n') { endRow(); ++i; continue; }

        field.push_back(c);
        ++i;
    }
    if (!field.empty() || !row.empty()) {
        endField();
        rows.push_back(row);
    }
    return rows;
}

// Sarlavha nomidan til slotini aniqlaydi ("uz", "uz-latn", "UZ " ham bo'ladi).
int langSlotFromHeader(const std::string& rawName) {
    const std::string h = lowerAscii(trimCopy(rawName));
    if (h.empty()) return -1;
    struct { const char* p; int slot; } kMap[] = {
        { "uz", kUz }, { "tr", kTr }, { "en", kEn }
    };
    for (const auto& m : kMap) {
        if (h == m.p) return m.slot;
        const size_t L = 2;
        if (h.size() > L && h.compare(0, L, m.p) == 0 &&
            (h[L] == '-' || h[L] == '_')) {
            return m.slot;
        }
    }
    return -1;
}

bool isKeyHeader(const std::string& rawName) {
    const std::string h = lowerAscii(trimCopy(rawName));
    return h == "keys" || h == "key" || h == "id" || h == "loc_key" || h == "lockey";
}

// Kod -> slot
int slotFromCode(const std::string& code) {
    if (code == "uz") return kUz;
    if (code == "tr") return kTr;
    if (code == "en") return kEn;
    return -1;
}

} // namespace

// ---------------------------------------------------------------------------

const char* const Loc::kLanguages[] = { "uz", "tr", "en", nullptr };

Loc& Loc::get() {
    static Loc instance;
    return instance;
}

void Loc::clear() {
    LocState& s = S();
    s.table.clear();
    s.missing.clear();
    s.missingSet.clear();
    s.missCache.clear();
}

bool Loc::loadCsv(const std::string& path) {
    if (path.empty()) return false;

    std::string text, usedPath;
    if (!readFileWithFallback(path, text, usedPath)) {
        return false;                       // fayl yo'q — halokatsiz chiqamiz
    }
    stripBom(text);
    if (text.empty()) return false;

    const std::vector<std::vector<std::string>> rows = parseCsv(text);
    if (rows.empty()) return false;

    // --- sarlavha qatoridan ustun indekslarini aniqlaymiz ---
    int keyCol = -1;
    int langCol[kLangCount] = { -1, -1, -1 };

    const std::vector<std::string>& head = rows[0];
    for (size_t c = 0; c < head.size(); ++c) {
        if (keyCol < 0 && isKeyHeader(head[c])) { keyCol = static_cast<int>(c); continue; }
        const int slot = langSlotFromHeader(head[c]);
        if (slot >= 0 && langCol[slot] < 0) langCol[slot] = static_cast<int>(c);
    }

    size_t firstData = 1;
    if (keyCol < 0) {
        // Sarlavha yo'q ko'rinadi -> standart tartibni qabul qilamiz va 1-qatorni ham o'qiymiz.
        keyCol     = 0;
        langCol[kUz] = 1;
        langCol[kTr] = 2;
        langCol[kEn] = 3;
        firstData    = 0;
    }

    LocState& s = S();
    size_t added = 0;
    for (size_t r = firstData; r < rows.size(); ++r) {
        const std::vector<std::string>& row = rows[r];
        if (row.empty()) continue;
        if (static_cast<size_t>(keyCol) >= row.size()) continue;

        const std::string key = trimCopy(row[static_cast<size_t>(keyCol)]);
        if (key.empty()) continue;
        if (key[0] == '#') continue;                    // izoh qatori

        Entry& e = s.table[key];                        // yo'q bo'lsa yaratiladi
        for (int slot = 0; slot < kLangCount; ++slot) {
            const int col = langCol[slot];
            if (col < 0 || static_cast<size_t>(col) >= row.size()) continue;
            const std::string val = trimCopy(row[static_cast<size_t>(col)]);
            // Bo'sh yacheyka = "kalit yo'q": eski qiymat saqlanadi, fallback ishlaydi.
            if (val.empty()) continue;
            e.v[slot] = unescapeText(val);              // keyingi fayl qayta yozadi
        }
        ++added;
    }

    // Yangi kalitlar kelgan bo'lishi mumkin — missing keshini tozalab qo'yamiz,
    // aks holda avval topilmagan kalit abadiy "missing" bo'lib qolardi.
    if (added > 0) {
        s.missing.clear();
        s.missingSet.clear();
        s.missCache.clear();
    }
    return added > 0;
}

void Loc::setLanguage(const std::string& code) {
    LocState& s   = S();
    const std::string c = lowerAscii(trimCopy(code));
    const int slot = slotFromCode(c);
    if (slot < 0) {                    // noma'lum kod -> "uz"
        s.lang     = "uz";
        s.langSlot = kUz;
        return;
    }
    s.lang     = c;
    s.langSlot = slot;
}

const std::string& Loc::language() const {
    return S().lang;
}

const char* Loc::languageNativeName(const std::string& code) {
    const std::string c = lowerAscii(trimCopy(code));
    if (c == "tr") return "Türkçe";
    if (c == "en") return "English";
    return "O'zbekcha";                // "uz" va noma'lum kodlar uchun
}

const std::string& Loc::tr(const std::string& key) const {
    LocState& s = S();
    if (key.empty()) return s.emptyStr;

    auto it = s.table.find(key);
    if (it != s.table.end()) {
        // Fallback tartibi: tanlangan til -> uz -> en
        const int order[kLangCount] = { s.langSlot, kUz, kEn };
        for (int k = 0; k < kLangCount; ++k) {
            const std::string& v = it->second.v[order[k]];
            if (!v.empty()) return v;
        }
    }

    // Topilmadi: diagnostikaga qo'shamiz va kalitning o'zini qaytaramiz.
    if (s.missingSet.insert(key).second) {
        s.missing.push_back(key);
    }
    // Doimiy keshda saqlaymiz -> vaqtinchalik obyektga havola qaytarilmaydi.
    return s.missCache.emplace(key, key).first->second;
}

std::string Loc::trOr(const std::string& key, const std::string& fallback) const {
    if (key.empty()) return fallback;
    LocState& s = S();
    auto it = s.table.find(key);
    if (it != s.table.end()) {
        const int order[kLangCount] = { s.langSlot, kUz, kEn };
        for (int k = 0; k < kLangCount; ++k) {
            const std::string& v = it->second.v[order[k]];
            if (!v.empty()) return v;
        }
    }
    return fallback;                   // missing ro'yxatiga QO'SHMAYDI
}

bool Loc::has(const std::string& key) const {
    if (key.empty()) return false;
    LocState& s = S();
    auto it = s.table.find(key);
    if (it == s.table.end()) return false;
    for (int k = 0; k < kLangCount; ++k) {
        if (!it->second.v[k].empty()) return true;
    }
    return false;
}

size_t Loc::keyCount() const {
    return S().table.size();
}

const std::vector<std::string>& Loc::missingKeys() const {
    return S().missing;
}

void Loc::dumpMissing(const std::string& outPath) const {
    if (outPath.empty()) return;
    LocState& s = S();

    std::ofstream f(outPath.c_str(), std::ios::binary | std::ios::trunc);
    if (!f.is_open()) return;          // yozib bo'lmasa jimgina chiqamiz

    std::vector<std::string> sorted = s.missing;
    std::sort(sorted.begin(), sorted.end());

    f << "# Topilmagan lokalizatsiya kalitlari: " << sorted.size() << "\n";
    f << "# Til: " << s.lang << ", yuklangan kalitlar: " << s.table.size() << "\n";
    f << "keys,uz,tr,en\n";
    for (const std::string& k : sorted) {
        f << k << ",,,\n";             // to'ldirish uchun tayyor CSV shakli
    }
}

// Qisqartma
const std::string& T(const std::string& key) {
    return Loc::get().tr(key);
}

} // namespace ert
