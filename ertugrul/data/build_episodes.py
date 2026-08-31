#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_episodes.py — `episodes_v2.json` generatori va validatori.

Nima uchun generator, qo'lda yozilgan JSON emas?
  • 48 epizod × ~40 maydon = 1900+ qiymat. Qo'lda yozilsa — xato muqarrar
    (sizning v1 JSON'ingizdagi `"season": 0` bug'i aynan shundan).
  • Bu skript ma'lumotni bitta jadvaldan quradi va DARHOL tekshiradi:
    indekslar, sanalar xronologiyasi, arxetip takrorlanishi, ot monopoliyasi,
    mix beat mavjudligi, dangling reference'lar.
  • CI/CD da ishlaydi: xato bo'lsa exit code 1 → build to'xtaydi.

Ishlatish:
    python3 build_episodes.py            # quradi + tekshiradi
    python3 build_episodes.py --check    # faqat tekshiradi
"""
from __future__ import annotations
import json, sys, datetime, collections
from pathlib import Path

OUT   = Path(__file__).parent / "episodes_v2.json"
SCHEMA= Path(__file__).parent / "schema" / "episode.schema.json"

# ═══════════════════════════════════════════════════════════════════════════
#  MAVSUMLAR
# ═══════════════════════════════════════════════════════════════════════════
SEASONS = [
    dict(id="S1", index=1, codename="SARHAD", loc="season.s1.title",
         y0=1227, y1=1230, setting="autumn", tiers=[1,2],
         boss="CHR_TITUS", region="Amanos / Halab / Sham",
         new_enemy="Templar sержant + arbaletchi",
         world="Janub yopiladi, Konya mintaqasi ochiladi"),
    dict(id="S2", index=2, codename="SULTON_SOYASI", loc="season.s2.title",
         y0=1230, y1=1238, setting="summer", tiers=[3,4],
         boss="CHR_KOPEK", region="Konya / Kubadabad / Erzincan",
         new_enemy="Xorazmiy nayzachi + saroy qotili",
         world="Jang tizimi qayta yoziladi (chap qo'l). Mo'g'ul mintaqasi ochiladi"),
    dict(id="S3", index=3, codename="QORA_BULUT", loc="season.s3.title",
         y0=1238, y1=1243, setting="winter", tiers=[4,5],
         boss="CHR_BAIJU", region="Sharqiy Anadolu / Erzurum / Köse Dağ",
         new_enemy="Mo'g'ul otliq kamonchi + nerge",
         world="Saljuqiy vassal. Sharq yopiladi, G'arb (Söğüt) ochiladi"),
    dict(id="S4", index=4, codename="SONGGI_YURISH", loc="season.s4.title",
         y0=1243, y1=1261, setting="full_cycle", tiers=[4,5],
         boss="CHR_BAIJU", region="Söğüt / Domaniç / Vizantiya chegarasi",
         new_enemy="Keshikten + qamal mashinasi",
         world="O'yin tugaydi. NG+ (Xronika rejimi) ochiladi"),
]

# ═══════════════════════════════════════════════════════════════════════════
#  48 EPIZOD — asosiy jadval
#  (slug, hijri, greg, year, conf, archetype, intro_arch, intro_sec,
#   date_mode, tier, mins, traversal[], mih_kind, mih_major,
#   phase, hp_start, hp_max, drain, codex[], enemies{}, max_sim, bg)
# ═══════════════════════════════════════════════════════════════════════════
D, DI, L = "DOCUMENTED", "DISPUTED", "LEGEND"

EPISODES = [
# ─────────────────────────── 1-MAVSUM: SARHAD ───────────────────────────
("och_qargalar","624 Ramazon","1227 Avgust",1227,D,"INVESTIGATION","WALK",180,"OVERLAY",1,50,
 ["FOOT_PARKOUR","HORSE"],"OBJECT",False,"INTACT",100,100,1.0,
 ["CDX_TOPAK_EV","CDX_YAYLAK_KISLAK","CDX_KAYI_TRIBE"],{},0,0,
 "autumn","day","clear","Anadolu chegarasi",False,[],False),

("ikki_asir","624 Shavvol","1227 Sentabr",1227,D,"ESCORT","WALK",120,"DIEGETIC",1,55,
 ["HORSE","FOOT_PARKOUR"],"ON_OTHER",False,"INTACT",100,100,1.0,
 ["CDX_COMPOSITE_BOW","CDX_JALALADDIN","CDX_REFUGEE_CRISIS"],{"Enemy.Crusader.Scout":4},4,0,
 "autumn","day","clear","O'rmon",False,[],False),

("oq_plash","624 Zulqada","1227 Oktabr",1227,D,"DEFENSE","PLUNGE",180,"OVERLAY",1,50,
 ["FOOT_PARKOUR","CAMP_SOCIAL"],"CORPSE",True,"INTACT",100,100,1.0,
 ["CDX_SALB_TASMIR","CDX_TEMPLAR_AMANUS","CDX_LAMELLAR"],
 {"Enemy.Mercenary":12,"Enemy.Templar.Sergeant":2},5,0,
 "autumn","night","clear","Qayi obasi",False,["violence.graphic","execution"],False),

("otaning_ismi","624 Zulhijja","1227 Noyabr",1227,DI,"COURT","WATCH",240,"SPOKEN",1,60,
 ["CAMP_SOCIAL"],"CAMERA",False,"INTACT",100,100,1.0,
 ["CDX_SULEYMAN_SHAH","CDX_GUNDUZ_ALP","CDX_OSMAN_COINS","CDX_ASIKPASAZADE"],{},0,0,
 "autumn","dusk","clear","Qayi obasi",False,[],False),

("qorli_dovon","625 Muharram","1227 Dekabr",1227,D,"SURVIVAL","HANDS",180,"OVERLAY",1,65,
 ["SNOW_ICE","FOOT_PARKOUR"],"MECHANICAL",False,"INTACT",100,100,1.2,
 ["CDX_KARA_CADIR","CDX_FELT_MAKING","CDX_NOMAD_ECONOMY"],{"Enemy.Wolf":6},6,0,
 "winter","varies","snow","Torus dovoni",False,["animal.harm"],False),

("sultan_han","625 Safar","1228 Yanvar",1228,D,"INVESTIGATION","WALK",180,"DIEGETIC",2,55,
 ["FOOT_PARKOUR","CAMP_SOCIAL"],"OBJECT",False,"INTACT",100,100,1.0,
 ["CDX_CARAVANSERAI","CDX_DIRHAM","CDX_SILK_ROAD_ANATOLIA","CDX_AHI_FUTUWWA"],
 {"Enemy.Spy":1},2,0,
 "winter","varies","clear","Sultan Han",False,[],False),

("halab_bola_amir","625 Rabiul-avval","1228 Fevral",1228,D,"COURT","WALK",240,"OVERLAY",2,65,
 ["FOOT_PARKOUR","CAMP_SOCIAL"],"OBJECT",False,"INTACT",100,100,1.0,
 ["CDX_AL_AZIZ","CDX_ATABEG_TOGHRIL","CDX_ALEPPO_CITADEL","CDX_AYYUBID"],
 {"Enemy.PalaceGuard":6},4,0,
 "winter","day","clear","Halab",False,[],False),

("sham_yoli_ibn_arabiy","625 Jumadal-avval","1228 Aprel",1228,D,"ESCORT","WALK",180,"OVERLAY",2,60,
 ["CAMEL","FOOT_PARKOUR"],"EVENT",True,"INTACT",100,100,1.0,
 ["CDX_SALB_TASMIR","CDX_IBN_ARABI","CDX_DAMASCUS_1228","CDX_FUSUS"],
 {"Enemy.DesertBandit":8},5,0,
 "spring","day","dust","Sham yo'li / Damashq",False,["torture.witnessed","execution"],True),

("kurdoglu_iplari","625 Rajab","1228 Iyun",1228,D,"INVESTIGATION","WATCH",120,"OVERLAY",2,55,
 ["CAMP_SOCIAL","FOOT_PARKOUR"],"OBJECT",False,"INTACT",100,100,1.0,
 ["CDX_TRIBAL_ELECTION","CDX_UC_BEY","CDX_ALP_RANK"],{"Enemy.Traitor":5},4,0,
 "summer","night","clear","Qayi obasi",False,[],False),

("ormondagi_qon","625 Ramazon","1228 Avgust",1228,D,"CHASE","OTHER_EYES",180,"OVERLAY",2,50,
 ["HORSE","FOOT_PARKOUR"],"CAMERA",False,"INTACT",100,100,1.0,
 ["CDX_CROSSBOW","CDX_TEMPLAR_TACTICS","CDX_AMANUS_GEOGRAPHY"],
 {"Enemy.Templar.Sergeant":10,"Enemy.Crossbowman":4},6,0,
 "summer","day","clear","Qarag'ay o'rmoni",False,["violence.graphic"],False),

("suv_yoli","625 Zulqada","1228 Oktabr",1228,D,"INFILTRATION","PLUNGE",240,"OVERLAY",2,70,
 ["SWIM","SEWER_CAVE","FOOT_PARKOUR","ROPE_GRAPPLE"],"ON_OTHER",False,"INTACT",100,100,1.0,
 ["CDX_BAGRAS_CASTLE","CDX_MEDIEVAL_WATER","CDX_TEMPLAR_RULE"],
 {"Enemy.Templar.Sergeant":16,"Enemy.Templar.Knight":6},4,0,
 "autumn","night","clear","Bagras qal'asi",False,["torture.aftermath"],False),

("bagras","627 Safar","1229 Dekabr",1229,D,"SIEGE","LABOUR",300,"OVERLAY",2,80,
 ["VERTICAL_SIEGE","ROPE_GRAPPLE","FOOT_PARKOUR"],"DIALOGUE",True,"INTACT",100,100,1.0,
 ["CDX_SIEGE_1229_BAGRAS","CDX_TREBUCHET","CDX_TEMPLAR_DEFEAT"],
 {"Enemy.Templar.Knight":20,"Enemy.Templar.Sergeant":40,"Boss.Titus":1},6,400,
 "winter","day","fog","Bagras qal'asi",False,["violence.graphic"],False),

# ───────────────────── 2-MAVSUM: SULTON SOYASI ─────────────────────
("yassicemen","627 Ramazon","10 Avgust 1230",1230,D,"SIEGE","WALK",240,"OVERLAY",3,70,
 ["HORSE","FOOT_PARKOUR"],"MECHANICAL",False,"INTACT",100,100,1.1,
 ["CDX_YASSICEMEN","CDX_JALALADDIN","CDX_KAYQUBAD_I","CDX_KHWAREZMIAN_ARMY"],
 {"Enemy.Khwarezmian.Spearman":60,"Enemy.Khwarezmian.Cavalry":30},7,3000,
 "summer","day","clear","Yassıçemen (Erzincan)",False,["violence.graphic","war.mass"],False),

("ov_boshligi","628 Muharram","1230 Noyabr",1230,D,"INVESTIGATION","WALK",180,"OVERLAY",3,55,
 ["HORSE","CAMP_SOCIAL"],"OBJECT",False,"INTACT",100,100,1.0,
 ["CDX_KOPEK","CDX_KUBADABAD","CDX_EMIR_I_SIKAR","CDX_SELJUK_COURT"],
 {"Enemy.Assassin.Court":4},4,0,
 "autumn","day","clear","Kubadabad",False,[],False),

("karacadag_birinchi_tosh","629 Rabiul-oxir","1232 Fevral",1232,L,"RITUAL","HANDS",240,"OVERLAY",3,60,
 ["CAMP_SOCIAL","FOOT_PARKOUR"],"DIALOGUE",False,"INTACT",100,100,1.0,
 ["CDX_SOGUT_DOMANIC","CDX_BLACKSMITHING","CDX_CRAFTING_TRADITION"],{},0,0,
 "winter","day","clear","Karacadağ",False,[],False),

("yetim_qoshin","630 Shaban","1233 May",1233,D,"ESCORT","WALK",180,"OVERLAY",3,60,
 ["MOVING_PLATFORM","HORSE"],"ON_OTHER",False,"INTACT",100,100,1.0,
 ["CDX_KHWAREZMIAN_REMNANT","CDX_IQTA","CDX_KIR_KHAN"],
 {"Enemy.Bandit":25},6,120,
 "spring","day","clear","Larande yo'li",False,["famine"],False),

("konya_kutubxona","631 Muharram","1233 Oktabr",1233,D,"COURT","WALK",240,"DIEGETIC",3,55,
 ["FOOT_PARKOUR","CAMP_SOCIAL"],"OBJECT",False,"INTACT",100,100,1.0,
 ["CDX_KONYA","CDX_RUMI_ARRIVAL","CDX_QUNAWI","CDX_SELJUK_LITERACY","CDX_MEDIEVAL_SURGERY"],
 {"Enemy.PalaceGuard":3},3,0,
 "autumn","day","rain","Konya",False,[],False),

("domanic_yozi","632 Rajab","1235 Aprel",1235,L,"SURVIVAL","BREATH",180,"OVERLAY",3,50,
 ["HORSE","FOOT_PARKOUR"],"CAMERA",False,"INTACT",100,100,1.0,
 ["CDX_TRANSHUMANCE","CDX_HORSE_BREEDING","CDX_STEPPE_MEDICINE"],
 {"Enemy.Wolf.Rabid":6},5,0,
 "spring","day","clear","Domaniç yaylovi",False,["animal.harm"],False),

("zaharlangan_taom","634 Zulhijja","31 May 1237",1237,D,"INVESTIGATION","WATCH",300,"SPOKEN",3,65,
 ["CAMP_SOCIAL","FOOT_PARKOUR"],"THREAT",False,"INTACT",100,100,1.0,
 ["CDX_KAYQUBAD_DEATH","CDX_KAYKHUSRAW_II","CDX_MEDIEVAL_POISONS"],
 {"Enemy.Assassin.Court":2},2,0,
 "spring","dusk","clear","Kayseri saroyi",False,["poisoning"],False),

("kopek_hukmronligi","635 Muharram","1237 Avgust",1237,D,"COURT","WALK",240,"OVERLAY",4,60,
 ["FOOT_PARKOUR","SEWER_CAVE"],"CORPSE",True,"INTACT",100,100,1.0,
 ["CDX_KOPEK_TERROR","CDX_SELJUK_EXECUTIONS","CDX_IBN_BIBI"],
 {"Enemy.Assassin.Court":8,"Enemy.PalaceGuard":10},5,0,
 "summer","dawn","clear","Konya",False,["execution","torture.aftermath"],False),

("yolgiz_kelasan","635 Rabiul-avval","1237 Oktabr",1237,D,"INFILTRATION","WALK",240,"DIEGETIC",4,65,
 ["FOOT_PARKOUR","CAMP_SOCIAL"],"THREAT",True,"INTACT",100,100,1.0,
 ["CDX_ZAZADIN_HAN","CDX_KOPEK_PATRONAGE"],
 {"Enemy.Assassin.Court":30},0,0,
 "autumn","dusk","clear","Zazadin Han",False,["threat.mutilation"],False),

("chegara_yonmoqda","635 Jumadal-oxir","1238 Yanvar",1238,D,"DEFENSE","WATCH",180,"OVERLAY",4,60,
 ["FOOT_PARKOUR","SNOW_ICE"],"MECHANICAL",False,"INTACT",100,100,1.3,
 ["CDX_CHORMAQAN","CDX_MONGOL_SCOUTS","CDX_NERGE"],
 {"Enemy.Raider":40,"Enemy.Mongol.Scout":3},6,150,
 "winter","night","snow","Chegara",False,["violence.graphic"],False),

("kopekning_oxiri","636 Rabiul-avval","1238 Sentabr",1238,D,"CHASE","WALK",240,"OVERLAY",4,55,
 ["FOOT_PARKOUR","VERTICAL_SIEGE"],"OBJECT",True,"INTACT",100,100,1.0,
 ["CDX_KOPEK_DEATH","CDX_HUSAM_AL_DIN"],
 {"Enemy.PalaceGuard":12,"Enemy.Assassin.Court":8},6,0,
 "autumn","night","clear","Konya saroyi",False,["execution","fire"],False),

("mix","636 Jumadal-avval","1238 Dekabr",1238,D,"INFILTRATION","WATCH",360,"OVERLAY",4,75,
 ["FOOT_PARKOUR","SNOW_ICE"],"EVENT",True,"FRESH",0,55,2.0,
 ["CDX_SALB_TASMIR","CDX_HAND_ANATOMY","CDX_MEDIEVAL_PAIN"],
 {"Enemy.Assassin.Court":6},6,0,
 "winter","night","snow","Karvonsaroy xarobasi",True,
 ["torture.graphic","mutilation","restraint"],True),

# ───────────────────── 3-MAVSUM: QORA BULUT ─────────────────────
("qorda_uch_kun","636 Jumadal-oxir","1238 Dekabr",1238,D,"SURVIVAL","PLUNGE",240,"OVERLAY",4,70,
 ["SNOW_ICE","FOOT_PARKOUR"],"MECHANICAL",True,"FRESH",0,55,2.0,
 ["CDX_CAUTERIZATION","CDX_STEPPE_MEDICINE","CDX_HYPOTHERMIA","CDX_WOLF_BEHAVIOR"],
 {"Enemy.Wolf":3},3,0,
 "winter","varies","snow","Qorli o'rmon",False,["self.surgery","gore"],False),

("chap_qol_yoli","636 Rajab","1239 Fevral",1239,DI,"RITUAL","HANDS",300,"OVERLAY",4,65,
 ["SEWER_CAVE","FOOT_PARKOUR"],"DIALOGUE",True,"FRESH",25,55,1.4,
 ["CDX_HACI_BEKTAS","CDX_KHORASAN_MIGRATION","CDX_DERVISH_LODGES","CDX_SWORD_FORGING"],
 {},0,0,
 "winter","varies","snow","G'or",False,[],False),

("oz_qabringni_korish","637 Ramazon","1240 Aprel",1240,D,"COURT","WALK",240,"OVERLAY",4,55,
 ["CAMP_SOCIAL","FOOT_PARKOUR"],"DIALOGUE",True,"CHRONIC",40,55,1.2,
 ["CDX_TRIBAL_SUCCESSION","CDX_OATH_BINDING","CDX_KURULTAY"],
 {"Enemy.Rival.Bey":1},1,0,
 "spring","day","clear","Qayi obasi",False,[],False),

("paiza","638 Safar","1240 Sentabr",1240,D,"INVESTIGATION","WATCH",180,"OVERLAY",4,60,
 ["FOOT_PARKOUR","HORSE","CAMP_SOCIAL"],"ADVANTAGE",False,"CHRONIC",45,55,1.1,
 ["CDX_PAIZA","CDX_YAM_POSTAL","CDX_MONGOL_INTELLIGENCE"],
 {"Enemy.Spy":9,"Enemy.PalaceGuard":21},5,0,
 "autumn","day","clear","Konya / Kayseri / Sivas",False,[],False),

("nerge","638 Jumadal-avval","1240 Dekabr",1240,D,"CHASE","WALK",240,"OVERLAY",4,65,
 ["HORSE"],"MECHANICAL",True,"CHRONIC",45,55,1.6,
 ["CDX_NERGE","CDX_MONGOL_BOW","CDX_PARTHIAN_SHOT","CDX_MONGOL_HORSES","CDX_ARBAN_JAGUN"],
 {"Enemy.Mongol.HorseArcher":60},8,0,
 "winter","day","snow","Dasht",False,["violence.graphic"],False),

("erzurum_devorlari","639 Muharram","1241 Iyul",1241,D,"DEFENSE","LABOUR",240,"OVERLAY",5,70,
 ["VERTICAL_SIEGE","ROPE_GRAPPLE"],"MECHANICAL",True,"CHRONIC",40,55,1.5,
 ["CDX_BAIJU_APPOINTED","CDX_MONGOL_SIEGE_ENGINES","CDX_ERZURUM","CDX_OGEDEI_DEATH"],
 {"Enemy.Mongol.HorseArcher":80,"Enemy.Mongol.Sapper":20,"Enemy.SiegeEngine":4},8,300,
 "summer","day","clear","Erzurum",False,["violence.graphic","war.mass"],False),

("oq_plash_qora_bulut","639 Rajab","1242 Yanvar",1242,D,"COURT","WALK",240,"OVERLAY",5,55,
 ["FOOT_PARKOUR","CAMP_SOCIAL"],"DIALOGUE",True,"CHRONIC",45,55,1.2,
 ["CDX_TEMPLAR_MONGOL_RELATIONS","CDX_ANTIOCH_1242"],{},0,0,
 "winter","day","fog","Bagras qal'asi",False,[],False),

("noyan","640 Safar","1242 Avgust",1242,D,"INFILTRATION","WALK",360,"OVERLAY",5,60,
 ["CAMP_SOCIAL","FOOT_PARKOUR"],"THREAT",True,"CHRONIC",42,55,1.2,
 ["CDX_BAIJU_NOYAN","CDX_MONGOL_ADMINISTRATION","CDX_YASSA_LAW","CDX_MONGOL_TOLERANCE"],
 {"Enemy.Keshig":40},0,600,
 "summer","day","dust","Mo'g'ul lageri",False,["threat.mutilation"],False),

("sivas_kengashi","640 Shaban","1243 Fevral",1243,DI,"COURT","WATCH",240,"OVERLAY",5,55,
 ["CAMP_SOCIAL"],"DIALOGUE",False,"CHRONIC",45,55,1.1,
 ["CDX_KOSE_DAG_ARMIES","CDX_SELJUK_MILITARY","CDX_CHRONICLE_EXAGGERATION"],{},0,0,
 "winter","day","clear","Sivas",False,[],False),

("oxirgi_tinch_kun","640 Zulhijja","1243 May",1243,D,"RITUAL","BREATH",180,"OVERLAY",5,45,
 ["CAMP_SOCIAL"],"RITUAL_ACT",False,"CHRONIC",45,55,1.0,
 ["CDX_PRE_BATTLE_RITUAL","CDX_WILL_INHERITANCE","CDX_HELAL"],{},0,0,
 "spring","dusk","clear","Köse Dağ lageri",True,[],False),

("kose_dag","641 Muharram","26 Iyun 1243",1243,D,"SIEGE","WATCH",300,"OVERLAY",5,85,
 ["HORSE","FOOT_PARKOUR","SNOW_ICE"],"MECHANICAL",True,"CHRONIC",45,55,2.2,
 ["CDX_KOSE_DAG","CDX_FEIGNED_RETREAT","CDX_MONGOL_TRIBUTE","CDX_SELJUK_COLLAPSE"],
 {"Enemy.Mongol.HorseArcher":200,"Enemy.Keshig":40,"Enemy.Mongol.Heavy":60},8,3000,
 "summer","dawn","fog","Köse Dağ",False,["violence.graphic","war.mass"],False),

("sanoq","641 Safar","1243 Iyul",1243,D,"INFILTRATION","WALK",300,"OVERLAY",5,70,
 ["CAMP_SOCIAL","FOOT_PARKOUR","SNOW_ICE"],"THREAT",True,"CHRONIC",20,55,1.6,
 ["CDX_MONGOL_CENSUS","CDX_CRAFTSMEN_DEPORTATION","CDX_KESHIG"],
 {"Enemy.Keshig":24},4,800,
 "summer","night","clear","Asirlar lageri",False,["threat.mutilation","captivity"],False),

# ───────────────────── 4-MAVSUM: SO'NGGI YURISH ─────────────────────
("on_ikki_million","641 Rajab","1243 Dekabr",1243,D,"COURT","HANDS",240,"DIEGETIC",5,55,
 ["CAMP_SOCIAL"],"ADVANTAGE",False,"CHRONIC",45,55,1.1,
 ["CDX_MONGOL_TRIBUTE","CDX_DARUGACHI","CDX_VASSALAGE"],
 {"Enemy.Mongol.TaxGuard":8},4,0,
 "winter","day","clear","Qayi obasi",False,[],False),

("garbga","642 Muharram","1244 Iyun",1244,D,"ESCORT","WALK",240,"OVERLAY",4,70,
 ["MOVING_PLATFORM","BOAT","FOOT_PARKOUR","HORSE"],"RITUAL_ACT",False,"CHRONIC",45,55,1.2,
 ["CDX_TURKMEN_MIGRATION","CDX_UC_FRONTIER","CDX_SAKARYA"],
 {"Enemy.Bandit":30,"Enemy.Byzantine.Akritai":12},6,400,
 "summer","varies","varies","G'arbga yo'l",False,["famine","child.risk"],False),

("sogut","642 Ramazon","1245 Fevral",1245,DI,"INVESTIGATION","WALK",240,"OVERLAY",4,60,
 ["FOOT_PARKOUR","CAMP_SOCIAL"],"RITUAL_ACT",True,"CHRONIC",45,55,1.2,
 ["CDX_SOGUT_HISTORY","CDX_BYZANTINE_VILLAGERS","CDX_TAHRIR_DEFTERI"],
 {"Enemy.Byzantine.Akritai":8},4,0,
 "winter","day","fog","Söğüt",False,[],False),

("axiy_ochogi","643 Rabiul-avval","1245 Avgust",1245,D,"RITUAL","HANDS",240,"OVERLAY",4,55,
 ["CAMP_SOCIAL","FOOT_PARKOUR"],"ADVANTAGE",False,"CHRONIC",48,55,1.0,
 ["CDX_AHI_BROTHERHOOD","CDX_FUTUWWA","CDX_AN_NASIR","CDX_GUILD_ECONOMY"],{},0,0,
 "summer","day","clear","Axiy lodjasi",False,[],False),

("karacahisar","645 Shaban","1247 Dekabr",1247,DI,"SIEGE","HANDS",300,"OVERLAY",5,75,
 ["VERTICAL_SIEGE","ROPE_GRAPPLE","SEWER_CAVE"],"MECHANICAL",True,"CHRONIC",42,55,1.7,
 ["CDX_KARACAHISAR","CDX_WITTEK_THESIS","CDX_GERMIYAN"],
 {"Enemy.Byzantine.Garrison":50,"Enemy.Germiyan.Warrior":30},7,300,
 "winter","dawn","snow","Karacahisar",False,["violence.graphic"],False),

("ogil","648 Rajab","1250 Oktabr",1250,DI,"RITUAL","BREATH",180,"OVERLAY",4,50,
 ["CAMP_SOCIAL","HORSE"],"DIALOGUE",False,"CHRONIC",45,55,1.0,
 ["CDX_OSMAN_BIRTH","CDX_SUCCESSION_CUSTOM"],{},0,0,
 "autumn","day","clear","Söğüt",False,[],False),

("vizantiya_chegarasi","650 Muharram","1252 Mart",1252,D,"INFILTRATION","WALK",240,"OVERLAY",5,65,
 ["FOOT_PARKOUR","CAMP_SOCIAL","BOAT"],"ADVANTAGE",True,"ADAPTED",45,70,1.0,
 ["CDX_NICAEA","CDX_AKRITAI","CDX_BYZANTINE_MEDICINE","CDX_FRONTIER_INTERMARRIAGE"],
 {"Enemy.Byzantine.Akritai":14},5,0,
 "spring","day","rain","Nikeya chegarasi",False,[],False),

("ikkinchi_mix","653 Ramazon","1255 Oktabr",1255,D,"DEFENSE","BREATH",300,"OVERLAY",5,70,
 ["FOOT_PARKOUR","VERTICAL_SIEGE"],"THREAT",True,"ADAPTED",55,70,1.6,
 ["CDX_MONGOL_ANATOLIA_1255","CDX_ILKHANATE"],
 {"Enemy.Mongol.HorseArcher":250,"Enemy.Keshig":60,"Enemy.SiegeEngine":6},8,600,
 "autumn","day","clear","Söğüt",False,["threat.mutilation","violence.graphic"],False),

("hamzaning_kechirimi","654 Safar","1256 Mart",1256,D,"INVESTIGATION","WALK",180,"OVERLAY",5,55,
 ["FOOT_PARKOUR","HORSE"],"DIALOGUE",True,"ADAPTED",55,70,1.1,
 ["CDX_FRONTIER_DEFECTION","CDX_FORGIVENESS_CUSTOM"],
 {"Enemy.Mongol.Deserter":6},4,0,
 "spring","day","rain","Xaroba",False,[],False),

("tungi_olov","656 Zulhijja","1258 Dekabr",1258,D,"INFILTRATION","PLUNGE",300,"OVERLAY",5,75,
 ["SNOW_ICE","FOOT_PARKOUR","ROPE_GRAPPLE"],"MECHANICAL",True,"ADAPTED",30,70,2.0,
 ["CDX_BAGHDAD_1258","CDX_MONGOL_WINTER_CAMP","CDX_HULEGU"],
 {"Enemy.Keshig":80,"Enemy.Mongol.HorseArcher":300,"Enemy.Shaman":1},6,600,
 "winter","night","snow","Mo'g'ul qishki lageri",True,["fire","violence.graphic"],False),

("uchinchi_mix","657 Muharram","1259 Yanvar",1259,DI,"SIEGE","WATCH",240,"OVERLAY",5,65,
 ["FOOT_PARKOUR"],"EVENT",True,"ADAPTED",40,70,2.4,
 ["CDX_BAIJU_DEATH","CDX_MONGOL_SUCCESSION"],
 {"Boss.Baiju":1},1,0,
 "winter","night","snow","Yonayotgan lager",False,["violence.graphic","mutilation"],False),

("tanga","659 Rabiul-avval","1261 Fevral",1261,D,"COURT","WALK",300,"DIEGETIC",4,70,
 ["CAMP_SOCIAL","FOOT_PARKOUR"],"RITUAL_ACT",True,"ADAPTED",50,70,1.0,
 ["CDX_OSMAN_COINS","CDX_BYZANTINE_SILENCE","CDX_HISTORICAL_METHOD"],{},0,0,
 "spring","day","clear","Söğüt",False,[],False),
]

# ═══════════════════════════════════════════════════════════════════════════
#  7 SHUBHA SAHNASI
# ═══════════════════════════════════════════════════════════════════════════
UNCERTAINTY = [
 ("SS_1","EP004","ss.1.q",[("suleyman_shah","ss.1.a", L,["CDX_SULEYMAN_SHAH","CDX_ASIKPASAZADE"],
    "Aşıkpaşazade (1480-yillar) va usmonli xronikalari. Zamondosh manba yo'q."),
   ("gunduz_alp","ss.1.b", DI,["CDX_GUNDUZ_ALP","CDX_OSMAN_COINS"],
    "Usmonning Yenişehir tangalari: «Osman bin Ertuğrul bin Gündüz Alp». İnalcık, Ortaylı, Turan shu fikrda; TDV Sulaymon Shoh versiyasini 'aniq noto'g'ri' deb baholaydi.")],
  ["world_state.father_lineage","EP048.genealogy_scene"]),

 ("SS_2","EP012","ss.2.q",[("kayi","ss.2.a", L,["CDX_KAYI_TRIBE","CDX_YAZICIOGLU_ALI"],
    "Qayi shajarasi birinchi marta ~1420-30 da Yazıcıoğlu Ali tomonidan kiritilgan — Ertug'rul vafotidan ~150 yil keyin."),
   ("unnamed","ss.2.b", DI,["CDX_SOURCE_CRITICISM","CDX_KAFADAR"],
    "Kafadar: 'XV asr shajara ixtirosidagi ijodiy qayta kashf'. Lowry, Imber, Lindner ham manbalar yetarli emas deb hisoblaydi.")],
  ["world_state.tribe_identity"]),

 ("SS_3","EP019","ss.3.q",[("kopek","ss.3.a", DI,["CDX_KOPEK","CDX_IBN_BIBI"],
    "Ibn Bibi an'anasi Köpekni ko'rsatadi, lekin bu keyingi talqin."),
   ("kaykhusraw_faction","ss.3.b", DI,["CDX_KAYKHUSRAW_II"],
    "Keyxusrav II guruhi ham manfaatdor edi."),
   ("unknown","ss.3.c", D,["CDX_KAYQUBAD_DEATH"],
    "Suyaklarida zahar forensik tahlilda TOPILGAN — o'lim isbotlangan. Aybdor esa manbalarda aniq emas.")],
  ["world_state.poisoner_belief","S3.politics"]),

 ("SS_4","EP026","ss.4.q",[("sultan_grant","ss.4.a", L,["CDX_SOGUT_DOMANIC"],
    "Aşıkpaşazade va TDV: Sulton Alauddin Söğütni kışlak, Domaniçni yaylak qilib bergan. Hujjat yo'q."),
   ("uc_bey_seizure","ss.4.b", DI,["CDX_UC_BEY","CDX_FRONTIER_SEIZURE"],
    "Xronikalarning boshqa versiyasi: Ertug'rul uc beyi sifatida o'rnashgan va yerni o'zi egallagan.")],
  ["world_state.sogut_claim"]),

 ("SS_5","EP033","ss.5.q",[("modern_80k","ss.5.a", D,["CDX_KOSE_DAG_ARMIES"],
    "Zamonaviy tahlil: ~80,000 saljuqiy, ~30,000 mo'g'ul."),
   ("chronicle_200k","ss.5.b", L,["CDX_CHRONICLE_EXAGGERATION"],
    "Xronikalar 160,000-200,000 deydi — o'rta asr manbalarida odatiy mubolag'a.")],
  ["EP035.battle_scale","EP035.agent_count"]),

 ("SS_6","EP041","ss.6.q",[("byzantine","ss.6.a", L,["CDX_KARACAHISAR"],
    "TDV va usmonli an'anasi: vizantiya qal'asi, Ertug'rul 629/1231-32 da olgan."),
   ("germiyan","ss.6.b", DI,["CDX_WITTEK_THESIS","CDX_GERMIYAN"],
    "Wittek, Kafadar, Lindner, Foss: mintaqa ~1180-dan beri vizantiya emas edi; qal'a germiyan bo'lishi mumkin va hikoya usmonli-germiyan urushini g'azo qilib ko'rsatgan.")],
  ["world_state.karacahisar_claim"]),

 ("SS_7","EP048","ss.7.q",[("full_genealogy","ss.7.a", L,["CDX_OGHUZ_GENEALOGY"],
    "To'liq shajara — O'g'uz Xongacha. Bu XV asrda yozilgan."),
   ("father_only","ss.7.b", DI,["CDX_OSMAN_COINS"],
    "Faqat otagacha — tanga dalili qanchalik ruxsat bersa, shuncha."),
   ("no_genealogy","ss.7.c", D,["CDX_HISTORICAL_METHOD"],
    "Shajarasiz — 'Men kim ekanimni bilmayman, lekin nima qurganimni bilaman.'")],
  ["EP048.ending_variant"]),
]

# ═══════════════════════════════════════════════════════════════════════════
#  SIDE QUEST ZANJIRLARI
# ═══════════════════════════════════════════════════════════════════════════
CHAINS = [
 ("SQC_AYKIZ_TEARS","CHR_AYKIZ","sqc.aykiz.sum",["EP003","EP006","EP009","EP012"],
  "Kurdo'g'lu fosh bo'lishi 1 epizod tezlashadi",["Reward.Evidence.Kurdoglu"]),
 ("SQC_CLAUDIUS_TO_OMAR","CHR_IBN_ARABI","sqc.claudius.sum",["EP008","EP011","EP012","EP022","EP031"],
  "EP031 da Titus bilan ittifoq imkoniyati ochiladi",["Reward.Ally.Templar"]),
 ("SQC_WOLF_HUNT","CHR_DELI_DEMIR","sqc.wolf.sum",["EP005","EP010","EP015"],
  "Zirh retsepti — EP044 mudofaasida +15% chidamlilik",["Reward.Recipe.WolfArmor"]),
 ("SQC_TRAIL_OF_EFTELYA","CHR_AFSIN_BEY","sqc.eftelya.sum",["EP006","EP014","EP017","EP020"],
  "Köpekning tarmog'i 2 epizod erta ochiladi",["Reward.Intel.KopekNetwork"]),
 ("SQC_SELCAN_REPENTANCE","CHR_SELCAN","sqc.selcan.sum",["EP009","EP016","EP027","EP038","EP042"],
  "Merosxo'r tanlovi kengayadi (3 → 4 variant)",["Reward.Heir.Extra"]),
 ("SQC_GONCAGUL_TRAP","CHR_BANU_CICEK","sqc.goncagul.sum",["EP016","EP022","EP027"],
  "EP027 da beklikni qaytarish 2 ta ishontirish kam talab qiladi",["Reward.Social.Proof"]),
 ("SQC_BEKTAS_WISDOM","CHR_HACI_BEKTAS","sqc.bektas.sum",["EP026","EP034","EP040","EP045","EP048"],
  "Iymon 76+ darajasiga chiqish imkoni — «Sukunat» mahorati",["Reward.Ability.Sukunat"]),
 ("SQC_ARTUK_APOTHECARY","CHR_ARTUK_BEY","sqc.artuk.sum",["EP018","EP025","EP030","EP043"],
  "HandIntegrity davolash retseptlari (+35 muolaja)",["Reward.Recipe.HandSalve"]),
 ("SQC_DUNDAR_FIRST_SWORD","CHR_DUNDAR","sqc.dundar.sum",["EP015","EP027","EP042"],
  "Merosxo'r jang sifati oshadi",["Reward.Heir.Combat"]),
 ("SQC_HAMZA_CONSCIENCE","CHR_HAMZA","sqc.hamza.sum",["EP017","EP023","EP032","EP036","EP045"],
  "EP047 da Hamza sizni qutqaradi; bajarilmasa — u o'ladi",["Reward.Ally.Hamza"]),
 ("SQC_AHI_TREASURY","CHR_AHI_MASTER","sqc.ahi.sum",["EP040","EP042","EP044","EP048"],
  "Iqtisodiy g'alaba yo'li — EP048 da alternativ final",["Reward.Economy.Path"]),
 ("SQC_GREEK_PHYSICIAN","CHR_NIKEPHOROS","sqc.greek.sum",["EP039","EP041","EP043"],
  "Suyak protezi — MaxIntegrity +15 (o'yindagi yagona yaxshilanish)",["Reward.Prosthesis"]),
]

# ═══════════════════════════════════════════════════════════════════════════
#  QURISH
# ═══════════════════════════════════════════════════════════════════════════
def parry_window(hp: float, phase: str) -> float:
    if phase == "INTACT":
        return 180.0
    t = max(0.0, min(hp / 100.0, 1.0))
    return round(110.0 + (165.0 - 110.0) * (t ** 0.7), 1)


def build() -> dict:
    eps = []
    for i, row in enumerate(EPISODES):
        (slug, hijri, greg, year, conf, arch, iarch, isec, dmode, tier, mins,
         trav, mih_kind, mih_major, phase, hp0, hpmax, drain,
         codex, enemies, maxsim, bg, env_season, tod, weather, region,
         silence, warnings, torture) = row

        num = i + 1
        eid = f"EP{num:03d}"
        sidx = i % 12
        season = SEASONS[i // 12]

        ep = {
            "id": eid,
            "season_id": season["id"],
            "global_index": i,
            "season_index": sidx,

            "loc_key_title":       f"ep.{eid.lower()}.title",
            "loc_key_synopsis":    f"ep.{eid.lower()}.synopsis",
            "loc_key_cliffhanger": f"ep.{eid.lower()}.cliffhanger",

            "anchor": {
                "hijri": hijri,
                "gregorian": greg,
                "year_gregorian": year,
                "confidence": conf,
                "loc_key_note": f"ep.{eid.lower()}.history_note",
            },

            "intro": {
                "archetype": iarch,
                "duration_sec": isec,
                "date_display": dmode,
                "loc_key_description": f"ep.{eid.lower()}.intro",
                "allow_movement": eid != "EP024",
                "allow_combat": False,
                "director_camera_weight_max": 0.35,
            },

            "archetype": arch,
            "difficulty_tier": tier,
            "estimated_minutes": mins,

            "objectives": [{
                "id": f"OBJ_{slug.upper()}_MAIN",
                "loc_key": f"ep.{eid.lower()}.obj.main",
                "kind": {
                    "INFILTRATION": "STEALTH", "SIEGE": "ELIMINATE",
                    "SURVIVAL": "SURVIVE", "INVESTIGATION": "INVESTIGATE",
                    "COURT": "DIALOGUE", "ESCORT": "ESCORT",
                    "DEFENSE": "HOLD", "CHASE": "REACH", "RITUAL": "CRAFT",
                }[arch],
                "optional": False,
                "scripted_failure": eid in ("EP010", "EP020", "EP035"),
            }],

            "enemy_composition": {
                "archetypes": enemies,
                "max_simultaneous": maxsim,
                "background_agents": bg,
            },

            "traversal": trav,
            "mechanics_introduced": [],

            "mih_beat": {
                "kind": mih_kind,
                "loc_key": f"ep.{eid.lower()}.mih",
                "tag": f"Mih.Beat.{mih_kind.title().replace('_','')}",
                "is_major": mih_major,
            },

            "wound_state": {
                "phase": phase,
                "expected_integrity_at_start": float(hp0),
                "max_integrity": float(hpmax),
                "drain_multiplier": float(drain),
                "parry_window_ms": parry_window(hp0, phase),
            },

            "codex_unlocks": codex,
            "prerequisites": [] if num == 1 else [f"EP{num-1:03d}"],
            "unlocks": [] if num == 48 else [f"EP{num+1:03d}"],

            "checkpoints": [f"CP_{eid}_INTRO_END", f"CP_{eid}_MID", f"CP_{eid}_FINALE"],
            "fail_conditions": ["Fail.PlayerDeath"] + (
                ["Fail.EscortLost"] if arch == "ESCORT" else []) + (
                ["Fail.PositionLost"] if arch == "DEFENSE" else []) + (
                ["Fail.Detected"] if arch == "INFILTRATION" and eid != "EP032" else []),
            "world_state_deltas": [f"Episode.{eid}.Completed"],

            "levels": [f"LVL_{slug.upper()}"],
            "characters": ["CHR_ERTUGRUL"],

            "environment": {
                "season": env_season, "time_of_day": tod,
                "weather": weather, "region": region,
            },
            "audio": {
                "music_cue":    f"MUS_{eid}",
                "ambience_tag": f"Ambience.{env_season.title()}.{weather.title()}",
                "silence_scene": silence,
            },
            "telemetry": {
                "funnel_id": f"funnel_{eid.lower()}",
                "target_completion_pct": round(max(18.0, 96.0 - i * 1.55), 1),
                "watch_metrics": ["deaths", "duration", "abandon_point"] +
                                 (["hand_integrity", "sabr"] if phase != "INTACT" else []),
            },
            "content": {
                "warnings": warnings,
                "has_torture_scene": torture,
                **({"skippable_scene_id": f"SCENE_{eid}_TORTURE"} if torture else {}),
            },
            "retention_hook": f"ep.{eid.lower()}.teaser",
        }

        if conf != "DOCUMENTED":
            ep["anchor"]["scholar_note"] = SCHOLAR_NOTES.get(eid,
                "Manbalar ziddiyatli — Kodeksda to'liq muhokama qilinadi.")

        # Shubha sahnasi tanlovi
        for ss_id, ss_ep, ss_q, ss_opts, _ in UNCERTAINTY:
            if ss_ep == eid:
                ep["choices"] = [{
                    "id": ss_id,
                    "loc_key_prompt": ss_q,
                    "is_uncertainty_scene": True,
                    "weight": "IRREVERSIBLE",
                    "options": [{
                        "id": o[0], "loc_key": o[1],
                        "result_flags": [f"Choice.{ss_id}.{o[0]}"],
                        "codex_unlocks": o[3],
                    } for o in ss_opts],
                }]
        eps.append(ep)

    # Budjetlar
    arch_budget = collections.Counter(e["archetype"] for e in eps)
    trav_budget = collections.Counter(e["traversal"][0] for e in eps)

    for s_i, s in enumerate(SEASONS):
        s["episode_ids"] = [f"EP{n:03d}" for n in range(s_i * 12 + 1, s_i * 12 + 13)]

    return {
        "schema_version": 2,
        "game_id": "dirilis_last_march",
        "generated_at": datetime.datetime.now(datetime.timezone.utc)
                        .replace(microsecond=0).isoformat().replace("+00:00", "Z"),
        "timeline": {
            "start_year_gregorian": 1227,
            "end_year_gregorian": 1261,
            "note": "v1 (1225-1226) dan ko'chirildi: Bayju Noyan 1241-da tayinlangan, "
                    "Köse Dağ 1243, Köpek terrori 1237-38, Ibn Arabiy 1223-dan Damashqda. "
                    "Bu ko'chirish 11 ta anaxronizmning 9 tasini hal qiladi.",
        },
        "seasons": [{
            "id": s["id"], "index": s["index"], "loc_key_title": s["loc"],
            "codename": s["codename"], "year_from": s["y0"], "year_to": s["y1"],
            "setting": s["setting"], "tier_range": s["tiers"], "boss": s["boss"],
            "region": s["region"], "new_enemy_archetype": s["new_enemy"],
            "world_change": s["world"], "episode_ids": s["episode_ids"],
        } for s in SEASONS],
        "episodes": eps,
        "side_quest_chains": [{
            "id": c[0], "giver": c[1], "loc_key_summary": c[2],
            "stages": [{"stage": i + 1, "episode_gate": g, "loc_key": f"{c[2]}.s{i+1}"}
                       for i, g in enumerate(c[3])],
            "affects_main_story": c[4], "reward_tags": c[5],
        } for c in CHAINS],
        "uncertainty_scenes": [{
            "id": u[0], "episode_id": u[1], "loc_key_question": u[2],
            "options": [{"id": o[0], "loc_key": o[1], "confidence": o[2],
                         "codex_unlocks": o[3], "scholar_note": o[4]} for o in u[3]],
            "affects": u[4],
        } for u in UNCERTAINTY],
        "archetype_budget": dict(arch_budget),
        "traversal_budget": dict(trav_budget),
    }


SCHOLAR_NOTES = {
 "EP004": "Usmon tangalarida 'Gündüz Alp' yozilgan; XV asr xronikalarida 'Sulaymon Shoh'. TDV ikkinchisini noto'g'ri deb baholaydi.",
 "EP015": "Karacadağ/Söğüt in'omi zamondosh hujjatda yo'q; faqat XV asr xronikalarida. Transhumans amaliyoti esa hujjatli.",
 "EP018": "Domaniç yaylovi Kayı bilan bog'lanishi keyingi an'ana; ko'chmanchi transhumans esa etnografik jihatdan tasdiqlangan.",
 "EP026": "Hoji Bektosh Vali hayoti afsonalar bilan qoplangan; Xuroson migratsiyasi va Kirshahrda o'rnashgani qabul qilingan.",
 "EP033": "Köse Dağ qo'shin raqamlari manbalarda keskin farq qiladi: zamonaviy ~80,000 vs xronika 160-200,000.",
 "EP039": "Söğütning Kayı bilan bog'lanishi XVI asr tahrir defterlarida qayd etilgan; XIII asr hujjati yo'q.",
 "EP041": "Karacahisar: TDV Ertug'rul/1231-32, xronikalar Usmon/1288, zamonaviy tadqiqot germiyan versiyasini ilgari suradi.",
 "EP042": "Usmonning tug'ilgan sanasi noma'lum (~1254-1258).",
 "EP047": "Bayju Noyan ~1258-1260 orasida, ehtimol Hulagu buyrug'i bilan vafot etgan. Ertug'rul bilan duel — badiiy.",
 "EP012": "Qayi shajarasi ~1420-30 da yozilgan.",
}


# ═══════════════════════════════════════════════════════════════════════════
#  VALIDATSIYA — CI/CD da build'ni to'xtatadi
# ═══════════════════════════════════════════════════════════════════════════
def validate(doc: dict) -> list[str]:
    errs, warns = [], []
    eps = doc["episodes"]

    # 1) Soni
    if len(eps) != 48:
        errs.append(f"48 epizod kerak, {len(eps)} ta topildi")

    # 2) ID va indeks izchilligi (v1 dagi off-by-one bug)
    for i, e in enumerate(eps):
        if e["id"] != f"EP{i+1:03d}":
            errs.append(f"[{i}] ID noto'g'ri: {e['id']}")
        if e["global_index"] != i:
            errs.append(f"{e['id']}: global_index {e['global_index']} != {i}")
        if e["season_index"] != i % 12:
            errs.append(f"{e['id']}: season_index noto'g'ri")
        if e["season_id"] != f"S{i//12+1}":
            errs.append(f"{e['id']}: season_id noto'g'ri")

    # 3) Xronologiya
    for i in range(1, len(eps)):
        a, b = eps[i-1]["anchor"]["year_gregorian"], eps[i]["anchor"]["year_gregorian"]
        if b < a:
            errs.append(f"{eps[i]['id']}: sana orqaga ketdi ({a} → {b})")

    # 4) ⭐ Mix beat — HAR epizodda majburiy (foydalanuvchi talabi)
    for e in eps:
        if not e.get("mih_beat", {}).get("kind"):
            errs.append(f"{e['id']}: mih_beat yo'q — har epizodda mix ko'rinishi kerak")

    # 5) ⭐ MaxIntegrity monotonik pasayishi (qo'l hech qachon tuzalmaydi)
    prev_max = 100.0
    for e in eps:
        m = e["wound_state"]["max_integrity"]
        if e["id"] == "EP043":       # yagona ruxsat etilgan ko'tarilish (protez)
            prev_max = m
            continue
        if m > prev_max:
            errs.append(f"{e['id']}: max_integrity ko'tarildi ({prev_max} → {m}) — "
                        f"faqat EP043 (protez) da ruxsat")
        prev_max = m

    # 6) ⭐ Arxetip 3 marta ketma-ket takrorlanmasin
    for i in range(2, len(eps)):
        if eps[i]["archetype"] == eps[i-1]["archetype"] == eps[i-2]["archetype"]:
            errs.append(f"{eps[i]['id']}: arxetip 3 marta ketma-ket ({eps[i]['archetype']})")

    # 7) ⭐ OT MONOPOLIYASI (foydalanuvchi talabi)
    horse = sum(1 for e in eps if e["traversal"][0] == "HORSE")
    pct = horse / len(eps) * 100
    if horse > 12:
        errs.append(f"Ot {horse} epizodda asosiy ({pct:.0f}%) — maksimum 12 (25%)")

    # 8) Dushman soni balansi
    for e in eps:
        ms = e["enemy_composition"]["max_simultaneous"]
        if ms > 8:
            errs.append(f"{e['id']}: max_simultaneous {ms} > 8 — o'lim tuzog'i")

    # 9) Kontent ogohlantirishi
    for e in eps:
        if e["content"]["has_torture_scene"] and not e["content"]["warnings"]:
            errs.append(f"{e['id']}: qiynoq sahnasi bor, ogohlantirish yo'q — sertifikatsiya rad etadi")

    # 10) BAHSLI/RIVOYAT uchun scholar_note majburiy
    for e in eps:
        a = e["anchor"]
        if a["confidence"] != "DOCUMENTED" and not a.get("scholar_note"):
            errs.append(f"{e['id']}: {a['confidence']} uchun scholar_note majburiy")

    # 11) O'ynaladigan intro (mp4 yo'qligi)
    for e in eps:
        if "intro_video" in e:
            errs.append(f"{e['id']}: intro_video maydoni bo'lmasligi kerak — o'ynaladigan intro")
        if not (90 <= e["intro"]["duration_sec"] <= 400):
            errs.append(f"{e['id']}: intro davomiyligi 90-400 sek bo'lishi kerak")

    # 12) Shubha sahnalari epizodlarga bog'langan
    ep_ids = {e["id"] for e in eps}
    for u in doc["uncertainty_scenes"]:
        if u["episode_id"] not in ep_ids:
            errs.append(f"{u['id']}: epizod topilmadi ({u['episode_id']})")

    # 13) Side quest zanjirlari
    for c in doc["side_quest_chains"]:
        gates = [s["episode_gate"] for s in c["stages"]]
        if gates != sorted(gates):
            errs.append(f"{c['id']}: bosqichlar xronologik emas")
        for g in gates:
            if g not in ep_ids:
                errs.append(f"{c['id']}: epizod topilmadi ({g})")

    # 14) Dangling prerequisite / unlock
    for e in eps:
        for p in e["prerequisites"] + e["unlocks"]:
            if p not in ep_ids:
                errs.append(f"{e['id']}: dangling reference {p}")

    # ── Ogohlantirishlar (build'ni to'xtatmaydi) ──
    dur = sum(e["estimated_minutes"] for e in eps)
    if not (2200 <= dur <= 3200):
        warns.append(f"Umumiy davomiylik {dur/60:.1f} soat — maqsad 40-53 soat")

    codex_all = [c for e in eps for c in e["codex_unlocks"]]
    if len(codex_all) != len(set(codex_all)):
        dupes = [c for c, n in collections.Counter(codex_all).items() if n > 1]
        warns.append(f"Takrorlangan kodeks ochilishi: {dupes}")

    for w in warns:
        print(f"  ⚠️  {w}")
    return errs


def report(doc: dict) -> None:
    eps = doc["episodes"]
    print("\n" + "═" * 68)
    print("  EPISODES_V2.JSON — HISOBOT")
    print("═" * 68)
    print(f"  Epizodlar          : {len(eps)}")
    print(f"  Mavsumlar          : {len(doc['seasons'])}")
    print(f"  Timeline           : {doc['timeline']['start_year_gregorian']}"
          f"–{doc['timeline']['end_year_gregorian']} "
          f"({doc['timeline']['end_year_gregorian']-doc['timeline']['start_year_gregorian']} yil)")
    print(f"  Umumiy davomiylik  : {sum(e['estimated_minutes'] for e in eps)/60:.1f} soat "
          f"(+ ~{sum(e['intro']['duration_sec'] for e in eps)/3600:.1f} soat o'ynaladigan intro)")
    print(f"  Kodeks ochilishlari: {len({c for e in eps for c in e['codex_unlocks']})}")
    print(f"  Shubha sahnalari   : {len(doc['uncertainty_scenes'])}")
    print(f"  Side-quest zanjiri : {len(doc['side_quest_chains'])}")

    print("\n  ── ARXETIP TAQSIMOTI ──")
    for k, v in sorted(doc["archetype_budget"].items(), key=lambda x: -x[1]):
        print(f"    {k:<14} {v:>2}  {'█'*v}")

    print("\n  ── ASOSIY HARAKAT TURI (ot monopoliyasi tekshiruvi) ──")
    tot = len(eps)
    for k, v in sorted(doc["traversal_budget"].items(), key=lambda x: -x[1]):
        mark = "  ⭐" if k == "HORSE" else ""
        print(f"    {k:<18} {v:>2}  ({v/tot*100:>4.1f}%) {'█'*v}{mark}")

    print("\n  ── ISHONCHLILIK ──")
    cc = collections.Counter(e["anchor"]["confidence"] for e in eps)
    for k in ("DOCUMENTED", "DISPUTED", "LEGEND"):
        print(f"    {k:<12} {cc.get(k,0):>2}  ({cc.get(k,0)/tot*100:>4.1f}%)")

    print("\n  ── QIYINLIK EGRI CHIZIG'I ──")
    for s in doc["seasons"]:
        tiers = [e["difficulty_tier"] for e in eps if e["season_id"] == s["id"]]
        print(f"    {s['id']} {s['codename']:<16} {''.join(str(t) for t in tiers)}"
              f"   (o'rtacha {sum(tiers)/len(tiers):.1f})")

    print("\n  ── MIX (MIH) BEAT TURLARI ──")
    mc = collections.Counter(e["mih_beat"]["kind"] for e in eps)
    for k, v in sorted(mc.items(), key=lambda x: -x[1]):
        print(f"    {k:<14} {v:>2}")
    print(f"    {'JAMI':<14} {sum(mc.values()):>2}  ← har epizodda mix bor ✅")
    print("═" * 68 + "\n")


if __name__ == "__main__":
    doc = build()
    print("\n🔍 Validatsiya...")
    errors = validate(doc)
    if errors:
        print(f"\n❌ {len(errors)} XATO:")
        for e in errors:
            print(f"  • {e}")
        sys.exit(1)
    print("  ✅ Barcha tekshiruvlar o'tdi")

    if "--check" not in sys.argv:
        OUT.write_text(json.dumps(doc, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"  ✅ Yozildi: {OUT}  ({OUT.stat().st_size/1024:.0f} KB)")
    report(doc)
