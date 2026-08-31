# 17 ta kvest JSON faylini generatsiya qiladi (GDD IV-V qismlari asosida).
# Ishlatish: python tools/gen_quests.py
import json, io, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
Q = os.path.join(ROOT, "data", "quests")
os.makedirs(Q, exist_ok=True)


def q(qid, ep, title, giver, qtype, objectives, rewards, nxt="", location="", systems=None, gdd=""):
    return {
        "id": qid, "episode": ep, "title_key": title, "giver": giver, "type": qtype,
        "location": location, "gdd_ref": gdd, "systems": systems or [],
        "objectives": objectives, "rewards": rewards, "next": nxt,
    }


def o(oid, key, optional=False):
    return {"id": oid, "text_key": key, "optional": optional}


quests = [
    q("q1_1_deer_hunt", 0, "Q1_1_TITLE", "hayme_ana", "main", [
        o("leave_oba", "Q1_1_OBJ_LEAVE"), o("call_horse", "Q1_1_OBJ_HORSE"),
        o("find_tracks", "Q1_1_OBJ_TRACKS"), o("bow_tutorial", "Q1_1_OBJ_BOW"),
        o("chase_deer", "Q1_1_OBJ_CHASE"), o("load_prey", "Q1_1_OBJ_LOAD"),
        o("hunt_wolves", "Q1_1_OBJ_WOLVES", True)],
      {"honor": 5, "skill_points": 1, "resources": {"meat": 12, "hide": 4}},
      "q1_2_forest_ambush", "forest_pine",
      ["UHorseComponent", "URangedComponent", "UTrackingSenseComponent"],
      "Epizod 1 / Kvest 1.1 - Kiyik ovi"),

    q("q1_2_forest_ambush", 0, "Q1_2_TITLE", "story", "main", [
        o("reach_screams", "Q1_2_OBJ_REACH"), o("dismount_attack", "Q1_2_OBJ_DISMOUNT"),
        o("parry_tutorial", "Q1_2_OBJ_PARRY"), o("dodge_tutorial", "Q1_2_OBJ_DODGE"),
        o("defeat_bisol", "Q1_2_OBJ_BISOL"), o("protect_strangers", "Q1_2_OBJ_PROTECT")],
      {"honor": 8, "skill_points": 1},
      "q1_3_first_blood", "forest_clearing",
      ["UMeleeCombatComponent", "UTargetLockComponent", "ABossAIController"],
      "Epizod 1 / Kvest 1.2 - Ormondagi pistirma"),

    q("q1_3_first_blood", 0, "Q1_3_TITLE", "ertugrul", "main", [
        o("escort_numan", "Q1_3_OBJ_ESCORT"), o("make_camp", "Q1_3_OBJ_CAMP"),
        o("halima_dialogue", "Q1_3_OBJ_HALIMA"), o("survive_waves", "Q1_3_OBJ_WAVES"),
        o("extinguish_fire", "Q1_3_OBJ_FIRE"), o("arrive_oba", "Q1_3_OBJ_ARRIVE"),
        o("first_council", "Q1_3_OBJ_COUNCIL")],
      {"honor": 10, "skill_points": 1, "resources": {"gold": 20}},
      "q2_1_journey_to_aleppo", "road_to_oba",
      ["UEscortObjectiveComponent", "UWaveSpawnerSubsystem", "UQTESubsystem"],
      "Epizod 1 / Kvest 1.3 - Birinchi qon va qochish"),

    q("q2_1_journey_to_aleppo", 1, "Q2_1_TITLE", "suleyman_shah", "main", [
        o("ride_to_aleppo", "Q2_1_OBJ_RIDE"), o("surrender_weapons", "Q2_1_OBJ_WEAPONS"),
        o("lose_pursuers", "Q2_1_OBJ_CROWD"), o("parkour_rooftops", "Q2_1_OBJ_ROOFS"),
        o("meet_ibn_arabi", "Q2_1_OBJ_IBN_ARABI"), o("palace_audience", "Q2_1_OBJ_PALACE"),
        o("search_nasir_room", "Q2_1_OBJ_NASIR", True), o("save_caravan", "Q2_1_OBJ_CARAVAN", True)],
      {"honor": 8, "skill_points": 1, "resources": {"gold": 60}},
      "q2_2_caravanserai_ambush", "aleppo",
      ["UParkourComponent", "UStealthComponent", "UCrowdSubsystem", "UDialogueSubsystem"],
      "Epizod 2 / Kvest 2.1 - Halabga sayohat"),

    q("q2_2_caravanserai_ambush", 1, "Q2_2_TITLE", "turgut", "main", [
        o("wake_alps", "Q2_2_OBJ_WAKE"), o("extinguish_torches", "Q2_2_OBJ_TORCHES"),
        o("silent_kills", "Q2_2_OBJ_SILENT"), o("command_companions", "Q2_2_OBJ_ORDERS"),
        o("duel_bisol", "Q2_2_OBJ_DUEL"), o("bisol_fate", "Q2_2_OBJ_FATE")],
      {"honor": 10, "skill_points": 1, "resources": {"iron": 6}},
      "q2_3_expose_traitor", "caravanserai",
      ["UAssassinationComponent", "ULightingAwarenessSubsystem", "UCompanionCommandSubsystem"],
      "Epizod 2 / Kvest 2.2 - Karvonsaroy tungi pistirmasi"),

    q("q2_3_expose_traitor", 1, "Q2_3_TITLE", "halima", "main", [
        o("gather_evidence", "Q2_3_OBJ_EVIDENCE"), o("tail_kurdoglu", "Q2_3_OBJ_TAIL"),
        o("eavesdrop_mill", "Q2_3_OBJ_EAVESDROP"), o("council_accusation", "Q2_3_OBJ_COUNCIL"),
        o("kurdoglu_chase", "Q2_3_OBJ_CHASE"), o("kurdoglu_fate", "Q2_3_OBJ_FATE")],
      {"honor": 12, "skill_points": 2},
      "q3_1_infiltrate_amanos", "kayi_oba",
      ["UInvestigationSubsystem", "UTailingObjectiveComponent", "UEavesdropComponent"],
      "Epizod 2 / Kvest 2.3 - Xoinni fosh qilish"),

    q("q3_1_infiltrate_amanos", 2, "Q3_1_TITLE", "claudius_omar", "main", [
        o("wear_monk_robe", "Q3_1_OBJ_DISGUISE"), o("pass_gate_check", "Q3_1_OBJ_GATE_CHECK"),
        o("blend_in_courtyard", "Q3_1_OBJ_BLEND"), o("kill_gate_guards", "Q3_1_OBJ_GUARDS"),
        o("find_yigit", "Q3_1_OBJ_YIGIT", True), o("open_gate", "Q3_1_OBJ_OPEN_GATE")],
      {"honor": 10, "skill_points": 1},
      "q3_2_castle_courtyard", "amanos_castle",
      ["UDisguiseComponent", "USuspicionMeterComponent", "UAssassinationComponent"],
      "Epizod 3 / Kvest 3.1 - Amanos qalasiga infiltratsiya"),

    q("q3_2_castle_courtyard", 2, "Q3_2_TITLE", "gundogdu", "main", [
        o("hold_gate_zone", "Q3_2_OBJ_GATE"), o("kill_priest_officer", "Q3_2_OBJ_PRIEST"),
        o("church_zone", "Q3_2_OBJ_CHURCH"), o("use_burning_cart", "Q3_2_OBJ_CART", True),
        o("tower_stairs", "Q3_2_OBJ_STAIRS")],
      {"honor": 8, "skill_points": 1},
      "q3_3_titus_duel", "amanos_castle",
      ["ULargeBattleSubsystem", "UCompanionCommandSubsystem", "UCrowdCombatLOD"],
      "Epizod 3 / Kvest 3.2 - Qala hovlisi jangi"),

    q("q3_3_titus_duel", 2, "Q3_3_TITLE", "story", "boss", [
        o("break_shield", "Q3_3_OBJ_SHIELD"), o("phase_longsword", "Q3_3_OBJ_LONGSWORD"),
        o("phase_wounded_rage", "Q3_3_OBJ_RAGE"), o("finisher_alp_strike", "Q3_3_OBJ_FINISHER"),
        o("save_halima", "Q3_3_OBJ_HALIMA"), o("mourn_suleyman", "Q3_3_OBJ_MOURNING")],
      {"honor": 20, "skill_points": 3},
      "q4_1_border_ambush", "amanos_tower",
      ["ABossAIController", "UBossPhaseComponent", "UShieldBreakComponent"],
      "Epizod 3 / Kvest 3.3 - Final boss: Titus"),

    q("q4_1_border_ambush", 3, "Q4_1_TITLE", "korkut_bey", "main", [
        o("ride_snow_pass", "Q4_1_OBJ_RIDE"), o("track_mongols", "Q4_1_OBJ_TRACKS"),
        o("mounted_combat", "Q4_1_OBJ_MOUNTED"), o("fight_lasso_riders", "Q4_1_OBJ_LASSO"),
        o("tugtekin_combo", "Q4_1_OBJ_TUGTEKIN"), o("hamza_betrayal", "Q4_1_OBJ_BETRAYAL")],
      {"honor": 6, "skill_points": 1},
      "q4_2_nail_scene", "snow_pass",
      ["UMountedCombatComponent", "USnowDeformationSubsystem", "UScriptedDefeatSubsystem"],
      "Epizod 4 / Kvest 4.1 - Chegaradagi pistirma"),

    q("q4_2_nail_scene", 3, "Q4_2_TITLE", "story", "cutscene", [
        o("breathing_qte", "Q4_2_OBJ_BREATH"), o("resist_qte", "Q4_2_OBJ_RESIST"),
        o("answer_noyan", "Q4_2_OBJ_ANSWER"), o("endure_nail", "Q4_2_OBJ_NAIL")],
      {"honor": 15},
      "q4_3_one_handed_escape", "mongol_camp",
      ["UCutsceneDirector", "UQTESubsystem", "UInjuryStateComponent"],
      "Epizod 4 / Kvest 4.2 - Qiynoq va mix sahnasi"),

    q("q4_3_one_handed_escape", 3, "Q4_3_TITLE", "sungurtekin", "main", [
        o("leave_cage", "Q4_3_OBJ_CAGE"), o("find_bandage_1", "Q4_3_OBJ_BANDAGE"),
        o("release_horses", "Q4_3_OBJ_HORSES", True), o("avoid_dogs", "Q4_3_OBJ_DOGS"),
        o("break_trail", "Q4_3_OBJ_TRAIL"), o("escape_tangut", "Q4_3_OBJ_TANGUT"),
        o("river_jump", "Q4_3_OBJ_RIVER")],
      {"honor": 12, "skill_points": 2},
      "q5_1_return_to_forge", "mongol_camp",
      ["UInjuryStateComponent", "UBleedingComponent", "USnowTrackSubsystem"],
      "Epizod 4 / Kvest 4.3 - Bir qolli qochish"),

    q("q5_1_return_to_forge", 4, "Q5_1_TITLE", "geyikli_baba", "main", [
        o("craft_medicine", "Q5_1_OBJ_MEDICINE"), o("forest_walk", "Q5_1_OBJ_FOREST"),
        o("return_to_oba", "Q5_1_OBJ_RETURN"), o("blacksmith_rhythm", "Q5_1_OBJ_FORGE"),
        o("bind_sword_qte", "Q5_1_OBJ_BIND"), o("rage_strike_tutorial", "Q5_1_OBJ_RAGE")],
      {"honor": 15, "skill_points": 2, "abilities": ["binding_sword", "rage_strike"]},
      "q5_2_test_of_faith", "kayi_oba_winter",
      ["UBlacksmithMinigame", "UBindingSwordComponent", "UFaithMeterComponent"],
      "Epizod 5 / Kvest 5.1 - Temirchilikka qaytish"),

    q("q5_2_test_of_faith", 4, "Q5_2_TITLE", "hayme_ana", "main", [
        o("position_alps", "Q5_2_OBJ_POSITION"), o("break_shield_rage", "Q5_2_OBJ_BREAK"),
        o("defeat_ulubilge", "Q5_2_OBJ_ULUBILGE"), o("ulubilge_fate", "Q5_2_OBJ_FATE")],
      {"honor": 10, "skill_points": 2},
      "q6_1_unite_the_beyliks", "kayi_oba_winter",
      ["UBindingSwordComponent", "UFaithMeterComponent", "ABossAIController"],
      "Epizod 5 / Kvest 5.2 - Iymon sinovi"),

    q("q6_1_unite_the_beyliks", 5, "Q6_1_TITLE", "sungurtekin", "main", [
        o("investigate_aytolun", "Q6_1_OBJ_INVESTIGATE"), o("dodurga_sparring", "Q6_1_OBJ_DODURGA"),
        o("cavdar_bandits", "Q6_1_OBJ_CAVDAR"), o("konya_audience", "Q6_1_OBJ_KONYA"),
        o("expose_gumustekin", "Q6_1_OBJ_GUMUSTEKIN"), o("gather_forces", "Q6_1_OBJ_FORCES")],
      {"honor": 14, "skill_points": 2, "resources": {"gold": 150}},
      "q6_2_night_raid", "konya",
      ["UInvestigationSubsystem", "UAllyForceSubsystem", "UFastTravelSubsystem"],
      "Epizod 6 / Kvest 6.1 - Beyliklarni birlashtirish"),

    q("q6_2_night_raid", 5, "Q6_2_TITLE", "ertugrul", "main", [
        o("infiltrate_camp", "Q6_2_OBJ_INFILTRATE"), o("burn_catapults", "Q6_2_OBJ_CATAPULTS"),
        o("burn_rams", "Q6_2_OBJ_RAMS"), o("mounted_pursuit", "Q6_2_OBJ_MOUNTED"),
        o("defeat_tangut", "Q6_2_OBJ_TANGUT"), o("hamza_choice", "Q6_2_OBJ_HAMZA", True)],
      {"honor": 12, "skill_points": 2},
      "q6_3_noyan_duel", "mongol_camp",
      ["USabotageObjectiveComponent", "UFireSpreadSubsystem", "ULargeBattleSubsystem"],
      "Epizod 6 / Kvest 6.2 - Mogul lageriga tungi reyd"),

    q("q6_3_noyan_duel", 5, "Q6_3_TITLE", "story", "boss", [
        o("unhorse_noyan", "Q6_3_OBJ_UNHORSE"), o("survive_whip", "Q6_3_OBJ_WHIP"),
        o("bloody_duel", "Q6_3_OBJ_BLOODY"), o("faith_finisher", "Q6_3_OBJ_FINISHER"),
        o("noyan_fate", "Q6_3_OBJ_FATE"), o("epilogue_wedding", "Q6_3_OBJ_EPILOGUE")],
      {"honor": 25, "skill_points": 4},
      "", "mongol_camp_shaman_ground",
      ["ABossAIController", "UBossPhaseComponent", "UGrabAttackComponent"],
      "Epizod 6 / Kvest 6.3 - Final boss: Bayju Noyan"),
]

for item in quests:
    path = os.path.join(Q, item["id"] + ".json")
    with io.open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(json.dumps(item, ensure_ascii=False, indent=2))
print("Yozildi:", len(quests), "kvest ->", Q)
