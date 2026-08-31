#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_episodes.py — `episodes_v2.json` generatori va validatori.
"""
from __future__ import annotations
import json, sys, datetime, collections
from pathlib import Path

OUT   = Path(__file__).parent / "episodes_v2.json"
SCHEMA= Path(__file__).parent / "schema" / "episode.schema.json"

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

D, DI, L = "DOCUMENTED", "DISPUTED", "LEGEND"

EPISODES = [
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

UNCERTAINTY = []
CHAINS = []
SCHOLAR_NOTES = {}

def parry_window(hp: float, phase: str) -> float:
    if phase == "INTACT": return 180.0
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
            "loc_key_title": f"ep.{eid.lower()}.title",
            "loc_key_synopsis": f"ep.{eid.lower()}.synopsis",
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
            "objectives": [],
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
            "fail_conditions": ["Fail.PlayerDeath"],
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
                "watch_metrics": ["deaths", "duration", "abandon_point"]
            },
            "content": {
                "warnings": warnings,
                "has_torture_scene": torture,
            },
            "retention_hook": f"ep.{eid.lower()}.teaser",
        }
        eps.append(ep)

    for s_i, s in enumerate(SEASONS):
        s["episode_ids"] = [f"EP{n:03d}" for n in range(s_i * 12 + 1, s_i * 12 + 13)]

    return {
        "schema_version": 2,
        "game_id": "dirilis_last_march",
        "generated_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "timeline": {
            "start_year_gregorian": 1227,
            "end_year_gregorian": 1261,
            "note": "v2"
        },
        "seasons": SEASONS,
        "episodes": eps,
    }

if __name__ == "__main__":
    doc = build()
    OUT.write_text(json.dumps(doc, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Generated {len(doc['episodes'])} episodes at {OUT}")
