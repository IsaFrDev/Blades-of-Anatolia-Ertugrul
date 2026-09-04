# -*- coding: utf-8 -*-
import json, io
D = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
L = []
def loc(k, uz, tr, en): L.append('"%s","%s","%s","%s"' % (k, uz.replace('"', "'"), tr.replace('"', "'"), en.replace('"', "'")))

# id, giver npc id, title uz/tr/en, giver line uz/tr/en, done line, phases, reward
Q = [
 ('sq_aykiz_tears', 'aykiz', ("Oyqizning ko'z yoshi", "Aykız'ın gözyaşı", "Aykiz's tears"),
  ("Bey, Kurdo'g'li Turgutni ayblayapti! U aybsiz - dalil toping, iltimos.", "Bey, Kurdoğlu Turgut'u suçluyor! O suçsuz - delil bulun lütfen.", "Bey, Kurdoglu accuses Turgut! He is innocent - find proof, please."),
  ("Rahmat, bey... Turgut ozod!", "Sağ ol bey... Turgut özgür!", "Thank you, bey... Turgut is free!"),
  [{'type': 'collect', 'n': 3, 'flags': ['sq_ay_a', 'sq_ay_b', 'sq_ay_c'], 'loc': 'ui.obj.collect_evidence'}, {'type': 'stealth', 'guards': 2}], (120, 40, 4)),
 ('sq_claudius_to_omar', 'ibn_arabi_np', ("Klavdiydan Umargacha", "Claudius'tan Ömer'e", "From Claudius to Omar"),
  ("O'g'lim, asir Templar ritsari o'limga hukm qilingan. Uni qutqar - u haqiqatni izlayapti.", "Evladım, esir Tapınakçı şövalyesi ölüme mahkûm. Onu kurtar - hakikati arıyor.", "My son, a captive Templar knight is condemned. Save him - he seeks the truth."),
  ("Endi uning ismi Umar. Yo'ling ochiq.", "Artık adı Ömer. Yolun açık.", "His name is Omar now. Your road is open."),
  [{'type': 'travel', 'n': 2}, {'type': 'stealth', 'guards': 3}, {'type': 'travel', 'n': 2, 'timed': True}], (150, 30, 8)),
 ('sq_wolf_hunt', 'deli_demir', ("Bo'ri ovi", "Kurt avı", "Wolf hunt"),
  ("Bey, bo'rilar podani qiryapti. Tunda poyla, terisini olib kel - zirh yasab beraman.", "Bey, kurtlar sürüyü kırıyor. Gece bekle, derisini getir - zırh yaparım.", "Bey, wolves are killing the herd. Watch by night, bring the pelts - I will make armour."),
  ("Mana teri! Zirhing endi qalinroq.", "İşte deri! Zırhın artık daha kalın.", "The pelts! Your armour is thicker now."),
  [{'type': 'defend', 'waves': 2}, {'type': 'hunt', 'n': 2}], (130, 50, 2)),
 ('sq_aleppo_caravan', 'gundogdu', ("Halab karvoni", "Halep kervanı", "The Aleppo caravan"),
  ("Uka, savdo karvoni Halabga ketmoqda. Himoya qil - oltin va obro' bo'ladi.", "Kardeşim, ticaret kervanı Halep'e gidiyor. Koru - altın ve itibar olur.", "Brother, a trade caravan leaves for Aleppo. Guard it - gold and repute await."),
  ("Karvon yetib bordi. Mana ulushing.", "Kervan ulaştı. İşte payın.", "The caravan arrived. Here is your share."),
  [{'type': 'travel', 'n': 2}, {'type': 'defend', 'waves': 2}, {'type': 'travel', 'n': 2}], (140, 90, 3)),
 ('sq_trail_of_eftelya', 'afsin_bey', ("Eftelyaning izi", "Eftelya'nın izi", "Eftelya's trail"),
  ("Bey, obada josus ayol bor deyishadi. Uni sezdirmay kuzat.", "Bey, obada casus bir kadın varmış. Fark ettirmeden izle.", "Bey, they say there is a spy woman in the tribe. Follow her unseen."),
  ("Demak Eftelya... Endi bilamiz.", "Demek Eftelya... Artık biliyoruz.", "So, Eftelya... Now we know."),
  [{'type': 'stealth', 'guards': 2}, {'type': 'collect', 'n': 2}, {'type': 'stealth', 'guards': 2}], (150, 40, 3)),
 ('sq_selcan_secrets', 'selcan', ("Selchanning sirlari", "Selcan'ın sırları", "Selcan's secrets"),
  ("Ertug'rul... men xato qildim. Tavba yo'lida menga yordam ber - Kurdo'g'lining xatlarini top.", "Ertuğrul... hata yaptım. Tövbe yolunda bana yardım et - Kurdoğlu'nun mektuplarını bul.", "Ertugrul... I erred. Help me on the path of repentance - find Kurdoglu's letters."),
  ("Rahmat. Endi vijdonim tinch.", "Sağ ol. Artık vicdanım rahat.", "Thank you. My conscience is at peace."),
  [{'type': 'collect', 'n': 3}, {'type': 'travel', 'n': 2}], (100, 20, 6)),
 ('sq_goncagul_trap', 'banu_cicek', ("Go'nchagulning tuzog'i", "Goncagül'ün tuzağı", "Goncagul's trap"),
  ("Bey, Dodurga obasida fitna pishmoqda. Dalillarni top, keyin fitnachilarni to'xtat.", "Bey, Dodurga obasında fitne pişiyor. Delilleri bul, sonra fitnecileri durdur.", "Bey, a plot brews in the Dodurga tribe. Find proof, then stop the plotters."),
  ("Fitna fosh bo'ldi. Obalar tinch.", "Fitne açığa çıktı. Obalar huzurlu.", "The plot is exposed. The tribes are at peace."),
  [{'type': 'collect', 'n': 3}, {'type': 'fight', 'waves': 1}], (160, 60, 4)),
 ('sq_geyikli_wisdom', 'geyikli_baba', ("Geyikli Bobo hikmati", "Geyikli Baba'nın hikmeti", "Geyikli Baba's wisdom"),
  ("O'tir, bola. Yuragingni tingla. Uch nuqtaga bor va har birida sukut saqla.", "Otur evlat. Kalbini dinle. Üç yere git ve her birinde sus.", "Sit, child. Listen to your heart. Go to three places and keep silence at each."),
  ("Iymoning oshdi. Bor endi.", "İmanın arttı. Git şimdi.", "Your faith has grown. Go now."),
  [{'type': 'travel', 'n': 3}], (80, 0, 12)),
 ('sq_artuk_apothecary', 'artuk_bey', ("Artuq Beyning dorixonasi", "Artuk Bey'in eczanesi", "Artuk Bey's apothecary"),
  ("Alp, dori uchun o'simlik kerak: to'rt joydan yig'ib kel.", "Alp, ilaç için bitki lazım: dört yerden toplayıp gel.", "Alp, I need herbs for medicine: gather them from four places."),
  ("Mana, dorilar tayyor. Ol.", "İşte ilaçlar hazır. Al.", "The medicines are ready. Take them."),
  [{'type': 'collect', 'n': 4}], (90, 0, 1)),
 ('sq_dundar_first_sword', 'dundar_np', ("Dundorning birinchi qilichi", "Dündar'ın ilk kılıcı", "Dundar's first sword"),
  ("Aka, menga qilich o'rgat! Bir marta ur, men to'saman.", "Abi, bana kılıç öğret! Bir kez vur, ben savunayım.", "Brother, teach me the sword! Strike once, I will block."),
  ("Endi men ham alp bo'laman!", "Artık ben de alp olacağım!", "Now I too shall be an alp!"),
  [{'type': 'duel'}], (70, 0, 3)),
 ('sq_hamza_conscience', 'hamza', ("Hamzaning vijdoni", "Hamza'nın vicdanı", "Hamza's conscience"),
  ("Bey... men xoin edim. Menga ikkinchi imkon ber - mo'g'ul lagerini ko'rsataman.", "Bey... ben haindim. Bana ikinci bir şans ver - Moğol kampını gösteririm.", "Bey... I was a traitor. Give me a second chance - I will show you the Mongol camp."),
  ("Rahmat, bey. Endi qonim bilan to'layman.", "Sağ ol bey. Artık kanımla öderim.", "Thank you, bey. Now I pay with my blood."),
  [{'type': 'travel', 'n': 2}, {'type': 'stealth', 'guards': 3}, {'type': 'fight', 'waves': 1}], (140, 30, 5)),
]
NEW_NPCS = {
 'aykiz': ('chr.aykiz.name', 'oba', 20, -30, 200, True, [0.55, 0.20, 0.30]),
 'ibn_arabi_np': ('chr.ibn_arabi.name', 'caravan', 8, 26, 90, False, [0.85, 0.82, 0.75]),
 'afsin_bey': ('chr.afsin_bey.name', 'oba', -22, 12, 60, False, [0.25, 0.30, 0.15]),
 'selcan': ('chr.selcan.name', 'oba', 30, 30, 240, True, [0.36, 0.20, 0.30]),
 'banu_cicek': ('chr.banu_cicek.name', 'city', -60, 20, 0, True, [0.20, 0.45, 0.35]),
 'geyikli_baba': ('chr.geyikli_baba.name', 'forest', 0, 0, 180, False, [0.55, 0.50, 0.40]),
 'artuk_bey': ('chr.artuk_bey.name', 'city', 40, 40, 200, False, [0.30, 0.35, 0.55]),
 'dundar_np': ('chr.dundar.name', 'oba', 40, -40, 120, False, [0.18, 0.28, 0.20]),
 'hamza': ('chr.hamza.name', 'oba', -50, -60, 30, False, [0.30, 0.22, 0.15]),
}
loc('chr.afsin_bey.name', 'Afshin Bey', 'Afşin Bey', 'Afsin Bey')
loc('chr.banu_cicek.name', 'Banu Chichek', 'Banu Çiçek', 'Banu Cicek')
loc('chr.geyikli_baba.name', 'Geyikli Bobo', 'Geyikli Baba', 'Geyikli Baba')
loc('chr.artuk_bey.name', 'Artuq Bey', 'Artuk Bey', 'Artuk Bey')
loc('ui.hud.sidequest', 'Yon kvest', 'Yan görev', 'Side quest')
loc('ui.hud.quests', 'Yon kvestlar', 'Yan görevler', 'Side quests')
loc('DLG_SQ_ACCEPT', "Qabul qilaman.", "Kabul ediyorum.", "I accept.")
loc('DLG_SQ_LATER', "Keyinroq.", "Sonra.", "Later.")
loc('DLG_SQ_GO', "Yaxshi. Yo'lni xaritada belgiladim.", "İyi. Yolu haritada işaretledim.", "Good. I marked the way on the map.")
loc('DLG_SQ_DECLINE', "Bo'pti, kutaman.", "Peki, beklerim.", "Fine, I will wait.")

# NPC records
npcs = json.load(io.open(D + 'npcs.json', encoding='utf-8'))
ids = {n['id'] for n in npcs['npcs']}
for nid, (name, place, u, v, yaw, woman, kaftan) in NEW_NPCS.items():
    if nid not in ids:
        npcs['npcs'].append({'id': nid, 'name': name, 'place': place, 'u': u, 'v': v, 'yaw': yaw, 'woman': woman, 'kaftan': kaftan, 'dialog': 'npc_' + nid})
        ids.add(nid)
# quest option per giver: modify/create giver dialogs
SQ = {'quests': []}
for qid, giver, title, line, done, phases, (xp, gold, honor) in Q:
    loc('sq.%s.title' % qid, *title)
    loc('DLG_%s_ASK' % qid.upper(), *line)
    loc('DLG_%s_DONE' % qid.upper(), *done)
    loc('DLG_%s_OPT' % qid.upper(), "Kvest: " + title[0], "Görev: " + title[1], "Quest: " + title[2])
    SQ['quests'].append({'id': qid, 'giver': giver, 'title': 'sq.%s.title' % qid, 'phases': phases, 'xp': xp, 'gold': gold, 'honor': honor, 'done_key': 'DLG_%s_DONE' % qid.upper()})
    # giver dialog
    rec = next((n for n in npcs['npcs'] if n['id'] == giver), None)
    dlg_id = rec['dialog'] if rec else 'npc_' + giver
    path = D + 'dialogue/' + dlg_id + '.json'
    try: g = json.load(io.open(path, encoding='utf-8'))
    except Exception: g = None
    if g is None:
        spk = title[0]
        g = {'id': dlg_id, 'start': 'a', 'nodes': {'a': {'speaker': 'NPC', 'text_key': 'DLG_%s_ASK' % qid.upper(), 'options': []}}}
    start = g['nodes'][g['start']]
    start.setdefault('options', [])
    if not any(o.get('set_flag') == 'sq_start_' + qid for o in start['options']):
        start['options'].insert(0, {'text_key': 'DLG_%s_OPT' % qid.upper(), 'next': 'sq_' + qid, 'requires_evidence': 'sq_avail_' + qid})
        g['nodes']['sq_' + qid] = {'speaker': start.get('speaker', 'NPC'), 'text_key': 'DLG_%s_ASK' % qid.upper(), 'options': [
            {'text_key': 'DLG_SQ_ACCEPT', 'next': 'sq_go_' + qid, 'set_flag': 'sq_start_' + qid},
            {'text_key': 'DLG_SQ_LATER', 'next': 'sq_no_' + qid}]}
        g['nodes']['sq_go_' + qid] = {'speaker': start.get('speaker', 'NPC'), 'text_key': 'DLG_SQ_GO', 'next': 'end'}
        g['nodes']['sq_no_' + qid] = {'speaker': start.get('speaker', 'NPC'), 'text_key': 'DLG_SQ_DECLINE', 'next': 'end'}
    # if start node had no text (new giver), make it a plain greeting: use ASK line
    if start.get('speaker') == 'NPC':
        start['speaker'] = title[0]
        for n in g['nodes'].values():
            if n.get('speaker') == 'NPC': n['speaker'] = NEW_NPCS.get(giver, ('', '', 0, 0, 0, False, []))[0]
    io.open(path, 'w', encoding='utf-8').write(json.dumps(g, ensure_ascii=False, indent=1))
io.open(D + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
io.open(D + 'sidequests.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(SQ, ensure_ascii=False, indent=1))
s = io.open(D + 'npc_loc.csv', encoding='utf-8').read()
for row in L:
    if row.split(',')[0] not in s: s += row + '\n'
io.open(D + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(s)
print('quests', len(Q), 'npcs', len(npcs['npcs']), 'loc', len(L))
