// Epizod jangi: maqsadlar, to'lqinlar, nazorat nuqtalari va qayta tug'ilish.
//
// TO'LIQ OQIM:
//   begin()  -> jang rejasi quriladi (arxetip + qiyinlik + max_simultaneous)
//   Briefing -> Fighting -> (WaveCleared -> Fighting)* -> Cleared
//                        \-> Failed -> restartFromCheckpoint() -> Briefing
#include "ertugrul/game/Encounter.h"
#include "ertugrul/game/Character.h"
#include "ertugrul/game/Episodes.h"
#include "ertugrul/game/Projectile.h"
#include "ertugrul/audio/Sfx.h"
#include "ertugrul/world/Level.h"
#include "ertugrul/world/Physics.h"
#include "ertugrul/loc/Loc.h"

#include <windows.h>
#include <GL/gl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace ert {

namespace {

// «Temir va Firuza» palitrasi (HUD bilan bir xil)
const float C_FERUZA[3] = {0.282f, 0.663f, 0.710f};
const float C_ZARHAL[3] = {0.753f, 0.588f, 0.314f};
const float C_YARA[3]   = {0.737f, 0.353f, 0.267f};
const float C_MATN[3]   = {0.894f, 0.918f, 0.918f};

// Nazorat nuqtasida saqlanadigan holat
struct Checkpoint {
    bool  valid = false;
    int   wave = 0;
    int   phase = 0;
    Vec3  playerPos{0, 0, 0};
    float playerYaw = 0.0f;
    std::vector<Objective> objectives;
    EncounterResult result;
    std::string name;
};

// Missiya bosqichi. Epizod = bosqichlar zanjiri; har bosqichning o'z maqsadlari
// va (bo'lsa) dushman to'lqinlari bor. Bosqich tugagach nazorat nuqtasi saqlanadi.
struct Phase {
    std::string            titleKey;      // ui.phase.*
    std::vector<Objective> objectives;
    std::vector<Wave>      waves;         // bo'sh bo'lishi mumkin (yo'l, qidiruv)
    Wave                   reinforce;     // yashirin bosqich: sezilsa qo'shimcha kuch
    bool                   reinforced = false;
};

struct Runtime {
    const Episode* ep = nullptr;
    Level*         level = nullptr;
    PhysicsWorld*  phys = nullptr;
    Character*     player = nullptr;

    EnemyManager   enemies;
    ArrowPool      arrows;          // o'yinchi o'qlari
    std::vector<Wave>      waves;
    std::vector<Objective> objectives;
    std::vector<Phase>     phases;
    int   phase  = 0;
    float phaseT = 0.0f;
    std::string phaseTitle;
    int   assassinations = 0;
    EncounterState state = EncounterState::Inactive;

    int   wave = 0;
    float stateT = 0.0f;
    float noiseT = 0.0f;
    float cpFlash = 0.0f;
    bool  waveFlawless = true;      // joriy to'lqinda zarba yemadi
    // --- Zarba qaytarmasi: muzlash va uchqunlar (HAQIQIY vaqtda so'nadi) ---
    float hitStopT = 0.0f;
    float pickupT  = 0.0f;          // o'q yig'ish HUD chaqnashi
    float shakeAmp = 0.0f;          // kamera silkinishi 0..1
    float shakeT   = 0.0f;          // silkinish fazasi (haqiqiy vaqt)
    struct Spark { Vec3 p; float t; int kind; };   // 0 = tegdi, 1 = bloklandi, 2 = o'ldi
    std::vector<Spark> sparks;
    EncounterResult result;
    Checkpoint cp;
    std::string episodeId, cpName;
    int   lockIdx = -1;
    uint32_t seed = 1u;
};

Runtime& RT() { static Runtime r; return r; }

// --- yordamchilar -------------------------------------------------------

int seasonNumber(const std::string& sid) {
    if (sid.size() >= 2 && (sid[0] == 'S' || sid[0] == 's')) {
        const int n = sid[1] - '0';
        if (n >= 1 && n <= 4) return n;
    }
    return 1;
}

uint32_t seedFrom(const std::string& s) {
    uint32_t h = 2166136261u;
    for (char c : s) { h ^= (uint32_t)(unsigned char)c; h *= 16777619u; }
    return h ? h : 1u;
}

// Mavsum va qiyinlikka qarab dushman turi
EnemyKind pickKind(int season, int tier, Rng& rng) {
    const float r = rng.nextFloat();
    switch (season) {
    case 1:
        if (tier >= 2 && r > 0.80f) return EnemyKind::Crossbow;
        if (r > 0.62f)              return EnemyKind::Sergeant;
        return EnemyKind::Footman;
    case 2:
        if (r > 0.78f) return EnemyKind::Assassin;
        if (r > 0.55f) return EnemyKind::Sergeant;
        return EnemyKind::Footman;
    case 3:
        if (r > 0.76f) return EnemyKind::HorseArcher;
        if (r > 0.52f) return EnemyKind::Sergeant;
        return EnemyKind::Footman;
    default:
        if (r > 0.80f) return EnemyKind::HorseArcher;
        if (r > 0.50f) return EnemyKind::Elite;
        return EnemyKind::Sergeant;
    }
}

// O'yinchidan minR..maxR masofada, band bo'lmagan joy topadi
Vec3 findSpot(const Vec3& around, float minR, float maxR, Rng& rng,
              PhysicsWorld* phys, Level* level) {
    for (int attempt = 0; attempt < 14; ++attempt) {
        const float a = rng.range(0.0f, TAU);
        const float d = rng.range(minR, maxR);
        Vec3 p{ around.x + std::cos(a) * d, 0.0f, around.z + std::sin(a) * d };
        if (level) level->terrain().clampToBounds(p, 6.0f);
        p.y = phys ? phys->groundAt(p.x, p.z)
                   : (level ? level->groundAt(p.x, p.z) : 0.0f);
        if (!phys || phys->mantleClear(p, 0.5f, 1.85f)) return p;
    }
    Vec3 p{ around.x + minR, 0.0f, around.z };
    p.y = phys ? phys->groundAt(p.x, p.z) : 0.0f;
    return p;
}

void addObjective(std::vector<Objective>& v, ObjectiveKind k, const char* loc,
                  int target, bool optional = false,
                  const Vec3& point = Vec3{0, 0, 0}, float radius = 4.0f) {
    Objective o;
    o.kind = k; o.locKey = loc; o.target = target;
    o.optional = optional; o.point = point; o.radius = radius;
    v.push_back(o);
}

// Joriy to'lqin dushmanlarini yaratadi
void spawnWave(Runtime& rt, int index) {
    rt.enemies.clear();
    if (index < 0 || index >= (int)rt.waves.size()) return;
    for (const EnemySpawn& s : rt.waves[(size_t)index].spawns)
        rt.enemies.spawn(s, rt.phys);
    rt.lockIdx = -1;
}

// Mavjud dushmanlarni O'CHIRMASDAN qo'shimcha kuch chiqaradi
void saveCheckpoint(Runtime& rt);

void spawnExtra(Runtime& rt, const Wave& w) {
    for (const EnemySpawn& s : w.spawns) rt.enemies.spawn(s, rt.phys);
}

// Ko'tarilgan nuqta: chiqib bo'ladigan quti (tom, qoya, devor) ustidagi joy.
// Topilmasa oddiy yer nuqtasi. Shunday nuqtaga yetish uchun sakrash/chiqish kerak.
Vec3 findElevated(const Vec3& around, float minR, float maxR, Rng& rng,
                  PhysicsWorld* phys, Level* level, bool& elevated) {
    elevated = false;
    if (phys) {
        std::vector<const Box*> cand;
        for (const Box& b : phys->boxes()) {
            if (!b.climbable && !b.vaultable) continue;
            const float cx = b.centerX(), cz = b.centerZ();
            const float d = std::sqrt((cx - around.x) * (cx - around.x) + (cz - around.z) * (cz - around.z));
            if (d < minR || d > maxR) continue;
            const float g  = phys->groundAt(cx, cz);
            const float hh = b.topY - g;
            if (hh < 1.2f || hh > 6.5f) continue;
            if ((b.maxX - b.minX) < 1.2f || (b.maxZ - b.minZ) < 1.2f) continue;   // turib bo'lmaydi
            cand.push_back(&b);
        }
        if (!cand.empty()) {
            const Box& b = *cand[(size_t)(rng.nextFloat() * 0.999f * (float)cand.size())];
            elevated = true;
            return Vec3{b.centerX(), b.topY, b.centerZ()};
        }
    }
    return findSpot(around, minR, maxR, rng, phys, level);
}

void startPhase(Runtime& rt, int idx) {
    if (idx < 0 || idx >= (int)rt.phases.size()) return;
    Phase& P = rt.phases[(size_t)idx];
    rt.phase      = idx;
    rt.phaseT     = 0.0f;
    rt.objectives = P.objectives;
    rt.waves      = P.waves;
    rt.wave       = 0;
    rt.waveFlawless = true;
    spawnWave(rt, 0);                        // to'lqin yo'q bo'lsa dushmanlarni tozalaydi
    for (Objective& o : rt.objectives) {
        if (o.kind == ObjectiveKind::DefeatAll) { o.target = rt.enemies.total(); o.progress = 0; }
        if (o.kind == ObjectiveKind::Hunt) {
            int n = 0;
            for (const Enemy& e : rt.enemies.all()) if (e.kind() == EnemyKind::Deer) ++n;
            if (n > 0) o.target = n;
        }
    }
    rt.phaseTitle = Loc::get().trOr(P.titleKey, P.titleKey);
    saveCheckpoint(rt);
    std::printf("[Missiya] %s bosqich %d/%d: %s (maqsad %d, to'lqin %d)\n",
                rt.episodeId.c_str(), idx + 1, (int)rt.phases.size(), P.titleKey.c_str(),
                (int)P.objectives.size(), (int)P.waves.size());
}

void saveCheckpoint(Runtime& rt) {
    rt.cp.valid      = true;
    rt.cp.wave       = rt.wave;
    rt.cp.phase      = rt.phase;
    rt.cp.playerPos  = rt.player ? rt.player->position() : Vec3{0, 0, 0};
    rt.cp.playerYaw  = rt.player ? rt.player->yaw() : 0.0f;
    rt.cp.objectives = rt.objectives;
    rt.cp.result     = rt.result;

    // Nazorat nuqtasi nomi (Episode strukturasida checkpoints maydoni yo'q,
    // shuning uchun epizod id sidan quramiz)
    {
        char b[64];
        std::snprintf(b, sizeof b, "CP_%s_W%d", rt.episodeId.c_str(), rt.wave + 1);
        rt.cp.name = b;
    }
    rt.cpName  = rt.cp.name;
    rt.cpFlash = 1.0f;
}

// Kameraga qaragan kvadrat («mix boshi» motivi)
void billboardSquare(const Vec3& p, float size, const float col[3], float alpha) {
    GLfloat mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    // Ko'rinish matritsasining aylanish qismi transponirlansa — kameraning
    // o'ng va yuqori vektorlari chiqadi.
    const Vec3 right{ mv[0], mv[4], mv[8] };
    const Vec3 up   { mv[1], mv[5], mv[9] };
    const Vec3 r = right * (size * 0.5f);
    const Vec3 u = up    * (size * 0.5f);
    glColor4f(col[0], col[1], col[2], alpha);
    glBegin(GL_QUADS);
    glVertex3f(p.x - r.x - u.x, p.y - r.y - u.y, p.z - r.z - u.z);
    glVertex3f(p.x + r.x - u.x, p.y + r.y - u.y, p.z + r.z - u.z);
    glVertex3f(p.x + r.x + u.x, p.y + r.y + u.y, p.z + r.z + u.z);
    glVertex3f(p.x - r.x + u.x, p.y - r.y + u.y, p.z - r.z + u.z);
    glEnd();
}

} // namespace

// ===========================================================================
// Objective
// ===========================================================================

std::string Objective::text() const {
    std::string base = Loc::get().trOr(locKey, locKey);
    char b[64];
    switch (kind) {
    case ObjectiveKind::DefeatAll:
    case ObjectiveKind::DefeatCount:
        if (target > 0) {
            std::snprintf(b, sizeof b, "  %d/%d", progress, target);
            base += b;
        }
        break;
    case ObjectiveKind::SurviveTime:
    case ObjectiveKind::HoldPoint:
    case ObjectiveKind::Timer:
        std::snprintf(b, sizeof b, "  %d s", std::max(0, target - progress));
        base += b;
        break;
    case ObjectiveKind::Route:
        std::snprintf(b, sizeof b, "  %d/%d", progress, (int)points.size());
        base += b;
        break;
    case ObjectiveKind::Hunt:
    case ObjectiveKind::Collect:
        std::snprintf(b, sizeof b, "  %d/%d", progress, target);
        base += b;
        break;
    default: break;
    }
    if (failed) base += "  (" + Loc::get().trOr("ui.obj.failed", "failed") + ")";
    return base;
}

// ===========================================================================
// Encounter
// ===========================================================================

Encounter& Encounter::get() { static Encounter e; return e; }

// Kamera turtkisi. Eng kuchlisi saqlanadi — ketma-ket zarbalar bir-birini
// qo'shib yubormasin (aks holda ekran uzluksiz chayqalardi).
void kick(Runtime& rt, float amp) {
    if (!(amp > 0.0f)) return;
    if (amp > rt.shakeAmp) rt.shakeAmp = clampf(amp, 0.0f, 1.0f);
    static const bool fxLog = (std::getenv("ERT_FX_LOG") != nullptr);
    if (fxLog)
        std::printf("[fx] turtki=%.2f  silkinish=%.2f  muzlash=%.3f s\n",
                    amp, rt.shakeAmp, rt.hitStopT);
}

// Uchqun qo'shish (eng eskisi tashlanadi)
void pushSpark(Runtime& rt, const Vec3& p, int kind) {
    if (!(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z))) return;
    if (rt.sparks.size() >= 24) rt.sparks.erase(rt.sparks.begin());
    Runtime::Spark s; s.p = p; s.t = 0.0f; s.kind = kind;
    rt.sparks.push_back(s);
}

// O'q urilishini o'yin hodisasiga aylantiradi.
void resolveArrowHit(Runtime& rt, Character& pl, const ArrowHit& h) {
    if (h.kind == ArrowHitKind::Enemy) {
        bool killed = false;
        rt.enemies.arrowHit(h.enemyIndex, h.zone, h.charge, h.dist,
                            pl.position(), h.silent, killed);
        Sfx::get().play(SfxId::ArrowHit, distanceXZ(h.point, pl.position()));
        if (killed) {
            ++rt.result.kills;
            // Sezilmay otilgan o'q — deyarli yashirin o'ldirish
            pl.faith.add(h.silent ? 0.4f : 0.2f);
            // Jasad shovqini: jim otilgan bo'lsa atigi 6 m
            rt.enemies.broadcastNoise(h.point, h.silent ? 6.0f : 14.0f,
                                                h.silent ? 0.35f : 0.7f);
        } else {
            rt.enemies.broadcastNoise(h.point, 12.0f, 0.6f);   // og'rigan qichqiradi
        }
    } else if (h.kind == ArrowHitKind::World) {
        // DIQQATNI CHALG'ITISH: devorga tekkan o'q qorovulni o'sha yoqqa yuboradi
        rt.enemies.broadcastNoise(h.point, 9.0f, 0.45f);
        Sfx::get().play(SfxId::ArrowWall, distanceXZ(h.point, pl.position()));
    }
}

bool Encounter::begin(const Episode& ep, Level& level, PhysicsWorld& phys, Character& player) {
    Runtime& rt = RT();
    rt.ep = &ep;
    rt.level = &level;
    rt.phys = &phys;
    rt.player = &player;
    rt.episodeId = ep.id;
    rt.seed = seedFrom(ep.id);
    rt.waves.clear();
    rt.objectives.clear();
    rt.enemies.clear();
    rt.result = EncounterResult();
    rt.cp = Checkpoint();
    rt.waveFlawless = true;
    rt.arrows.clear();
    rt.hitStopT = 0.0f;
    rt.shakeAmp = 0.0f;
    rt.sparks.clear();
    // Iymon epizodlar orasida saqlanadi - jang uni davom ettiradi, nolga qaytarmaydi
    player.faith.reset(Progress::get().faith());
    rt.wave = 0;
    rt.stateT = 0.0f;
    rt.cpFlash = 0.0f;
    rt.lockIdx = -1;

    Rng rng(rt.seed);
    const int season = seasonNumber(ep.seasonId);
    const int tier   = clampf((float)ep.difficultyTier, 1.0f, 5.0f);
    int perWave = ep.maxSimultaneous;
    if (perWave <= 0) perWave = 3;
    perWave = (int)clampf((float)perWave, 2.0f, 8.0f);
    rt.phases.clear();
    rt.phase = 0; rt.phaseT = 0.0f; rt.assassinations = 0;

    // ------------------------------------------------------------------
    // Bosqich quruvchilar. Har biri o'yinchining (yoki oldingi bosqich
    // oxirining) atrofida joylashadi, shunda epizod xaritada OLDINGA yuradi.
    // ------------------------------------------------------------------
    Vec3 cursor = player.position();

    auto makeWave = [&](int n, float delay, bool patrol) {
        Wave wv; wv.delay = delay;
        for (int i = 0; i < n; ++i) {
            EnemySpawn sp;
            sp.kind = pickKind(season, tier, rng);
            const EnemyStats& st = enemyStats(sp.kind);
            const float minR = (st.attackRange > 6.0f) ? 18.0f : 13.0f;
            sp.pos = findSpot(cursor, minR, minR + 13.0f, rng, &phys, &level);
            sp.yaw = yawFromDir(normalize(Vec3{cursor.x - sp.pos.x, 0.0f, cursor.z - sp.pos.z}));
            sp.patrolRadius = patrol ? rng.range(4.0f, 8.0f) : 0.0f;
            wv.spawns.push_back(sp);
        }
        return wv;
    };

    // YO'L: n ta nuqta zanjiri; imkon bo'lsa tom/qoya ustida — sakrash, chiqish kerak.
    auto addTravel = [&](int n, bool timed) {
        Phase P; P.titleKey = "ui.phase.travel";
        Objective o; o.kind = ObjectiveKind::Route; o.locKey = "ui.obj.route"; o.radius = 2.6f;
        Vec3 c = cursor;
        bool anyElev = false;
        for (int k = 0; k < n; ++k) {
            bool el = false;
            Vec3 pnt = (k % 2 == 1) ? findElevated(c, 16.0f, 30.0f, rng, &phys, &level, el)
                                    : findSpot(c, 18.0f, 30.0f, rng, &phys, &level);
            anyElev = anyElev || el;
            o.points.push_back(pnt);
            c = pnt;
        }
        o.needY  = anyElev;
        o.target = n;
        P.objectives.push_back(o);
        if (timed) {
            Objective t; t.kind = ObjectiveKind::Timer; t.locKey = "ui.obj.timer";
            t.target = 14 * n; t.optional = true; P.objectives.push_back(t);
        }
        cursor = c;
        rt.phases.push_back(P);
    };
    // OV: kiyiklar 26-44 m da o'tlaydi, sezsa qochadi — kamon kerak.
    auto addHunt = [&](int n) {
        Phase P; P.titleKey = "ui.phase.hunt";
        Wave wv;
        for (int i = 0; i < n; ++i) {
            EnemySpawn sp; sp.kind = EnemyKind::Deer;
            sp.pos = findSpot(cursor, 26.0f, 44.0f, rng, &phys, &level);
            sp.yaw = rng.range(0.0f, 360.0f);
            sp.patrolRadius = rng.range(5.0f, 9.0f);
            wv.spawns.push_back(sp);
        }
        P.waves.push_back(wv);
        Objective o; o.kind = ObjectiveKind::Hunt; o.locKey = "ui.obj.hunt"; o.target = n;
        P.objectives.push_back(o);
        rt.phases.push_back(P);
    };
    // YASHIRIN: qorovullar aylanib yuradi; sezilmasdan yo'q qiling. Sezilsa — qo'shimcha kuch.
    auto addStealth = [&](int guards) {
        Phase P; P.titleKey = "ui.phase.stealth";
        P.waves.push_back(makeWave(guards, 0.0f, true));
        P.reinforce = makeWave(std::max(2, perWave - 1), 0.0f, false);
        addObjective(P.objectives, ObjectiveKind::StayUndetected, "ui.obj.undetected", 0, true);
        addObjective(P.objectives, ObjectiveKind::DefeatAll, "ui.obj.guards", 0);
        rt.phases.push_back(P);
    };
    // HIMOYA: nuqtani N sekund ushlab turing; to'lqinlar ketma-ket keladi.
    auto addDefend = [&](int seconds, int waves) {
        Phase P; P.titleKey = "ui.phase.defend";
        const Vec3 pt = findSpot(cursor, 6.0f, 12.0f, rng, &phys, &level);
        Objective o; o.kind = ObjectiveKind::HoldPoint; o.locKey = "ui.obj.hold";
        o.point = pt; o.radius = 7.0f; o.target = seconds; P.objectives.push_back(o);
        for (int w = 0; w < waves; ++w) P.waves.push_back(makeWave(perWave + (w == waves - 1 ? 1 : 0), 1.2f, false));
        cursor = pt;
        rt.phases.push_back(P);
    };
    // QIDIRUV: N ta dalil — markerlarga yaqin boring.
    auto addCollect = [&](int n) {
        Phase P; P.titleKey = "ui.phase.collect";
        Objective o; o.kind = ObjectiveKind::Collect; o.locKey = "ui.obj.collect";
        o.radius = 1.9f; o.target = n;
        for (int k = 0; k < n; ++k) o.points.push_back(findSpot(cursor, 10.0f, 28.0f, rng, &phys, &level));
        P.objectives.push_back(o);
        rt.phases.push_back(P);
    };
    // YAKKAMA-YAKKA: bitta kuchli raqib.
    auto addDuel = [&]() {
        Phase P; P.titleKey = "ui.phase.duel";
        Wave wv;
        EnemySpawn sp; sp.kind = (tier >= 4) ? EnemyKind::Elite : (tier >= 2 ? EnemyKind::Sergeant : EnemyKind::Footman);
        sp.pos = findSpot(cursor, 10.0f, 16.0f, rng, &phys, &level);
        sp.yaw = yawFromDir(normalize(Vec3{cursor.x - sp.pos.x, 0.0f, cursor.z - sp.pos.z}));
        wv.spawns.push_back(sp);
        P.waves.push_back(wv);
        addObjective(P.objectives, ObjectiveKind::DefeatAll, "ui.obj.duel", 0);
        rt.phases.push_back(P);
    };
    // JANG: to'lqinlar.
    auto addFight = [&](int waves, bool survive) {
        Phase P; P.titleKey = "ui.phase.fight";
        for (int w = 0; w < waves; ++w) P.waves.push_back(makeWave(perWave + (w == waves - 1 ? 1 : 0), 1.2f, w == 0));
        addObjective(P.objectives, ObjectiveKind::DefeatAll, "ui.obj.defeat_all", 0);
        if (survive) addObjective(P.objectives, ObjectiveKind::SurviveTime, "ui.obj.survive", 30 + tier * 6, true);
        rt.phases.push_back(P);
    };

    // ------------------------------------------------------------------
    // Arxetip bo'yicha ssenariy. Har epizodda jang bor, lekin hammasi jang emas:
    // yo'l, ov, qidiruv, yashirin, himoya, yakkama-yakka aralashadi.
    // ------------------------------------------------------------------
    const std::string& a = ep.archetype;
    const int fightWaves = (tier <= 1) ? 1 : (tier >= 4 ? 3 : 2);
    if (a == "SIEGE" || a == "DEFENSE") {
        addTravel(2, true);
        addDefend(36 + tier * 8, 2);
        addTravel(2, false);
        addFight(fightWaves, true);
    } else if (a == "SURVIVAL") {
        addHunt(2);
        addTravel(3, false);
        addFight(fightWaves, true);
    } else if (a == "INFILTRATION") {
        addTravel(2, false);
        addStealth(std::max(2, std::min(4, perWave)));
        addCollect(2);
        addTravel(2, true);
    } else if (a == "CHASE") {
        addTravel(4, true);
        addFight(1, false);
        addTravel(2, true);
        addDuel();
    } else if (a == "ESCORT") {
        addTravel(2, false);
        addDefend(30 + tier * 6, 2);
        addTravel(2, false);
        addFight(1, false);
    } else if (a == "RITUAL") {
        addCollect(3);
        addHunt(1);
        addTravel(2, false);
        addDuel();
    } else if (a == "COURT") {
        addCollect(2);
        addTravel(2, false);
        addStealth(2);
        addDuel();
    } else {   // INVESTIGATION va boshqalar
        addTravel(2, false);
        addCollect(3);
        addStealth(2);
        addFight(1, false);
    }

    startPhase(rt, 0);
    rt.state = EncounterState::Briefing;
    rt.stateT = 0.0f;

    std::printf("[Jang] %s: %d bosqich, %d dushman/to'lqin (arxetip %s, tier %d)\n",
                ep.id.c_str(), (int)rt.phases.size(), perWave, a.c_str(), tier);
    return true;
}

void Encounter::stop() {
    Runtime& rt = RT();
    rt.enemies.clear();
    rt.waves.clear();
    rt.objectives.clear();
    rt.state = EncounterState::Inactive;
    rt.ep = nullptr;
    rt.player = nullptr;
    rt.lockIdx = -1;
}

void Encounter::update(float dt) {
    Runtime& rt = RT();
    if (rt.state == EncounterState::Inactive || rt.player == nullptr) return;
    if (!(dt > 0.0f)) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;

    rt.stateT += dt;
    rt.cpFlash = std::max(0.0f, rt.cpFlash - dt * 0.45f);

    Character& pl = *rt.player;

    switch (rt.state) {

    case EncounterState::Briefing:
        if (rt.stateT > 2.5f) { rt.state = EncounterState::Fighting; rt.stateT = 0.0f; }
        break;

    case EncounterState::Fighting: {
        rt.result.timeSec += dt;
        rt.phaseT += dt;
        rt.result.bestHealth = std::min(rt.result.bestHealth, pl.vitals.health);

        // --- dushmanlar ---
        rt.enemies.update(pl, dt);

        // --- o'yinchi zarbasi ---
        Attack a;
        if (pl.consumeAttack(a)) {
            // Yoy tovushi zarba TEKKANIDAN qat'i nazar eshitiladi — havoga urgan
            // zarba ham og'irlik his qildirsin
            if (a.type != DamageType::Assassinate) Sfx::get().play(SfxId::Swing, 0.0f, 0.55f);
            const int vi = pl.execVictim();
            std::vector<Enemy>& all = rt.enemies.all();
            if (a.type == DamageType::Assassinate && vi >= 0 && vi < (int)all.size()
                && all[(size_t)vi].alive()) {
                // YAKUNLOVCHI ZARBA: aniq qurbon, qat'iy o'lim. Ilgari zarba
                // playerAttack konusidan o'tardi va dushmanning invulnT oynasi
                // 999 zararni jimgina "Dodged" qilib yuborishi mumkin edi.
                HitResult hr;
                if (rt.enemies.deathblow(vi, hr)) {
                    ++rt.result.kills;
                    ++rt.assassinations;
                    kick(rt, 0.60f);
                    Sfx::get().play(SfxId::Kill, distanceXZ(hr.point, pl.position()), 1.15f);
                    // FaithEvent::Finisher ni QAYTA BERMAYMIZ — Character::playExecute()
                    // uni zarba boshlanganda allaqachon bergan.
                    rt.hitStopT = 0.100f;
                    pushSpark(rt, hr.point, 2);
                }
            } else {
                bool killed = false;
                HitResult hr = rt.enemies.playerAttack(pl.position(), pl.yaw(), a, killed);
                if (killed) {
                    ++rt.result.kills;
                    pl.faith.event(a.type == DamageType::Assassinate
                                   ? FaithEvent::Assassinate : FaithEvent::Kill);
                }
                // Ilgari natija `(void)hr;` bilan TASHLANARDI — ya'ni zarba
                // tekkani hech qayerga bildirilmasdi va "urishi" bo'sh his qilinardi.
                const float hd = distanceXZ(hr.point, pl.position());
                switch (hr.outcome) {
                    case HitOutcome::Killed:
                        rt.hitStopT = 0.100f; pushSpark(rt, hr.point, 2); kick(rt, 0.45f);
                        Sfx::get().play(SfxId::Kill, hd);
                        break;
                    case HitOutcome::Hit:
                        rt.hitStopT = 0.055f; pushSpark(rt, hr.point, 0); kick(rt, 0.28f);
                        Sfx::get().play(SfxId::Hit, hd);
                        break;
                    case HitOutcome::Blocked:
                        rt.hitStopT = 0.035f; pushSpark(rt, hr.point, 1); kick(rt, 0.18f);
                        Sfx::get().play(SfxId::Block, hd);
                        break;
                    default: break;      // Dodged / Parried / Miss — qaytarma yo'q
                }
            }
        }

        // --- o'yinchi kamoni ---
        {
            BowShot sh;
            if (pl.consumeShot(sh)) {
                // Character faqat O'Z shovqinini biladi; ogoh dushman bo'lsa
                // otish baribir sezilgan hisoblanadi.
                sh.silent = sh.silent && (rt.enemies.awareCount() == 0);
                rt.arrows.spawn(sh);
                Sfx::get().play(SfxId::BowShot, 0.0f, 0.85f);
            }
            static std::vector<ArrowHit> hits;      // qayta ishlatiladigan bufer
            hits.clear();
            if (rt.phys != nullptr) rt.arrows.update(dt, *rt.phys, rt.enemies.all(), hits);
            for (size_t i = 0; i < hits.size(); ++i) resolveArrowHit(rt, pl, hits[i]);
            // --- o'q yig'ish: yerga/jasad yoniga tushgan o'q ustidan o'tsa sadoqqa qaytadi ---
            if (pl.arrows() < pl.arrowsMax()) {
                const int got = rt.arrows.collect(pl.position(), 1.15f);
                if (got > 0) {
                    pl.addArrows(got);
                    rt.pickupT = 0.9f;
                    Sfx::get().play(SfxId::Pickup, 0.0f, 0.9f);
                }
            }
        }

        // --- dushman zarbalari ---
        for (const EnemyManager::IncomingHit& h : rt.enemies.incoming()) {
            if (h.attack == nullptr) continue;
            HitResult r = pl.receiveHit(*h.attack, h.from);
            if (r.outcome == HitOutcome::Parried && h.src != nullptr)
                h.src->vitals.posture += 35.0f;      // parry raqibni ochadi
            if (r.outcome == HitOutcome::Parried) {
                rt.hitStopT = 0.130f; pushSpark(rt, r.point, 1); kick(rt, 0.40f);
                Sfx::get().play(SfxId::Parry, 0.0f, 1.1f);
            }
            if (r.outcome == HitOutcome::Blocked) {
                rt.hitStopT = 0.040f; pushSpark(rt, r.point, 1); kick(rt, 0.30f);
                Sfx::get().play(SfxId::Block, 0.0f);
            }
            if (r.outcome == HitOutcome::Hit) {
                rt.hitStopT = 0.070f;
                pushSpark(rt, r.point, 0);
                // O'zi zarba YEGANDA silkinish kuchliroq — bu jazо
                kick(rt, 0.65f);
                Sfx::get().play(SfxId::Hit, 0.0f, 1.15f);
                // Iymon "yig'iladigan ochko" emas: zarba yesangiz tushadi
                pl.faith.event(FaithEvent::TookHit);
                rt.waveFlawless = false;
            }
        }

        // --- shovqin ---
        rt.noiseT += dt;
        if (rt.noiseT > 0.25f) {
            rt.noiseT = 0.0f;
            const float n = pl.lastNoise();
            if (n > 0.30f) {
                const NoiseEvent& ev = pl.noiseEvent();
                const float radius = ev.radius > 0.5f ? ev.radius : (6.0f + n * 14.0f);
                rt.enemies.broadcastNoise(ev.radius > 0.5f ? ev.pos : pl.position(), radius, n);
            }
        }

        // --- maqsadlar ---
        const int alive = rt.enemies.aliveCount();
        const int dead  = rt.enemies.total() - alive;
        for (Objective& o : rt.objectives) {
            if (o.done) continue;
            switch (o.kind) {
            case ObjectiveKind::DefeatAll:
                o.progress = dead;
                if (alive == 0 && rt.enemies.total() > 0) o.done = true;
                break;
            case ObjectiveKind::DefeatCount:
                o.progress = rt.result.kills;
                if (o.progress >= o.target) o.done = true;
                break;
            case ObjectiveKind::ReachPoint:
                if (distanceXZ(pl.position(), o.point) < o.radius) o.done = true;
                break;
            case ObjectiveKind::SurviveTime:
                o.progress = (int)rt.result.timeSec;
                if (o.progress >= o.target) o.done = true;
                break;
            case ObjectiveKind::StayUndetected:
                if (!o.failed && rt.enemies.awareCount() > 0) {
                    o.failed = true;
                    rt.result.undetected = false;
                    // Sezildingiz — qo'shimcha kuch keladi (bir marta)
                    Phase& P = rt.phases[(size_t)rt.phase];
                    if (!P.reinforced && !P.reinforce.spawns.empty()) {
                        P.reinforced = true;
                        spawnExtra(rt, P.reinforce);
                        for (Objective& q : rt.objectives)
                            if (q.kind == ObjectiveKind::DefeatAll) q.target = rt.enemies.total();
                    }
                }
                break;
            case ObjectiveKind::Route: {
                if (o.progress < (int)o.points.size()) {
                    const Vec3& tp = o.points[(size_t)o.progress];
                    const bool isNear = distanceXZ(pl.position(), tp) < o.radius;
                    const bool isUp   = !o.needY || std::fabs(pl.position().y - tp.y) < 1.4f
                                      || tp.y - pl.position().y < 0.6f;   // yer nuqtasi
                    if (isNear && isUp) { ++o.progress; pl.faith.event(FaithEvent::FlawlessWave); }
                }
                if (o.progress >= (int)o.points.size()) o.done = true;
                break;
            }
            case ObjectiveKind::Collect: {
                for (size_t k = 0; k < o.points.size(); ++k) {
                    if (distanceXZ(pl.position(), o.points[k]) < o.radius) {
                        o.points.erase(o.points.begin() + (long)k);
                        ++o.progress;
                        Sfx::get().play(SfxId::Pickup, 0.0f, 1.0f);
                        break;
                    }
                }
                if (o.progress >= o.target) o.done = true;
                break;
            }
            case ObjectiveKind::Hunt: {
                int n = 0;
                for (const Enemy& e : rt.enemies.all())
                    if (e.kind() == EnemyKind::Deer && !e.alive()) ++n;
                o.progress = n;
                if (o.progress >= o.target) o.done = true;
                break;
            }
            case ObjectiveKind::HoldPoint:
                if (distanceXZ(pl.position(), o.point) < o.radius) o.hold += dt;
                o.progress = (int)o.hold;
                if (o.progress >= o.target) o.done = true;
                break;
            case ObjectiveKind::Timer:
                o.progress = (int)rt.phaseT;
                if (o.progress > o.target) o.failed = true;
                break;
            default: break;
            }
        }

        // --- o'yinchi halok bo'ldimi ---
        if (pl.dead() || pl.vitals.health <= 0.0f) {
            ++rt.result.deaths;
            // Halok bo'lish Iymonni tushiradi - qayta urinish oson bo'lmaydi
            pl.faith.event(FaithEvent::Death);
            Progress::get().setFaith(pl.faith.value);
            rt.state = EncounterState::Failed;
            rt.stateT = 0.0f;
            break;
        }

        // --- to'lqin tozalandimi ---
        if (alive == 0 && rt.enemies.total() > 0 && rt.wave + 1 < (int)rt.waves.size()) {
            ++rt.result.wavesDone;
            // Zarba yemasdan tugatilgan to'lqin - Iymonning eng katta manbai
            if (rt.waveFlawless) pl.faith.event(FaithEvent::FlawlessWave);
            rt.waveFlawless = true;
            pl.addArrows(4);                 // to'lqin oralig'ida sadoq to'ldiriladi
            rt.state = EncounterState::WaveCleared;
            rt.stateT = 0.0f;
            break;
        }

        // --- bosqich tugadimi: majburiy maqsadlar bajarilgan va dushman qolmagan ---
        {
            bool allDone = true;
            for (const Objective& o : rt.objectives)
                if (!o.optional && !o.done) { allDone = false; break; }
            const bool wavesDone = rt.waves.empty() || (rt.wave + 1 >= (int)rt.waves.size() && alive == 0);
            if (allDone && wavesDone) {
                if (!rt.waves.empty()) { ++rt.result.wavesDone; if (rt.waveFlawless) pl.faith.event(FaithEvent::FlawlessWave); }
                pl.addArrows(4);
                if (rt.phase + 1 < (int)rt.phases.size()) {
                    startPhase(rt, rt.phase + 1);
                    rt.state = EncounterState::Fighting;   // brifingsiz — oqim uzilmasin
                    rt.stateT = 0.0f;
                } else {
                    pl.faith.event(FaithEvent::EpisodeDone);
                    Progress::get().setFaith(pl.faith.value);
                    rt.state = EncounterState::Cleared;
                    rt.stateT = 0.0f;
                }
            }
        }
        break;
    }

    case EncounterState::WaveCleared: {
        rt.enemies.update(pl, dt);
        if (rt.stateT < 1.5f) break;
        // Keyingi to'lqin (bosqich tugashi Fighting ichida tekshiriladi)
        ++rt.wave;
        spawnWave(rt, rt.wave);
        for (Objective& o : rt.objectives)
            if (o.kind == ObjectiveKind::DefeatAll && !o.done) {
                o.target = rt.enemies.total(); o.progress = 0;
            }
        saveCheckpoint(rt);
        rt.state = EncounterState::Fighting;
        rt.stateT = 0.0f;
        break;
    }

    default:
        break;
    }

    // Qulflangan nishon tirikligini tekshiramiz
    if (rt.lockIdx >= 0) {
        std::vector<Enemy>& all = rt.enemies.all();
        if (rt.lockIdx >= (int)all.size() || !all[(size_t)rt.lockIdx].alive())
            rt.lockIdx = -1;
    }
}

void Encounter::restartFromCheckpoint() {
    Runtime& rt = RT();
    if (!rt.cp.valid || rt.player == nullptr) { restartEpisode(); return; }

    rt.phase      = rt.cp.phase;
    if (rt.phase >= 0 && rt.phase < (int)rt.phases.size()) {
        rt.waves      = rt.phases[(size_t)rt.phase].waves;
        rt.phaseTitle = Loc::get().trOr(rt.phases[(size_t)rt.phase].titleKey, "");
    }
    rt.wave       = rt.cp.wave;
    rt.phaseT     = 0.0f;
    rt.objectives = rt.cp.objectives;
    const int deaths = rt.result.deaths;
    rt.result       = rt.cp.result;
    rt.result.deaths = deaths;            // o'limlar hisobi saqlanadi

    rt.player->revive(rt.cp.playerPos, rt.cp.playerYaw);
    spawnWave(rt, rt.wave);
    for (Objective& o : rt.objectives)
        if (o.kind == ObjectiveKind::DefeatAll && !o.done) {
            o.target = rt.enemies.total();
            o.progress = 0;
        }
    rt.state = EncounterState::Briefing;
    rt.stateT = 1.3f;                     // qisqaroq brifing
    std::printf("[Jang] nazorat nuqtasidan qayta boshlandi: %s (to'lqin %d)\n",
                rt.cpName.c_str(), rt.wave + 1);
}

void Encounter::restartEpisode() {
    Runtime& rt = RT();
    if (rt.ep == nullptr || rt.level == nullptr || rt.phys == nullptr || rt.player == nullptr) return;
    const int deaths = rt.result.deaths;
    const Episode* ep = rt.ep;
    Level* lv = rt.level;
    PhysicsWorld* ph = rt.phys;
    Character* pl = rt.player;

    Vec3 sp = pl->position();
    float syaw = pl->yaw();
    if (const SpawnPoint* s = lv->spawn("player")) { sp = s->pos; syaw = s->yaw; }
    sp.y = lv->groundAt(sp.x, sp.z);
    pl->revive(sp, syaw);

    begin(*ep, *lv, *ph, *pl);
    RT().result.deaths = deaths;
}

void Encounter::tickFeedback(float realDt) {
    { Runtime& q = RT(); q.pickupT = (q.pickupT > realDt) ? q.pickupT - realDt : 0.0f; }
    Runtime& rt = RT();
    if (!(realDt > 0.0f)) return;
    if (realDt > 0.1f) realDt = 0.1f;
    if (rt.hitStopT > 0.0f) { rt.hitStopT -= realDt; if (rt.hitStopT < 0.0f) rt.hitStopT = 0.0f; }
    // Silkinish tez so'nadi (halflife ~58 ms) — "titroq" emas, "turtki" bo'lib sezilsin
    rt.shakeT += realDt;
    if (rt.shakeT > 1000.0f) rt.shakeT -= 1000.0f;
    if (rt.shakeAmp > 0.0f) {
        rt.shakeAmp = damp(rt.shakeAmp, 0.0f, 12.0f, realDt);
        if (rt.shakeAmp < 0.004f) rt.shakeAmp = 0.0f;
    }
    for (size_t i = 0; i < rt.sparks.size(); ) {
        rt.sparks[i].t += realDt;
        if (rt.sparks[i].t >= 0.22f) rt.sparks.erase(rt.sparks.begin() + (long)i);
        else ++i;
    }
}

// Zarba tekkanda o'yin qisqa muddatga muzlaydi — zarbaning OG'IRLIGI shundan sezildi.
float Encounter::timeScale() const { return (RT().hitStopT > 0.0f) ? 0.12f : 1.0f; }
float Encounter::pickupFlash() const { return clampf(RT().pickupT / 0.9f, 0.0f, 1.0f); }

float Encounter::cameraShake() const {
    const Runtime& rt = RT();
    return clampf(rt.shakeAmp, 0.0f, 1.0f);
}

EncounterState Encounter::state() const { return RT().state; }
const std::vector<Objective>& Encounter::objectives() const { return RT().objectives; }
const EncounterResult& Encounter::result() const { return RT().result; }
EnemyManager& Encounter::enemies() { return RT().enemies; }
const EnemyManager& Encounter::enemies() const { return RT().enemies; }
int   Encounter::waveIndex() const { return RT().wave; }
int   Encounter::waveCount() const { return (int)RT().waves.size(); }
int   Encounter::aliveEnemies() const { return RT().enemies.aliveCount(); }
float Encounter::stateTime() const { return RT().stateT; }
const std::string& Encounter::episodeId() const { return RT().episodeId; }
const std::string& Encounter::checkpointName() const { return RT().cpName; }
int   Encounter::phaseIndex() const { return RT().phase; }
int   Encounter::phaseCount() const { return (int)RT().phases.size(); }
const std::string& Encounter::phaseTitle() const { return RT().phaseTitle; }
float Encounter::checkpointFlash() const { return RT().cpFlash; }

Enemy* Encounter::lockTarget() const {
    Runtime& rt = RT();
    std::vector<Enemy>& all = rt.enemies.all();
    if (rt.lockIdx < 0 || rt.lockIdx >= (int)all.size()) return nullptr;
    Enemy& e = all[(size_t)rt.lockIdx];
    return e.alive() ? &e : nullptr;
}

void Encounter::cycleLockTarget() {
    Runtime& rt = RT();
    if (rt.player == nullptr) return;
    std::vector<Enemy>& all = rt.enemies.all();
    if (all.empty()) { rt.lockIdx = -1; return; }

    const Vec3 pp = rt.player->position();
    // Joriy nishondan keyingi TIRIK va 22 m ichidagi dushmanni tanlaymiz
    const int n = (int)all.size();
    for (int k = 1; k <= n; ++k) {
        const int i = ((rt.lockIdx + k) % n + n) % n;
        if (!all[(size_t)i].alive()) continue;
        if (distanceXZ(all[(size_t)i].position(), pp) > 22.0f) continue;
        rt.lockIdx = i;
        return;
    }
    rt.lockIdx = -1;
}

void Encounter::clearLockTarget() { RT().lockIdx = -1; }

void Encounter::draw() {
    Runtime& rt = RT();
    if (rt.state == EncounterState::Inactive) return;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT |
                 GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_TEXTURE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    rt.arrows.draw();          // uchayotgan va qadalgan o'qlar

    // --- Maqsad markerlari: yerdagi halqa + yuqoriga ko'tarilgan ustun ---
    auto marker = [&](const Vec3& pt, float radius, const float* col, float h, bool onTop) {
        // onTop: nuqta tom/qoya USTIDA — halqa o'sha balandlikda
        const float gy = onTop ? pt.y : (rt.phys ? rt.phys->groundAt(pt.x, pt.z) : pt.y);
        glDisable(GL_DEPTH_TEST);
        glLineWidth(2.0f);
        glColor4f(col[0], col[1], col[2], 0.75f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 28; ++i) {
            const float a = TAU * (float)i / 28.0f;
            glVertex3f(pt.x + std::cos(a) * radius, gy + 0.06f, pt.z + std::sin(a) * radius);
        }
        glEnd();
        glBegin(GL_LINES);
        glColor4f(col[0], col[1], col[2], 0.55f);
        glVertex3f(pt.x, gy, pt.z);
        glColor4f(col[0], col[1], col[2], 0.0f);
        glVertex3f(pt.x, gy + h, pt.z);
        glEnd();
        glEnable(GL_DEPTH_TEST);
    };
    for (const Objective& o : rt.objectives) {
        if (o.done) continue;
        switch (o.kind) {
        case ObjectiveKind::ReachPoint:
            marker(o.point, o.radius, C_FERUZA, 7.0f, false);
            break;
        case ObjectiveKind::Route:
            if (o.progress < (int)o.points.size())
                marker(o.points[(size_t)o.progress], o.radius, C_FERUZA, 9.0f, o.needY);
            // keyingi nuqta xira ko'rsatiladi — yo'nalish sezilsin
            if (o.progress + 1 < (int)o.points.size())
                marker(o.points[(size_t)o.progress + 1], o.radius * 0.6f, C_MATN, 3.0f, o.needY);
            break;
        case ObjectiveKind::Collect:
            for (const Vec3& pt : o.points) marker(pt, o.radius, C_ZARHAL, 4.0f, false);
            break;
        case ObjectiveKind::HoldPoint: {
            const float pulse = 0.85f + 0.15f * std::sin(rt.result.timeSec * 4.0f);
            marker(o.point, o.radius * pulse, C_YARA, 5.0f, false);
            break;
        }
        default: break;
        }
    }

    // --- Dushman ogohlik belgilari («mix boshi» kvadrati) ---
    glDisable(GL_DEPTH_TEST);
    for (Enemy& e : rt.enemies.all()) {
        if (!e.alive()) continue;
        const Vec3 h = e.headPos() + Vec3{0.0f, 0.32f, 0.0f};
        if (e.vitals.staggered) {
            // Yakunlovchi zarba mumkin — oq pulsatsiya
            const float p = 0.55f + 0.45f * std::sin(rt.result.timeSec * 9.0f);
            billboardSquare(h, 0.30f, C_MATN, p);
        } else if (e.alertness() >= 0.999f) {
            billboardSquare(h, 0.24f, C_YARA, 0.92f);
        } else if (e.alertness() > 0.35f) {
            // to'lib boruvchi shubha
            billboardSquare(h, 0.16f + 0.10f * e.alertness(), C_ZARHAL,
                            0.35f + 0.55f * e.alertness());
        }
    }
    glEnable(GL_DEPTH_TEST);

    glDepthMask(GL_TRUE);
    glPopAttrib();
}

// ===========================================================================
// Progress — saves/progress.json
// ===========================================================================

namespace {

struct ProgData {
    std::vector<std::string> completed;
    int kills = 0, deaths = 0;
    float faith = 50.0f;              // saqlanadigan Iymon
    std::string last;
    bool loaded = false;
};

ProgData& PD() { static ProgData p; return p; }

bool listHas(const std::vector<std::string>& v, const std::string& s) {
    for (const std::string& x : v) if (x == s) return true;
    return false;
}

} // namespace

Progress& Progress::get() { static Progress p; return p; }

bool Progress::load(const std::string& path) {
    ProgData& d = PD();
    d.loaded = true;
    try {
        std::ifstream f(path);
        if (!f) return false;
        nlohmann::json j;
        f >> j;
        d.completed.clear();
        if (j.contains("faith") && j["faith"].is_number())
            d.faith = clampf((float)j["faith"].get<double>(), 0.0f, 100.0f);
        if (j.contains("completed") && j["completed"].is_array())
            for (const auto& e : j["completed"])
                if (e.is_string()) d.completed.push_back(e.get<std::string>());
        d.kills  = j.value("kills", 0);
        d.deaths = j.value("deaths", 0);
        d.last   = j.value("last", std::string());
        return true;
    } catch (...) { return false; }
}

bool Progress::save(const std::string& path) const {
    const ProgData& d = PD();
    try {
        nlohmann::json j;
        j["completed"] = d.completed;
        j["kills"]     = d.kills;
        j["deaths"]    = d.deaths;
        j["last"]      = d.last;
        j["faith"]     = d.faith;
        CreateDirectoryA("saves", nullptr);
        std::ofstream f(path);
        if (!f) return false;
        f << j.dump(2);
        return true;
    } catch (...) { return false; }
}

bool Progress::completed(const std::string& episodeId) const {
    return listHas(PD().completed, episodeId);
}

bool Progress::unlocked(const std::string& episodeId) const {
    const Episode* e = EpisodeDb::get().byId(episodeId);
    if (e == nullptr) return true;
    if (e->globalIndex <= 0) return true;                  // birinchisi doim ochiq
    if (completed(episodeId)) return true;
    const Episode* prev = EpisodeDb::get().byIndex((size_t)e->globalIndex - 1);
    return prev == nullptr ? true : completed(prev->id);
}

void Progress::markCompleted(const std::string& episodeId, const EncounterResult& r) {
    ProgData& d = PD();
    if (!listHas(d.completed, episodeId)) d.completed.push_back(episodeId);
    d.kills  += r.kills;
    d.deaths += r.deaths;
    d.last    = episodeId;
}

float Progress::faith() const { return PD().faith; }

void Progress::setFaith(float v) {
    PD().faith = clampf(std::isfinite(v) ? v : 50.0f, 0.0f, 100.0f);
}

void Progress::addDeath(const std::string& episodeId) {
    ProgData& d = PD();
    ++d.deaths;
    d.last = episodeId;
}

int Progress::totalKills() const     { return PD().kills; }
int Progress::totalDeaths() const    { return PD().deaths; }
int Progress::completedCount() const { return (int)PD().completed.size(); }
const std::string& Progress::lastEpisode() const { return PD().last; }
void Progress::setLastEpisode(const std::string& id) { PD().last = id; }

void Progress::reset() {
    ProgData& d = PD();
    d.completed.clear();
    d.kills = d.deaths = 0;
    d.last.clear();
}

} // namespace ert
