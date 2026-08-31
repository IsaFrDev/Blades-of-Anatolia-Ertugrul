// ertugrul/game/Episodes.h amalga oshirilishi.
// data/episodes/episodes_v2.json (48 epizod, 4 mavsum) ni xavfsiz o'qiydi.
// Har bir maydon tur tekshiruvi bilan olinadi — ISTISNO TASHLANMAYDI.
#include "ertugrul/game/Episodes.h"
#include "ertugrul/loc/Loc.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace ert {

namespace {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Global holat (header'da a'zo maydonlar yo'q -> hammasi shu yerda)
// ---------------------------------------------------------------------------
struct DbState {
    std::vector<Episode>          episodes;
    std::vector<Season>           seasons;
    std::map<std::string, size_t> indexById;
    bool                          usable = false;   // loaded(): foydalanish mumkin ma'lumot bormi
    std::string                   error;
};

DbState& S() {
    static DbState s;
    return s;
}

// ---------------------------------------------------------------------------
// Kichik yordamchilar
// ---------------------------------------------------------------------------
std::string lowerAscii(const std::string& s) {
    std::string r = s;
    for (char& c : r) {
        unsigned char u = static_cast<unsigned char>(c);
        if (u < 128) c = static_cast<char>(std::tolower(u));
    }
    return r;
}

std::string trimCopy(const std::string& s) {
    size_t a = 0, b = s.size();
    auto sp = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (a < b && sp(s[a])) ++a;
    while (b > a && sp(s[b - 1])) --b;
    return s.substr(a, b - a);
}

// "S3" -> 3 ; raqam topilmasa 0
int seasonNumber(const std::string& seasonId) {
    int n = 0;
    bool any = false;
    for (char c : seasonId) {
        unsigned char u = static_cast<unsigned char>(c);
        if (u >= '0' && u <= '9') {
            if (n < 100000) n = n * 10 + (c - '0');
            any = true;
        } else if (any) {
            break;
        }
    }
    return any ? n : 0;
}

// ---------------------------------------------------------------------------
// Xavfsiz JSON o'qish yordamchilari (hech qachon istisno tashlamaydi)
// ---------------------------------------------------------------------------
const json& jObj(const json& j, const char* key) {
    static const json kEmpty = json::object();
    if (!j.is_object()) return kEmpty;
    auto it = j.find(key);
    if (it == j.end() || !it->is_object()) return kEmpty;
    return *it;
}

const json& jArr(const json& j, const char* key) {
    static const json kEmpty = json::array();
    if (!j.is_object()) return kEmpty;
    auto it = j.find(key);
    if (it == j.end() || !it->is_array()) return kEmpty;
    return *it;
}

std::string jStr(const json& j, const char* key, const std::string& def = std::string()) {
    if (!j.is_object()) return def;
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return def;
    if (it->is_string())          return it->get<std::string>();
    if (it->is_number_integer())  return std::to_string(it->get<long long>());
    if (it->is_number_unsigned()) return std::to_string(it->get<unsigned long long>());
    if (it->is_boolean())         return it->get<bool>() ? "true" : "false";
    return def;
}

int jInt(const json& j, const char* key, int def = 0) {
    if (!j.is_object()) return def;
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return def;
    if (it->is_number_integer() || it->is_number_unsigned()) {
        const long long v = it->get<long long>();
        if (v >  1000000000LL) return def;
        if (v < -1000000000LL) return def;
        return static_cast<int>(v);
    }
    if (it->is_number_float()) {
        const double d = it->get<double>();
        if (d > 1e9 || d < -1e9) return def;
        return static_cast<int>(d);
    }
    if (it->is_boolean()) return it->get<bool>() ? 1 : 0;
    if (it->is_string()) {
        // Raqamli satr ham qabul qilinadi ("50")
        const std::string s = trimCopy(it->get<std::string>());
        if (s.empty()) return def;
        char* end = nullptr;
        const long v = std::strtol(s.c_str(), &end, 10);
        if (end == s.c_str()) return def;
        return static_cast<int>(v);
    }
    return def;
}

float jFloat(const json& j, const char* key, float def = 0.0f) {
    if (!j.is_object()) return def;
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return def;
    if (it->is_number()) {
        const double d = it->get<double>();
        if (d != d) return def;                        // NaN
        if (d > 1e18 || d < -1e18) return def;
        return static_cast<float>(d);
    }
    if (it->is_string()) {
        const std::string s = trimCopy(it->get<std::string>());
        if (s.empty()) return def;
        char* end = nullptr;
        const double d = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return def;
        return static_cast<float>(d);
    }
    return def;
}

bool jBool(const json& j, const char* key, bool def = false) {
    if (!j.is_object()) return def;
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return def;
    if (it->is_boolean()) return it->get<bool>();
    if (it->is_number())  return it->get<double>() != 0.0;
    if (it->is_string()) {
        const std::string s = lowerAscii(trimCopy(it->get<std::string>()));
        return s == "true" || s == "1" || s == "yes" || s == "ha";
    }
    return def;
}

std::vector<std::string> jStrArray(const json& j, const char* key) {
    std::vector<std::string> out;
    const json& a = jArr(j, key);
    out.reserve(a.size());
    for (const auto& v : a) {
        if (v.is_string()) {
            const std::string s = trimCopy(v.get<std::string>());
            if (!s.empty()) out.push_back(s);
        } else if (v.is_number_integer()) {
            out.push_back(std::to_string(v.get<long long>()));
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Fayl o'qish (build/ ichidan ishga tushirilgan holat uchun zaxira prefikslar)
// ---------------------------------------------------------------------------
bool readWholeFile(const std::string& path, std::string& out) {
    std::ifstream f(path.c_str(), std::ios::binary);
    if (!f.is_open()) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    if (f.bad()) return false;
    out = ss.str();
    return true;
}

bool readWithFallback(const std::string& path, std::string& out, std::string& usedPath) {
    if (path.empty()) return false;
    static const char* kPrefix[] = { "", "../", "../../", "../../../" };
    for (const char* p : kPrefix) {
        const std::string full = std::string(p) + path;
        if (readWholeFile(full, out)) { usedPath = full; return true; }
    }
    return false;
}

void stripBom(std::string& s) {
    if (s.size() >= 3 &&
        static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
}

// ---------------------------------------------------------------------------
// Zaxira (fallback) matnlar — hech qachon xom "ep.ep001.title" ko'rinmasin
// ---------------------------------------------------------------------------
const std::string& curLang() {
    return Loc::get().language();
}

std::string yearText(const Episode& e) {
    if (e.anchor.year > 0) return std::to_string(e.anchor.year);
    if (!e.anchor.gregorian.empty()) return e.anchor.gregorian;
    return std::string();
}

std::string fallbackTitle(const Episode& e) {
    const int sn = seasonNumber(e.seasonId) > 0 ? seasonNumber(e.seasonId) : 1;
    const int en = (e.seasonIndex >= 0 ? e.seasonIndex : 0) + 1;
    const std::string y = yearText(e);
    const std::string& L = curLang();

    std::ostringstream ss;
    if (L == "tr") {
        ss << sn << ". Sezon, " << en << ". Bölüm";
    } else if (L == "en") {
        ss << "Season " << sn << ", Episode " << en;
    } else {
        ss << sn << "-mavsum, " << en << "-epizod";
    }
    if (!y.empty()) ss << " (" << y << ")";
    return ss.str();
}

// Arxetip nomini lokalizatsiya orqali chiroyli ko'rinishga keltiradi.
std::string archetypeText(const std::string& archetype) {
    if (archetype.empty()) return std::string();
    return Loc::get().trOr("ui.arch." + archetype, archetype);
}

std::string fallbackSynopsis(const Episode& e) {
    const std::string& L = curLang();
    std::vector<std::string> parts;

    const std::string y = yearText(e);
    if (!y.empty()) parts.push_back(y);

    const std::string a = archetypeText(e.archetype);
    if (!a.empty()) parts.push_back(a);

    if (!e.region.empty()) parts.push_back(e.region);

    if (e.estimatedMinutes > 0) {
        std::ostringstream m;
        m << "~" << e.estimatedMinutes << " ";
        if (L == "tr")      m << "dakika";
        else if (L == "en") m << "min";
        else                m << "daqiqa";
        parts.push_back(m.str());
    }

    if (parts.empty()) return fallbackTitle(e);

    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) out += " · ";          // UTF-8 middle dot
        out += parts[i];
    }
    return out;
}

std::string fallbackCliffhanger(const Episode& e) {
    (void)e;
    const std::string& L = curLang();
    if (L == "tr") return "Devamı gelecek bölümde...";
    if (L == "en") return "To be continued...";
    return "Davomi keyingi epizodda...";
}

std::string fallbackIntro(const Episode& e) {
    const std::string& L = curLang();
    const std::string  y = yearText(e);
    const int sn = seasonNumber(e.seasonId) > 0 ? seasonNumber(e.seasonId) : 1;
    const int en = (e.seasonIndex >= 0 ? e.seasonIndex : 0) + 1;

    std::ostringstream ss;
    if (L == "tr") {
        if (!y.empty()) ss << y << " yılı";
        if (!e.region.empty()) { if (!y.empty()) ss << ", "; ss << e.region; }
        if (!y.empty() || !e.region.empty()) ss << ". ";
        ss << sn << ". Sezon, " << en << ". Bölüm.";
    } else if (L == "en") {
        if (!y.empty()) ss << "Year " << y;
        if (!e.region.empty()) { if (!y.empty()) ss << ", "; ss << e.region; }
        if (!y.empty() || !e.region.empty()) ss << ". ";
        ss << "Season " << sn << ", Episode " << en << ".";
    } else {
        if (!y.empty()) ss << y << "-yil";
        if (!e.region.empty()) { if (!y.empty()) ss << ", "; ss << e.region; }
        if (!y.empty() || !e.region.empty()) ss << ". ";
        ss << sn << "-mavsum, " << en << "-epizod.";
    }
    return ss.str();
}

// Lokalizatsiya kalitini yechadi; tarjima yo'q bo'lsa (natija = kalitning o'zi)
// zaxira matnni qaytaradi.
std::string resolveOr(const std::string& locKey, const std::string& fallback) {
    if (locKey.empty()) return fallback;
    const std::string& v = Loc::get().tr(locKey);
    if (v.empty() || v == locKey) return fallback;
    return v;
}

// ---------------------------------------------------------------------------
// JSON -> Episode
// ---------------------------------------------------------------------------
std::string makeDefaultId(size_t arrayIndex) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "EP%03d", static_cast<int>(arrayIndex) + 1);
    return std::string(buf);
}

Episode parseEpisode(const json& je, size_t arrayIndex) {
    Episode e;

    e.id = trimCopy(jStr(je, "id"));
    if (e.id.empty()) e.id = makeDefaultId(arrayIndex);

    e.seasonId = trimCopy(jStr(je, "season_id"));
    e.globalIndex = jInt(je, "global_index", static_cast<int>(arrayIndex));
    e.seasonIndex = jInt(je, "season_index", -1);

    if (e.seasonId.empty()) {
        // Mavsum ko'rsatilmagan bo'lsa 12 epizodli bloklar bo'yicha taxmin qilamiz.
        const int idx = (e.globalIndex >= 0) ? e.globalIndex : static_cast<int>(arrayIndex);
        char buf[8];
        std::snprintf(buf, sizeof(buf), "S%d", (idx / 12) + 1);
        e.seasonId = buf;
    }
    if (e.seasonIndex < 0) {
        const int idx = (e.globalIndex >= 0) ? e.globalIndex : static_cast<int>(arrayIndex);
        e.seasonIndex = idx % 12;
    }

    e.locTitle       = trimCopy(jStr(je, "loc_key_title"));
    e.locSynopsis    = trimCopy(jStr(je, "loc_key_synopsis"));
    e.locCliffhanger = trimCopy(jStr(je, "loc_key_cliffhanger"));

    // --- anchor ---
    const json& ja = jObj(je, "anchor");
    e.anchor.hijri      = jStr(ja, "hijri");
    e.anchor.gregorian  = jStr(ja, "gregorian");
    e.anchor.confidence = jStr(ja, "confidence", "UNKNOWN");
    e.anchor.locNote    = jStr(ja, "loc_key_note");
    e.anchor.year       = jInt(ja, "year_gregorian", 0);
    if (e.anchor.year <= 0) e.anchor.year = jInt(ja, "year", 0);

    // --- intro ---
    const json& ji = jObj(je, "intro");
    e.intro.archetype      = jStr(ji, "archetype", "WALK");
    e.intro.durationSec    = jInt(ji, "duration_sec", 120);
    if (e.intro.durationSec <= 0 || e.intro.durationSec > 3600) e.intro.durationSec = 120;
    e.intro.dateDisplay    = jStr(ji, "date_display", "OVERLAY");
    e.intro.locDescription = trimCopy(jStr(ji, "loc_key_description"));
    e.intro.allowMovement  = jBool(ji, "allow_movement", true);
    e.intro.allowCombat    = jBool(ji, "allow_combat", false);
    e.intro.directorCameraWeight = jFloat(ji, "director_camera_weight_max",
                                          jFloat(ji, "director_camera_weight", 0.35f));
    if (e.intro.directorCameraWeight < 0.0f) e.intro.directorCameraWeight = 0.0f;
    if (e.intro.directorCameraWeight > 1.0f) e.intro.directorCameraWeight = 1.0f;

    // --- asosiy maydonlar ---
    e.archetype        = jStr(je, "archetype", "INVESTIGATION");
    e.difficultyTier   = jInt(je, "difficulty_tier", 1);
    if (e.difficultyTier < 1) e.difficultyTier = 1;
    if (e.difficultyTier > 9) e.difficultyTier = 9;
    e.estimatedMinutes = jInt(je, "estimated_minutes", 0);
    if (e.estimatedMinutes < 0) e.estimatedMinutes = 0;

    const json& jec = jObj(je, "enemy_composition");
    e.maxSimultaneous = jInt(jec, "max_simultaneous", 0);
    if (e.maxSimultaneous < 0) e.maxSimultaneous = 0;

    const json& jm = jObj(je, "mih_beat");
    e.mihKind   = jStr(jm, "kind");
    e.mihLocKey = trimCopy(jStr(jm, "loc_key"));

    const json& jenv = jObj(je, "environment");
    e.season    = jStr(jenv, "season", "autumn");
    e.timeOfDay = jStr(jenv, "time_of_day", "day");
    e.weather   = jStr(jenv, "weather", "clear");
    e.region    = jStr(jenv, "region");

    const json& jau = jObj(je, "audio");
    e.musicCue    = jStr(jau, "music_cue");
    e.ambienceTag = jStr(jau, "ambience_tag");

    e.retentionHook = trimCopy(jStr(je, "retention_hook"));

    e.characters   = jStrArray(je, "characters");
    e.levels       = jStrArray(je, "levels");
    e.codexUnlocks = jStrArray(je, "codex_unlocks");
    e.unlocks      = jStrArray(je, "unlocks");
    e.prerequisites= jStrArray(je, "prerequisites");
    e.mechanics    = jStrArray(je, "mechanics_introduced");
    if (e.mechanics.empty()) e.mechanics = jStrArray(je, "mechanics");
    e.traversal    = jStrArray(je, "traversal");

    // Ochilish sahnasi id'si: "EP001" -> "ep001_intro"
    e.cutsceneId = lowerAscii(e.id) + "_intro";

    return e;
}

// Epizod indekslarini tartiblash (mavsum ichidagi tartib bo'yicha)
void sortSeasonEpisodes(const std::vector<Episode>& eps, std::vector<size_t>& idx) {
    std::sort(idx.begin(), idx.end(), [&eps](size_t a, size_t b) {
        if (a >= eps.size() || b >= eps.size()) return a < b;
        if (eps[a].seasonIndex != eps[b].seasonIndex) return eps[a].seasonIndex < eps[b].seasonIndex;
        return eps[a].globalIndex < eps[b].globalIndex;
    });
}

// seasons massivi bo'lmasa epizodlardagi season_id lardan mavsumlarni quradi.
void buildSeasonsFromEpisodes(DbState& s) {
    s.seasons.clear();
    std::map<std::string, size_t> pos;
    for (size_t i = 0; i < s.episodes.size(); ++i) {
        const std::string& sid = s.episodes[i].seasonId;
        auto it = pos.find(sid);
        if (it == pos.end()) {
            Season se;
            se.id       = sid;
            se.locTitle = "season." + lowerAscii(sid) + ".title";
            se.episodes.push_back(i);
            pos[sid] = s.seasons.size();
            s.seasons.push_back(se);
        } else {
            s.seasons[it->second].episodes.push_back(i);
        }
    }
    for (Season& se : s.seasons) sortSeasonEpisodes(s.episodes, se.episodes);
}

void buildSeasons(DbState& s, const json& root) {
    const json& ja = jArr(root, "seasons");
    if (ja.empty()) {
        buildSeasonsFromEpisodes(s);
        return;
    }

    s.seasons.clear();
    s.seasons.reserve(ja.size());

    for (const auto& js : ja) {
        if (!js.is_object()) continue;
        Season se;
        se.id = trimCopy(jStr(js, "id"));
        if (se.id.empty()) {
            const int n = jInt(js, "index", static_cast<int>(s.seasons.size()) + 1);
            char buf[8];
            std::snprintf(buf, sizeof(buf), "S%d", n > 0 ? n : 1);
            se.id = buf;
        }
        se.locTitle = trimCopy(jStr(js, "loc"));
        if (se.locTitle.empty()) se.locTitle = trimCopy(jStr(js, "loc_key_title"));
        if (se.locTitle.empty()) se.locTitle = "season." + lowerAscii(se.id) + ".title";

        // episode_ids bo'yicha
        for (const std::string& eid : jStrArray(js, "episode_ids")) {
            auto it = s.indexById.find(eid);
            if (it != s.indexById.end()) se.episodes.push_back(it->second);
        }
        // season_id mos keladigan, lekin ro'yxatga tushmagan epizodlarni ham qo'shamiz
        for (size_t i = 0; i < s.episodes.size(); ++i) {
            if (s.episodes[i].seasonId != se.id) continue;
            if (std::find(se.episodes.begin(), se.episodes.end(), i) == se.episodes.end()) {
                se.episodes.push_back(i);
            }
        }
        sortSeasonEpisodes(s.episodes, se.episodes);
        s.seasons.push_back(se);
    }

    // Hech bir mavsumga tushmagan epizodlar bo'lsa — ularni ham joylashtiramiz.
    std::vector<bool> placed(s.episodes.size(), false);
    for (const Season& se : s.seasons) {
        for (size_t i : se.episodes) {
            if (i < placed.size()) placed[i] = true;
        }
    }
    for (size_t i = 0; i < s.episodes.size(); ++i) {
        if (placed[i]) continue;
        const std::string& sid = s.episodes[i].seasonId;
        auto it = std::find_if(s.seasons.begin(), s.seasons.end(),
                               [&sid](const Season& x) { return x.id == sid; });
        if (it == s.seasons.end()) {
            Season se;
            se.id       = sid;
            se.locTitle = "season." + lowerAscii(sid) + ".title";
            se.episodes.push_back(i);
            s.seasons.push_back(se);
        } else {
            it->episodes.push_back(i);
            sortSeasonEpisodes(s.episodes, it->episodes);
        }
    }

    if (s.seasons.empty()) buildSeasonsFromEpisodes(s);
}

// ---------------------------------------------------------------------------
// Zaxira baza: JSON o'qilmasa ham menyu bo'sh qolmasin (4 mavsum x 12 epizod)
// ---------------------------------------------------------------------------
void buildFallbackDb(DbState& s) {
    s.episodes.clear();
    s.seasons.clear();
    s.indexById.clear();

    struct SeasonDef { const char* id; int y0; int y1; const char* region; const char* season; };
    static const SeasonDef kSeasons[4] = {
        { "S1", 1227, 1230, "Amanos / Halab",     "autumn"     },
        { "S2", 1230, 1238, "Konya / Kubadabad",  "summer"     },
        { "S3", 1238, 1243, "Erzurum / Köse Dağ", "winter"     },
        { "S4", 1243, 1261, "Söğüt / Domaniç",    "full_cycle" },
    };
    static const char* kArch[9] = {
        "INVESTIGATION", "SIEGE", "INFILTRATION", "COURT", "ESCORT",
        "DEFENSE", "CHASE", "SURVIVAL", "RITUAL"
    };

    const int kPerSeason = 12;
    int global = 0;
    for (int si = 0; si < 4; ++si) {
        const SeasonDef& sd = kSeasons[si];
        Season se;
        se.id       = sd.id;
        se.locTitle = std::string("season.") + lowerAscii(sd.id) + ".title";

        for (int ei = 0; ei < kPerSeason; ++ei) {
            Episode e;
            char buf[16];
            std::snprintf(buf, sizeof(buf), "EP%03d", global + 1);
            e.id          = buf;
            e.seasonId    = sd.id;
            e.globalIndex = global;
            e.seasonIndex = ei;

            const std::string low = lowerAscii(e.id);
            e.locTitle       = "ep." + low + ".title";
            e.locSynopsis    = "ep." + low + ".synopsis";
            e.locCliffhanger = "ep." + low + ".cliffhanger";
            e.intro.locDescription = "ep." + low + ".intro";

            const int span = (sd.y1 > sd.y0) ? (sd.y1 - sd.y0) : 0;
            e.anchor.year       = sd.y0 + (span * ei) / (kPerSeason - 1);
            e.anchor.gregorian  = std::to_string(e.anchor.year);
            e.anchor.confidence = "INFERRED";

            e.archetype        = kArch[global % 9];
            e.difficultyTier   = 1 + (si * 2 > 5 ? 5 : si * 2);
            e.estimatedMinutes = 45;
            e.maxSimultaneous  = 4;
            e.season           = sd.season;
            e.timeOfDay        = "day";
            e.weather          = "clear";
            e.region           = sd.region;
            e.musicCue         = "MUS_" + e.id;
            e.ambienceTag      = "Ambience.Default";
            e.cutsceneId       = low + "_intro";

            se.episodes.push_back(s.episodes.size());
            s.indexById[e.id] = s.episodes.size();
            s.episodes.push_back(e);
            ++global;
        }
        s.seasons.push_back(se);
    }
    s.usable = !s.episodes.empty();
}

} // namespace

// ---------------------------------------------------------------------------

EpisodeDb& EpisodeDb::get() {
    static EpisodeDb instance;
    return instance;
}

bool EpisodeDb::load(const std::string& jsonPath) {
    DbState& s = S();
    s.episodes.clear();
    s.seasons.clear();
    s.indexById.clear();
    s.usable = false;
    s.error.clear();

    std::string text, usedPath;
    if (!readWithFallback(jsonPath, text, usedPath)) {
        s.error = "Epizod fayli topilmadi: " + jsonPath;
        buildFallbackDb(s);
        return false;
    }
    stripBom(text);
    if (text.empty()) {
        s.error = "Epizod fayli bo'sh: " + usedPath;
        buildFallbackDb(s);
        return false;
    }

    // allow_exceptions = false -> xato bo'lsa 'discarded' qaytadi, istisno tashlanmaydi.
    json root = json::parse(text.begin(), text.end(), nullptr, false, true);
    if (root.is_discarded() || !root.is_object()) {
        s.error = "JSON tahlil xatosi: " + usedPath;
        buildFallbackDb(s);
        return false;
    }

    const json& jeps = jArr(root, "episodes");
    if (jeps.empty()) {
        s.error = "'episodes' massivi yo'q yoki bo'sh: " + usedPath;
        buildFallbackDb(s);
        return false;
    }

    s.episodes.reserve(jeps.size());
    size_t i = 0;
    for (const auto& je : jeps) {
        if (!je.is_object()) { ++i; continue; }
        Episode e = parseEpisode(je, i);
        // Takrorlangan id bo'lsa noyob qilamiz (indeks bo'yicha qidiruv buzilmasin).
        if (s.indexById.find(e.id) != s.indexById.end()) {
            e.id = e.id + "_" + std::to_string(i);
            e.cutsceneId = lowerAscii(e.id) + "_intro";
        }
        s.indexById[e.id] = s.episodes.size();
        s.episodes.push_back(e);
        ++i;
    }

    if (s.episodes.empty()) {
        s.error = "Hech bir epizod o'qilmadi: " + usedPath;
        buildFallbackDb(s);
        return false;
    }

    buildSeasons(s, root);
    s.usable = true;
    return true;
}

bool EpisodeDb::loaded() const {
    // "Foydalanish mumkin ma'lumot bormi" — zaxira baza ham hisobga olinadi,
    // shunda menyu hech qachon bo'sh qolmaydi. Xato holati lastError() da.
    return S().usable;
}

const std::vector<Episode>& EpisodeDb::all() const {
    return S().episodes;
}

const std::vector<Season>& EpisodeDb::seasons() const {
    return S().seasons;
}

size_t EpisodeDb::count() const {
    return S().episodes.size();
}

const Episode* EpisodeDb::byId(const std::string& id) const {
    if (id.empty()) return nullptr;
    const DbState& s = S();
    auto it = s.indexById.find(id);
    if (it == s.indexById.end() || it->second >= s.episodes.size()) return nullptr;
    return &s.episodes[it->second];
}

const Episode* EpisodeDb::byIndex(size_t i) const {
    const DbState& s = S();
    if (i >= s.episodes.size()) return nullptr;
    return &s.episodes[i];
}

const Season* EpisodeDb::seasonById(const std::string& id) const {
    if (id.empty()) return nullptr;
    const DbState& s = S();
    for (const Season& se : s.seasons) {
        if (se.id == id) return &se;
    }
    return nullptr;
}

std::string EpisodeDb::title(const Episode& e) const {
    return resolveOr(e.locTitle, fallbackTitle(e));
}

std::string EpisodeDb::synopsis(const Episode& e) const {
    return resolveOr(e.locSynopsis, fallbackSynopsis(e));
}

std::string EpisodeDb::cliffhanger(const Episode& e) const {
    std::string v = resolveOr(e.locCliffhanger, std::string());
    if (!v.empty()) return v;
    // Zaxira sifatida retention_hook kalitini ham sinab ko'ramiz
    v = resolveOr(e.retentionHook, std::string());
    if (!v.empty()) return v;
    return fallbackCliffhanger(e);
}

std::string EpisodeDb::introText(const Episode& e) const {
    return resolveOr(e.intro.locDescription, fallbackIntro(e));
}

const std::string& EpisodeDb::lastError() const {
    return S().error;
}

} // namespace ert
