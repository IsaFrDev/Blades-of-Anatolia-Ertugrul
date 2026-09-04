# -*- coding: utf-8 -*-
import json, io
D = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
EP = json.load(io.open('D:/My_apps/Ertugrul/data/episodes/episodes_v2.json', encoding='utf-8'))['episodes']
L = []
def loc(k, uz, tr, en): L.append('"%s","%s","%s","%s"' % (k, uz.replace('"', "'"), tr.replace('"', "'"), en.replace('"', "'")))
# Arxetip shablonlari: (gapiruvchi nom kaliti, gapiruvchi ko'rsatma nomi, ayolmi, kaftan, tugunlar)
T = {}
loc('DLG_T_COURT_01', "Bey, saroyda sizga qarshi so'z yuribdi. Dalilingiz bormi?", "Bey, sarayda size karşı söz dolaşıyor. Deliliniz var mı?", "Bey, there is talk against you at court. Have you proof?")
loc('DLG_T_COURT_A', "Mana birinchi dalil.", "İşte ilk delil.", "Here is the first proof.")
loc('DLG_T_COURT_B', "Mana ikkinchi dalil.", "İşte ikinci delil.", "Here is the second proof.")
loc('DLG_T_COURT_NONE', "Hali yo'q.", "Henüz yok.", "Not yet.")
loc('DLG_T_COURT_WIN', "Yetarli. Saroy sizning tarafingizda.", "Yeterli. Saray sizin tarafınızda.", "Enough. The court stands with you.")
loc('DLG_T_COURT_LOSE', "Dalilsiz so'z - shamol. Ehtiyot bo'ling.", "Delilsiz söz rüzgârdır. Dikkatli olun.", "Words without proof are wind. Be careful.")
T['COURT'] = ('chr.alauddin.name', 'Sulton Alauddin Kayqubod', False, [0.45, 0.35, 0.10], 2, {
    'a': {'k': 'DLG_T_COURT_01', 'opts': [('DLG_T_COURT_A', 'a', 'clue_a', 1, 2), ('DLG_T_COURT_B', 'w', 'clue_b', 1, 2), ('DLG_T_COURT_NONE', 'l', None, 0, 0)]},
    'w': {'k': 'DLG_T_COURT_WIN', 'flag': 'court_won'}, 'l': {'k': 'DLG_T_COURT_LOSE'}})
T['INVESTIGATION'] = ('chr.dogan.name', "Dog'on Alp", False, [0.22, 0.18, 0.30], 2, {
    'a': {'k': 'DLG_T_INV_01', 'opts': [('DLG_T_INV_A', 'a', 'clue_a', 1, 0), ('DLG_T_INV_B', 'w', 'clue_b', 1, 0), ('DLG_T_COURT_NONE', 'l', None, 0, 0)]},
    'w': {'k': 'DLG_T_INV_WIN', 'flag': 'case_solved'}, 'l': {'k': 'DLG_T_INV_LOSE'}})
loc('DLG_T_INV_01', "Bey, nima topding? Izlar sovumasdan gapir.", "Bey, ne buldun? İzler soğumadan söyle.", "Bey, what did you find? Speak before the trail goes cold.")
loc('DLG_T_INV_A', "Izlar - shimolga, otliq.", "İzler kuzeye, atlı.", "Tracks - north, a rider.")
loc('DLG_T_INV_B', "Mana dalil - dushmanning belgisi.", "İşte delil - düşmanın nişanı.", "Here is proof - the enemy's mark.")
loc('DLG_T_INV_WIN', "Demak ular... Yigitlarni yig'aman.", "Demek onlar... Yiğitleri toplarım.", "So it is them... I gather the men.")
loc('DLG_T_INV_LOSE', "Kam. Yana qara, bey.", "Az. Yine bak bey.", "Too little. Look again, bey.")
loc('DLG_T_SIEGE_01', "Turk! Darvozani och - No'yon rahm qiladi. Yopiq qolsa - o't.", "Türk! Kapıyı aç - Noyan merhamet eder. Kapalı kalırsa ateş.", "Turk! Open the gate and Noyan shows mercy. Keep it shut, and fire.")
loc('DLG_T_SIEGE_REFUSE', "Qayilar tiz cho'kmaydi.", "Kayılar diz çökmez.", "The Kayi do not kneel.")
loc('DLG_T_SIEGE_TIME', "Tonggacha vaqt ber.", "Şafağa dek süre ver.", "Give me until dawn.")
loc('DLG_T_SIEGE_02', "Unda devorlaring qabring bo'ladi.", "O halde duvarların mezarın olur.", "Then your walls shall be your grave.")
loc('DLG_T_SIEGE_03', "Tonggacha. Keyin - o't.", "Şafağa dek. Sonra ateş.", "Until dawn. Then fire.")
T['SIEGE'] = ('chr.mongol_chavandoz.name', "Mo'g'ul chavandozi", False, [0.30, 0.10, 0.08], 0, {
    'a': {'k': 'DLG_T_SIEGE_01', 'opts': [('DLG_T_SIEGE_REFUSE', 'b', None, 0, 5), ('DLG_T_SIEGE_TIME', 'c', None, 0, 0)]},
    'b': {'k': 'DLG_T_SIEGE_02', 'flag': 'siege_defied'}, 'c': {'k': 'DLG_T_SIEGE_03', 'flag': 'siege_delay'}})
loc('DLG_T_DEF_01', "Bey, devorda odam kam. Kimni qayerga qo'yamiz?", "Bey, surda adam az. Kimi nereye koyalım?", "Bey, few men on the wall. Where do we place them?")
loc('DLG_T_DEF_GATE', "Hammasi darvozaga. U yerdan kelishadi.", "Hepsi kapıya. Oradan gelirler.", "Everyone to the gate. That is where they come.")
loc('DLG_T_DEF_SPREAD', "Kamonchilar minoraga, qolgani hovliga.", "Okçular kuleye, kalanı avluya.", "Archers to the tower, the rest to the yard.")
loc('DLG_T_DEF_02', "Bo'pti. Darvoza bizniki.", "Tamam. Kapı bizim.", "Done. The gate is ours.")
loc('DLG_T_DEF_03', "Oqilona. O'qlar tayyor.", "Akıllıca. Oklar hazır.", "Wise. Arrows ready.")
T['DEFENSE'] = ('chr.turgut.name', 'Turgut Alp', False, [0.20, 0.22, 0.12], 0, {
    'a': {'k': 'DLG_T_DEF_01', 'opts': [('DLG_T_DEF_GATE', 'b', None, 0, 1), ('DLG_T_DEF_SPREAD', 'c', None, 0, 2)]},
    'b': {'k': 'DLG_T_DEF_02', 'flag': 'defend_gate'}, 'c': {'k': 'DLG_T_DEF_03', 'flag': 'defend_spread'}})
loc('DLG_T_SURV_01', "Bey, suv tugadi, yigitlar holdan toydi. Nima qilamiz?", "Bey, su bitti, yiğitler bitkin. Ne yapalım?", "Bey, the water is gone and the men are spent. What do we do?")
loc('DLG_T_SURV_PUSH', "Yuramiz. To'xtasak o'lamiz.", "Yürüyoruz. Durursak ölürüz.", "We move. If we stop, we die.")
loc('DLG_T_SURV_REST', "Dam olamiz, keyin ov.", "Dinlenir, sonra avlanırız.", "We rest, then hunt.")
loc('DLG_T_SURV_02', "Sen bilan oxirigacha, bey.", "Sonuna kadar seninleyiz bey.", "With you to the end, bey.")
loc('DLG_T_SURV_03', "Yaxshi. Olov yoqaman.", "İyi. Ateş yakarım.", "Good. I will light a fire.")
T['SURVIVAL'] = ('chr.bamsi.name', 'Bamsi Bayrak', False, [0.36, 0.26, 0.10], 0, {
    'a': {'k': 'DLG_T_SURV_01', 'opts': [('DLG_T_SURV_PUSH', 'b', None, 0, 2), ('DLG_T_SURV_REST', 'c', None, 0, 1)]},
    'b': {'k': 'DLG_T_SURV_02'}, 'c': {'k': 'DLG_T_SURV_03', 'flag': 'camp_fire'}})
loc('DLG_T_INF_01', "...O'ldirmang! Men faqat kotibman. Nima kerak sizga?", "...Öldürmeyin! Ben sadece kâtibim. Ne istiyorsunuz?", "...Do not kill me! I am only a scribe. What do you want?")
loc('DLG_T_INF_ASK', "Qo'mondon qayerda? Gapir - yashaysan.", "Komutan nerede? Konuş, yaşarsın.", "Where is the commander? Speak and live.")
loc('DLG_T_INF_KILL', "Gapirmasang - qilich gapiradi.", "Konuşmazsan kılıç konuşur.", "If you will not speak, the sword will.")
loc('DLG_T_INF_02', "Katta chodirda... shimolda. Rahmat, turk.", "Büyük çadırda... kuzeyde. Sağ ol Türk.", "In the great tent... to the north. Thank you, Turk.")
loc('DLG_T_INF_03', "...Katta chodir, shimolda! Iltimos...", "...Büyük çadır, kuzeyde! Lütfen...", "...The great tent, north! Please...")
T['INFILTRATION'] = ('chr.mongol_katib.name', "Mo'g'ul kotibi", False, [0.25, 0.25, 0.30], 0, {
    'a': {'k': 'DLG_T_INF_01', 'opts': [('DLG_T_INF_ASK', 'b', None, 0, 3), ('DLG_T_INF_KILL', 'c', None, 0, -3)]},
    'b': {'k': 'DLG_T_INF_02', 'flag': 'scribe_spared'}, 'c': {'k': 'DLG_T_INF_03', 'flag': 'scribe_threatened'}})
loc('DLG_T_CHASE_01', "Bey... ular daryo tomon ketdi. Meni qoldir, quv!", "Bey... nehre doğru gittiler. Beni bırak, kovala!", "Bey... they went toward the river. Leave me, give chase!")
loc('DLG_T_CHASE_GO', "Chidab tur, qaytaman.", "Dayan, dönerim.", "Hold on, I will return.")
loc('DLG_T_CHASE_STAY', "Avval yarangni bog'layman.", "Önce yaranı sararım.", "First I bind your wound.")
loc('DLG_T_CHASE_02', "Bor! Vaqt yo'q!", "Git! Vakit yok!", "Go! No time!")
loc('DLG_T_CHASE_03', "...Rahmat, bey.", "...Sağ ol bey.", "...Thank you, bey.")
T['CHASE'] = ('chr.dundar.name', 'Dundor', False, [0.18, 0.28, 0.20], 0, {
    'a': {'k': 'DLG_T_CHASE_01', 'opts': [('DLG_T_CHASE_GO', 'b', None, 0, -1), ('DLG_T_CHASE_STAY', 'c', None, 0, 4)]},
    'b': {'k': 'DLG_T_CHASE_02'}, 'c': {'k': 'DLG_T_CHASE_03', 'flag': 'ally_saved'}})
loc('DLG_T_ESC_01', "Alp, karvon sizga ishonadi. Yo'l xavfli - qaysi yo'ldan yuramiz?", "Alp, kervan size güvenir. Yol tehlikeli - hangi yoldan gideriz?", "Alp, the caravan trusts you. The road is dangerous - which way do we go?")
loc('DLG_T_ESC_ROAD', "Katta yo'ldan, tez.", "Ana yoldan, hızlı.", "The main road, fast.")
loc('DLG_T_ESC_HILLS', "Tepaliklardan, yashirin.", "Tepelerden, gizli.", "Through the hills, hidden.")
loc('DLG_T_ESC_02', "Tez bo'lsin. Otlar tayyor.", "Hızlı olsun. Atlar hazır.", "Fast then. The horses are ready.")
loc('DLG_T_ESC_03', "Uzoq, lekin xavfsiz. Yuramiz.", "Uzun ama güvenli. Yürüyoruz.", "Long but safe. We go.")
T['ESCORT'] = ('chr.ahi_evren.name', 'Axi Evren', False, [0.50, 0.42, 0.22], 0, {
    'a': {'k': 'DLG_T_ESC_01', 'opts': [('DLG_T_ESC_ROAD', 'b', None, 0, 0), ('DLG_T_ESC_HILLS', 'c', None, 0, 2)]},
    'b': {'k': 'DLG_T_ESC_02', 'flag': 'route_road'}, 'c': {'k': 'DLG_T_ESC_03', 'flag': 'route_hills'}})
loc('DLG_T_RIT_01', "O'g'lim, yuraging nima deydi? Qilich yo sabr?", "Evladım, kalbin ne der? Kılıç mı sabır mı?", "My son, what does your heart say? The sword or patience?")
loc('DLG_T_RIT_SWORD', "Qilich. Zolim so'zdan tushunmaydi.", "Kılıç. Zalim sözden anlamaz.", "The sword. The tyrant does not understand words.")
loc('DLG_T_RIT_PATIENCE', "Sabr. Avval haqiqatni izlayman.", "Sabır. Önce hakikati ararım.", "Patience. First I seek the truth.")
loc('DLG_T_RIT_02', "Qilich ham haq bo'lishi mumkin - agar qalb toza bo'lsa.", "Kılıç da hak olabilir - kalp temizse.", "The sword too can be just - if the heart is clean.")
loc('DLG_T_RIT_03', "Haqiqatni izlagan adashmaydi. Yo'ling ochiq.", "Hakikati arayan şaşmaz. Yolun açık.", "He who seeks truth does not go astray. Your road is open.")
T['RITUAL'] = ('chr.haci_bektas.name', 'Hoji Bektosh', False, [0.85, 0.82, 0.75], 0, {
    'a': {'k': 'DLG_T_RIT_01', 'opts': [('DLG_T_RIT_SWORD', 'b', None, 0, 1), ('DLG_T_RIT_PATIENCE', 'c', None, 0, 5)]},
    'b': {'k': 'DLG_T_RIT_02'}, 'c': {'k': 'DLG_T_RIT_03', 'flag': 'blessing'}})

M = json.load(io.open(D + 'missions.json', encoding='utf-8'))
n = 0
for e in EP[12:48]:
    a = e['archetype']; eid = e['id']; tier = e['difficulty_tier']
    if a not in T: continue
    name_key, spk, woman, kaftan, thr, nodes = T[a]
    did = eid.lower() + '_talk'
    g = {'id': did, 'start': 'a', 'duel_threshold': thr, 'nodes': {}}
    for nid, nd in nodes.items():
        node = {'speaker': spk, 'text_key': nd['k']}
        if 'opts' in nd:
            node['options'] = []
            for (tk, nxt, ev, dp, hon) in nd['opts']:
                o = {'text_key': tk, 'next': nxt, 'honor': hon}
                if ev: o['requires_evidence'] = eid.lower() + '_' + ev; o['duel_points'] = dp
                node['options'].append(o)
        else: node['next'] = 'end'
        if 'flag' in nd: node['set_flag'] = eid.lower() + '_' + nd['flag']
        g['nodes'][nid] = node
    io.open(D + 'dialogue/' + did + '.json', 'w', encoding='utf-8').write(json.dumps(g, ensure_ascii=False, indent=1))
    npc = [{'id': eid.lower() + '_npc', 'name': name_key, 'u': -6, 'v': -7, 'yaw': 180, 'woman': woman, 'kaftan': kaftan}]
    council = {'type': 'council', 'dialog': did, 'threshold': thr, 'npcs': npc}
    fw = 1 if tier <= 1 else (3 if tier >= 4 else 2)
    if a in ('COURT', 'INVESTIGATION'):
        ph = [{'type': 'travel', 'n': 2}, {'type': 'collect', 'n': 2, 'flags': [eid.lower() + '_clue_a', eid.lower() + '_clue_b'], 'loc': 'ui.obj.collect_evidence'}, council, {'type': 'stealth', 'guards': 2 + (tier >= 4)}, {'type': 'fight', 'waves': 1}]
    elif a == 'SIEGE':
        ph = [council, {'type': 'travel', 'n': 2, 'timed': True}, {'type': 'defend', 'waves': 2}, {'type': 'fight', 'waves': fw}]
    elif a == 'DEFENSE':
        ph = [council, {'type': 'defend', 'waves': 2}, {'type': 'travel', 'n': 2}, {'type': 'fight', 'waves': fw}]
    elif a == 'SURVIVAL':
        ph = [{'type': 'hunt', 'n': 2}, council, {'type': 'travel', 'n': 3}, {'type': 'fight', 'waves': fw}]
    elif a == 'INFILTRATION':
        ph = [{'type': 'travel', 'n': 2}, {'type': 'stealth', 'guards': 3}, council, {'type': 'collect', 'n': 2}, {'type': 'travel', 'n': 2, 'timed': True}]
    elif a == 'CHASE':
        ph = [{'type': 'travel', 'n': 3, 'timed': True}, {'type': 'fight', 'waves': 1}, council, {'type': 'travel', 'n': 2, 'timed': True}, {'type': 'duel'}]
    elif a == 'ESCORT':
        ph = [council, {'type': 'travel', 'n': 2}, {'type': 'defend', 'waves': 2}, {'type': 'travel', 'n': 2}, {'type': 'fight', 'waves': 1}]
    else:  # RITUAL
        ph = [{'type': 'collect', 'n': 3}, council, {'type': 'hunt', 'n': 1}, {'type': 'travel', 'n': 2}, {'type': 'duel'}]
    M[eid] = {'phases': ph}; n += 1
io.open(D + 'missions.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(M, ensure_ascii=False, indent=1))
s = io.open(D + 'npc_loc.csv', encoding='utf-8').read()
for row in L:
    if row.split(',')[0] not in s: s += row + '\n'
io.open(D + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(s)
print('episodes', n, 'loc', len(L), 'total missions', len([k for k in M if k.startswith('EP')]))
