// Tizim testlari — tashqi test kutubxonasisiz.
// Ishga tushirish: build/ertugrul_tests.exe   (0 = hammasi o'tdi)
#include <cmath>
#include <iostream>
#include <string>

#include "ertugrul/characters/Characters.h"
#include "ertugrul/components/Combat.h"
#include "ertugrul/components/Stealth.h"
#include "ertugrul/components/Vitals.h"
#include "ertugrul/core/Json.h"
#include "ertugrul/core/Log.h"
#include "ertugrul/minigames/Minigames.h"
#include "ertugrul/subsystems/Subsystems.h"
#include "ertugrul/world/World.h"

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const std::string& name) {
    ++g_checks;
    if (condition) {
        std::cout << "  [OK]   " << name << "\n";
    } else {
        ++g_failures;
        std::cout << "  [FAIL] " << name << "\n";
    }
}

void checkNear(float actual, float expected, float tolerance, const std::string& name) {
    check(std::fabs(actual - expected) <= tolerance,
          name + " (kutilgan " + std::to_string(expected) + ", olingan " + std::to_string(actual) + ")");
}

void section(const std::string& title) { std::cout << "\n" << title << "\n"; }

// ---------------------------------------------------------------- testlar

void testJson() {
    section("Json parser/serializer");
    const std::string text = R"({"id":"q1","episode":2,"objectives":[{"id":"a","optional":true}],"ok":false})";
    std::string error;
    const ert::Json json = ert::Json::parse(text, &error);
    check(error.empty(), "xatosiz o'qildi");
    check(json["id"].asString() == "q1", "string maydon");
    check(json["episode"].asInt() == 2, "son maydon");
    check(json["objectives"].size() == 1, "massiv o'lchami");
    check(json["objectives"][0]["optional"].asBool(), "ichma-ich obyekt");
    check(json["ok"].asBool(true) == false, "false qiymat");
    check(json["yoq"].isNull(), "mavjud bo'lmagan kalit Null");

    ert::Json out = ert::Json::object();
    out.set("honor", 42);
    out.set("name", "Ertugrul");
    const ert::Json roundTrip = ert::Json::parse(out.dump());
    check(roundTrip["honor"].asInt() == 42 && roundTrip["name"].asString() == "Ertugrul",
          "dump -> parse aylanishi");
}

void testHealthAndArmor() {
    section("Sog'lik va zirh (UHealthComponent)");
    ert::World world;
    ert::Actor* actor = world.spawn<ert::Actor>("dummy", "Sinov");
    world.tick(0.0f);
    auto* health = actor->addComponent<ert::HealthComponent>(100.0f);
    health->armor = 0.5f;

    health->takeDamage(40.0f, ert::DamageType::Slash, nullptr);
    checkNear(health->health, 80.0f, 0.01f, "zirh zararni yarmiga kamaytiradi");

    health->invulnerable = true;
    health->takeDamage(1000.0f, ert::DamageType::Slash, nullptr);
    checkNear(health->health, 80.0f, 0.01f, "invulnerable zararni to'sadi");
    health->invulnerable = false;

    health->takeDamage(10.0f, ert::DamageType::Assassination, nullptr);
    check(health->isDead(), "yashirin o'ldirish bir zarbada o'ldiradi");

    health->revive(0.5f);
    check(!health->isDead() && std::fabs(health->health - 50.0f) < 0.01f, "revive tiklaydi");
}

void testStaminaGate() {
    section("Chidam: spam jazolanadi (GDD 1.2)");
    ert::World world;
    ert::Actor* actor = world.spawn<ert::Actor>("dummy", "Sinov");
    world.tick(0.0f);
    auto* stamina = actor->addComponent<ert::StaminaComponent>(30.0f);
    check(stamina->consume(24.0f), "birinchi og'ir zarba o'tadi");
    check(!stamina->consume(24.0f), "chidam yetmasa ikkinchisi bloklanadi");
    for (int i = 0; i < 120; ++i) stamina->tick(1.0f / 30.0f);
    check(stamina->consume(24.0f), "tiklanishdan keyin yana o'tadi");
}

void testParryAndShield() {
    section("Parry, stagger va qalqon (GDD 2.3)");
    ert::World world;
    ert::Actor* attacker = world.spawn<ert::Actor>("attacker", "Ritsar");
    ert::Actor* defender = world.spawn<ert::Actor>("defender", "Ertugrul");
    world.tick(0.0f);

    attacker->addComponent<ert::HealthComponent>(100.0f);
    auto* attackerMelee = attacker->addComponent<ert::MeleeCombatComponent>();
    auto* shield = attacker->addComponent<ert::ShieldComponent>(3);

    defender->addComponent<ert::HealthComponent>(100.0f);
    auto* defenderMelee = defender->addComponent<ert::MeleeCombatComponent>();

    check(defenderMelee->requestParry(), "parry oynasi ochiladi");
    check(defenderMelee->tryDefend(attacker), "aniq vaqtdagi parry zarbani qaytaradi");
    check(attackerMelee->isVulnerable(), "parry raqibni ochadi (stagger)");
    check(shield->durability == 2, "parry qalqon mustahkamligini kamaytiradi");

    shield->onParriedByPlayer();
    shield->onParriedByPlayer();
    check(shield->broken, "uch parry qalqonni sindiradi");

    // Parry oynasidan tashqarida himoya ishlamaydi
    for (int i = 0; i < 30; ++i) defenderMelee->tick(1.0f / 30.0f);
    check(!defenderMelee->tryDefend(attacker), "kech parry ishlamaydi");
}

void testInjuryPhases() {
    section("Yarador holat: mix -> bog'langan qilich (GDD 4.2 / 5.1)");
    ert::World world;
    ert::Actor* actor = world.spawn<ert::Actor>("ertugrul", "Ertugrul");
    world.tick(0.0f);
    actor->addComponent<ert::HealthComponent>(140.0f);
    actor->addComponent<ert::BleedingComponent>();
    auto* injury = actor->addComponent<ert::InjuryStateComponent>();
    auto* melee = actor->addComponent<ert::MeleeCombatComponent>();
    actor->addComponent<ert::FaithComponent>();
    auto* binding = actor->addComponent<ert::BindingSwordComponent>();

    check(melee->requestLight(), "sog' holatda qilich ishlaydi");
    for (int i = 0; i < 40; ++i) melee->tick(1.0f / 30.0f);

    injury->applyNailedHand();
    check(!injury->canUseSword && !injury->canParry, "mixlangan qo'l qilich va parryni bloklaydi");
    check(!injury->canClimb, "bir qo'l bilan devorga chiqib bo'lmaydi");
    check(!melee->requestLight(), "yarador holatda yengil zarba bloklanadi");
    checkNear(injury->damageMultiplier, 0.55f, 0.001f, "zarar koeffitsienti pasayadi");

    binding->bindSword(0.95f);
    check(injury->phase == ert::InjuryPhase::BoundSword, "qilich qo'lga bog'landi");
    check(injury->canParry && injury->canUseSword, "parry va qilich qaytadi");
    checkNear(binding->damageBonus(), 0.15f, 0.001f, "yuqori sifat +15% bonus beradi");
    checkNear(injury->parryRecoveryTime(), ert::balance::kParryStunAfterInjury, 0.001f,
              "parrydan keyin stun jazosi bor");
    check(ert::GameState::get().hasAbility("rage_strike"), "G'azab zarbasi ochiladi");
}

void testStealthVisibility() {
    section("Stealth: yorug'lik, cho'kkalash, berkinish (GDD 2.4)");
    ert::World world;
    world.hidingSpots.push_back({{10.0f, 0.0f, 0.0f}, 3.0f, 0.1f, "bush"});
    ert::Actor* actor = world.spawn<ert::Actor>("player", "Ertugrul");
    actor->addTag("player");
    world.tick(0.0f);
    auto* stealth = actor->addComponent<ert::StealthComponent>();

    ert::WorldClock::get().forceTime(12.0f, true);
    stealth->tick(0.1f);
    const float dayVisibility = stealth->visibility;

    ert::WorldClock::get().forceTime(23.0f, true);
    stealth->tick(0.1f);
    const float nightVisibility = stealth->visibility;
    check(nightVisibility < dayVisibility, "tunda ko'rinuvchanlik kamayadi");

    stealth->crouching = true;
    stealth->tick(0.1f);
    check(stealth->visibility < nightVisibility, "cho'kkalash ko'rinuvchanlikni kamaytiradi");

    actor->position = {10.0f, 0.0f, 0.0f};
    stealth->tick(0.1f);
    check(stealth->hidingKind == "bush", "berkinish joyi aniqlanadi");
    ert::WorldClock::get().unlock();
}

void testDetectionCone() {
    section("Dushman sezgisi: ko'rish konusi va to'siq");
    ert::World world;
    ert::Actor* player = world.spawn<ert::Actor>("player", "Ertugrul");
    player->addTag("player");
    player->position = {0.0f, 0.0f, 10.0f};
    player->addComponent<ert::StealthComponent>();

    ert::Actor* guard = world.spawn<ert::Actor>("guard", "Soqchi");
    guard->addTag("enemy");
    guard->position = {0.0f, 0.0f, 0.0f};
    guard->yaw = 0.0f;   // +Z ga qaraydi
    auto* detection = guard->addComponent<ert::DetectionComponent>();
    world.tick(0.0f);

    check(detection->canSee(*player), "oldindagi o'yinchi ko'rinadi");

    guard->yaw = 3.14159f;   // orqaga qaradi
    check(!detection->canSee(*player), "orqadagi o'yinchi ko'rinmaydi");

    guard->yaw = 0.0f;
    world.blockers.push_back({{0.0f, 1.0f, 5.0f}, {3.0f, 2.0f, 1.0f}});
    check(!detection->canSee(*player), "to'siq ko'rish chizig'ini bloklaydi");

    detection->hearNoise({0.0f, 0.0f, 6.0f}, 12.0f);
    check(detection->suspicion >= 0.5f, "shovqin shubhani oshiradi");
}

void testQuestChain() {
    section("Kvest zanjiri va mukofotlar (GDD IV)");
    ert::QuestManager& quests = ert::QuestManager::get();
    const int loaded = quests.loadFromDirectory("data/quests");
    check(loaded == 17, "17 kvest yuklandi");

    quests.reset();
    check(quests.start("q1_1_deer_hunt"), "kvest boshlandi");
    check(quests.isActive("q1_1_deer_hunt"), "kvest faol");

    const ert::QuestManager::Quest* quest = quests.quest("q1_1_deer_hunt");
    check(quest != nullptr && quest->next == "q1_2_forest_ambush", "keyingi kvest bog'langan");

    const int honorBefore = ert::GameState::get().honor;
    const int meatBefore = ert::ObaManager::get().resource("meat");
    for (const auto& objective : quest->objectives) {
        quests.completeObjective("q1_1_deer_hunt", objective.id);
    }
    check(quests.isCompleted("q1_1_deer_hunt"), "barcha maqsadlar -> kvest yakunlandi");
    check(ert::GameState::get().honor > honorBefore, "sharaf mukofoti berildi");
    check(ert::ObaManager::get().resource("meat") == meatBefore + 12, "resurs mukofoti berildi");
    check(quests.isActive("q1_2_forest_ambush"), "keyingi kvest avtomatik boshlandi");
}

void testDialogueDuel() {
    section("Dialog-duel: dalil talab qiladi (GDD 2.3)");
    ert::DialogueSystem& dialogue = ert::DialogueSystem::get();
    check(dialogue.loadFromDirectory("data/dialogue") >= 2, "dialoglar yuklandi");
    check(dialogue.start("council_kurdoglu"), "kengash dialogi boshlandi");

    dialogue.advance();   // opening -> accusation
    const std::size_t withoutEvidence = dialogue.visibleOptions().size();

    ert::GameState::get().setFlag("evidence_letter", true);
    ert::GameState::get().setFlag("evidence_seal", true);
    const std::size_t withEvidence = dialogue.visibleOptions().size();
    check(withEvidence > withoutEvidence, "dalil yangi javob variantini ochadi");

    const int scoreBefore = dialogue.duelScore();
    dialogue.choose(0);
    check(dialogue.duelScore() > scoreBefore, "dalil duel ochkosini oshiradi");
}

void testBlacksmithQuality() {
    section("Temirchilik ritmi -> qilich sifati (GDD 5.1)");
    ert::BlacksmithMinigame minigame;
    minigame.totalBeats = 16;
    minigame.start();

    constexpr float step = 1.0f / 120.0f;
    int guard = 0;
    while (minigame.running() && guard++ < 20000) {
        if (minigame.timeToNextBeat() <= step) minigame.hit();
        minigame.tick(step);
    }
    check(minigame.perfectCount() > 0, "ritmga urish perfect beradi");
    check(minigame.quality() > 0.8f, "aniq ritm yuqori sifat beradi");

    ert::BlacksmithMinigame sloppy;
    sloppy.totalBeats = 16;
    sloppy.start();
    guard = 0;
    while (sloppy.running() && guard++ < 20000) sloppy.tick(step);   // hech qachon urmaydi
    check(sloppy.missCount() > 0, "urilmagan zarbalar miss hisoblanadi");
    check(sloppy.quality() < 0.2f, "ritmsiz o'yin past sifat beradi");
}

void testSaveRoundTrip() {
    section("Saqlash va yuklash");
    ert::GameState::get().honor = 77;
    ert::GameState::get().setFlag("kurdoglu_exposed", true);
    ert::GameState::get().unlockAbility("binding_sword");
    ert::ObaManager::get().addResource("iron", 9);

    check(ert::SaveSystem::get().save("saves/test_slot.json"), "saqlandi");

    ert::GameState::get().honor = 0;
    ert::GameState::get().flags.clear();
    ert::GameState::get().abilities.clear();

    check(ert::SaveSystem::get().load("saves/test_slot.json"), "yuklandi");
    check(ert::GameState::get().honor == 77, "sharaf tiklandi");
    check(ert::GameState::get().flag("kurdoglu_exposed"), "bayroq tiklandi");
    check(ert::GameState::get().hasAbility("binding_sword"), "qobiliyat tiklandi");
    check(ert::ObaManager::get().resource("iron") >= 9, "oba resursi tiklandi");
}

void testLocalization() {
    section("Lokalizatsiya: uz / tr / en (GDD VIII)");
    ert::LocaleManager& locale = ert::LocaleManager::get();
    check(locale.loadCsv("localization/ertugrul_loc.csv") > 100, "CSV yuklandi");
    check(locale.availableLocales().size() == 3, "uchta til mavjud");

    locale.setLocale("uz");
    const std::string uz = locale.tr("Q1_1_TITLE");
    locale.setLocale("en");
    const std::string en = locale.tr("Q1_1_TITLE");
    check(uz != en && !uz.empty() && !en.empty(), "til almashishi matnni o'zgartiradi");
    check(locale.tr("YOQ_KALIT") == "YOQ_KALIT", "topilmagan kalit o'zi qaytadi");
}

void testEnemyArchetypes() {
    section("Dushman turlari data/enemies dan (GDD VII)");
    ert::ContentDatabase& content = ert::ContentDatabase::get();
    check(content.loadEnemies("data/enemies") >= 20, "dushman turlari yuklandi");
    check(!content.enemy("templar_knight").isNull(), "templar_knight mavjud");
    check(content.enemy("bayju_noyan")["phases"].size() == 3, "Noyanda 3 faza bor");

    ert::World world;
    ert::EnemyCharacter* knight =
        ert::spawnEnemy(world, "k1", "templar_knight", {0.0f, 0.0f, 0.0f}, ert::Faction::Templar);
    world.tick(0.0f);
    check(knight->health->maxHealth == 110.0f, "arxetip sog'lig'i qo'llandi");
    check(knight->get<ert::ShieldComponent>() != nullptr, "qalqonli dushman qalqon oladi");

    ert::BossCharacter* noyan =
        ert::spawnBoss(world, "noyan", "bayju_noyan", {0.0f, 0.0f, 0.0f}, ert::Faction::Mongol);
    world.tick(0.0f);
    check(noyan->survivesDefeat, "Noyan mag'lubiyatdan keyin tirik qoladi");
    check(noyan->phases->phases.size() == 3, "boss fazalari yuklandi");
}

void testCombatDirectorTokens() {
    section("Olomon jangi navbati (GDD 2.3)");
    ert::CombatDirector& director = ert::CombatDirector::get();
    director.reset();
    ert::GameState::get().difficulty = ert::Difficulty::Alp;

    ert::World world;
    ert::Actor* a = world.spawn<ert::Actor>("a", "A");
    ert::Actor* b = world.spawn<ert::Actor>("b", "B");
    ert::Actor* c = world.spawn<ert::Actor>("c", "C");
    world.tick(0.0f);

    check(director.requestAttackToken(a), "birinchi raqib hujum qiladi");
    check(director.requestAttackToken(b), "ikkinchi raqib hujum qiladi");
    check(!director.requestAttackToken(c), "uchinchisi navbat kutadi (Alp rejimi)");

    ert::GameState::get().difficulty = ert::Difficulty::Gazi;
    check(director.requestAttackToken(c), "Gozi rejimida hammasi bir vaqtda hujum qiladi");
    ert::GameState::get().difficulty = ert::Difficulty::Alp;
    director.reset();
}

void testMovementAxes() {
    section("Harakat o'qlari (kamera bilan mos)");
    // Kamera +Z ga qaraydi, up = +Y  =>  ekran o'ngi = -X (o'ng qo'l koordinata tizimi).
    // D bosilganda personaj ekranning o'ng tomoniga yurishi kerak.
    ert::World world;
    auto* player = world.spawn<ert::PlayerCharacter>();
    world.tick(0.0f);
    player->yaw = 0.0f;

    player->position = {0.0f, 0.0f, 0.0f};
    player->moveInput(1.0f, 0.0f, 0.1f);
    check(player->position.z > 0.05f, "W oldinga yuradi (+Z)");

    player->position = {0.0f, 0.0f, 0.0f};
    player->moveInput(-1.0f, 0.0f, 0.1f);
    check(player->position.z < -0.05f, "S orqaga yuradi (-Z)");

    player->position = {0.0f, 0.0f, 0.0f};
    player->moveInput(0.0f, 1.0f, 0.1f);
    check(player->position.x < -0.05f, "D ekran o'ngiga yuradi (-X)");

    player->position = {0.0f, 0.0f, 0.0f};
    player->moveInput(0.0f, -1.0f, 0.1f);
    check(player->position.x > 0.05f, "A ekran chapiga yuradi (+X)");

    // 90 daraja burilgach o'qlar ham buriladi
    player->yaw = 1.5707963f;            // +X ga qaraydi
    player->position = {0.0f, 0.0f, 0.0f};
    player->moveInput(1.0f, 0.0f, 0.1f);
    check(player->position.x > 0.05f, "burilgandan keyin W yangi yo'nalishga yuradi");
}

void testHonorTiers() {
    section("Sharaf darajalari (GDD 1.4)");
    check(ert::honorTier(-60) == ert::HonorTier::Dishonored, "-60 -> Dishonored");
    check(ert::honorTier(0) == ert::HonorTier::Wavering, "0 -> Wavering");
    check(ert::honorTier(40) == ert::HonorTier::Respected, "40 -> Respected");
    check(ert::honorTier(90) == ert::HonorTier::Honored, "90 -> Honored");
    check(ert::honorTier(150) == ert::HonorTier::Gazi, "150 -> Gazi");
}

}  // namespace

int main() {
    ert::Log::setLevel(ert::LogLevel::Error);   // testlarda log shovqinini kamaytiramiz
    std::cout << "Dirilis: Ertugrul — tizim testlari\n";

    testJson();
    testHealthAndArmor();
    testStaminaGate();
    testParryAndShield();
    testInjuryPhases();
    testStealthVisibility();
    testDetectionCone();
    testQuestChain();
    testDialogueDuel();
    testBlacksmithQuality();
    testSaveRoundTrip();
    testLocalization();
    testEnemyArchetypes();
    testCombatDirectorTokens();
    testMovementAxes();
    testHonorTiers();

    std::cout << "\n================================\n";
    std::cout << "Tekshiruvlar: " << g_checks << " | Xatolar: " << g_failures << "\n";
    std::cout << (g_failures == 0 ? "NATIJA: HAMMASI O'TDI\n" : "NATIJA: XATOLAR BOR\n");
    return g_failures == 0 ? 0 : 1;
}
