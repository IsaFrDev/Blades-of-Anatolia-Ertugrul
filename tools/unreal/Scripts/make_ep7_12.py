# -*- coding: utf-8 -*-
import json, io
D = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
L = []
def loc(k, uz, tr, en): L.append('"%s","%s","%s","%s"' % (k, uz.replace('"', "'"), tr.replace('"', "'"), en.replace('"', "'")))
def node(sp, key, nxt=None, opts=None, flag=None):
    n = {'speaker': sp, 'text_key': key}
    if opts: n['options'] = opts
    else: n['next'] = nxt or 'end'
    if flag: n['set_flag'] = flag
    return n
g = {}
loc('chr.al_aziz_np.name', 'Al-Aziz Muhammad', 'El-Aziz Muhammed', 'Al-Aziz Muhammad')
loc('chr.ibn_arabi_np.name', 'Ibn Arabiy', 'İbn Arabi', 'Ibn Arabi')
loc('chr.kopek_np.name', "Sa'd al-Din Ko'pek", 'Sadeddin Köpek', 'Sadeddin Kopek')
loc('chr.noyan_np.name', "No'yon", 'Noyan', 'Noyan')
# EP007 Halab: bola amir
loc('DLG_EP007_AZIZ_01', "Turk beyi, Halab qal'asida xoinlar bor. Sen kimga ishonasan?", "Türk beyi, Halep kalesinde hainler var. Sen kime güvenirsin?", "Turkish bey, there are traitors in the Aleppo citadel. Whom do you trust?")
loc('DLG_EP007_OPT_PROOF', "Mana vazirning xati - u Templarlar bilan yozishmoqda.", "İşte vezirin mektubu - Tapınakçılarla yazışıyor.", "Here is the vizier's letter - he writes to the Templars.")
loc('DLG_EP007_OPT_WITNESS', "Qorovul guvoh: tunda qal'aga notanish kirdi.", "Muhafız tanık: gece kaleye yabancı girdi.", "A guard witnessed a stranger enter the citadel at night.")
loc('DLG_EP007_OPT_NONE', "Amir, hali ishonchli dalilim yo'q.", "Emir, henüz güvenilir delilim yok.", "Emir, I have no sure proof yet.")
loc('DLG_EP007_AZIZ_02', "Demak vazir... Seni qo'llayman, Ertug'rul.", "Demek vezir... Seni destekliyorum Ertuğrul.", "So the vizier... I stand with you, Ertugrul.")
loc('DLG_EP007_AZIZ_03', "Dalilsiz gap saroyda o'limdir. Ehtiyot bo'l.", "Delilsiz söz sarayda ölümdür. Dikkat et.", "Words without proof are death at court. Be careful.")
g['ep007_aziz'] = {'id': 'ep007_aziz', 'start': 'a', 'duel_threshold': 2, 'nodes': {
    'a': node('Al-Aziz Muhammad', 'DLG_EP007_AZIZ_01', opts=[
        {'text_key': 'DLG_EP007_OPT_PROOF', 'next': 'a', 'requires_evidence': 'vizier_letter', 'duel_points': 1, 'honor': 2},
        {'text_key': 'DLG_EP007_OPT_WITNESS', 'next': 'b', 'requires_evidence': 'guard_witness', 'duel_points': 1, 'honor': 2},
        {'text_key': 'DLG_EP007_OPT_NONE', 'next': 'c'}]),
    'b': node('Al-Aziz Muhammad', 'DLG_EP007_AZIZ_02', flag='aziz_ally'),
    'c': node('Al-Aziz Muhammad', 'DLG_EP007_AZIZ_03')}}
# EP008 Ibn Arabiy
loc('DLG_EP008_ARABI_01', "Yo'lda ko'p qon ko'rding, o'g'lim. Qilich bilan nima izlaysan?", "Yolda çok kan gördün evladım. Kılıçla ne ararsın?", "You have seen much blood on the road, my son. What do you seek with the sword?")
loc('DLG_EP008_OPT_JUSTICE', "Adolat. Zolimdan mazlumni himoya qilish.", "Adalet. Zalimden mazlumu korumak.", "Justice. To shield the oppressed from the tyrant.")
loc('DLG_EP008_OPT_GLORY', "Shon-shuhrat va yurt.", "Şan ve yurt.", "Glory and a homeland.")
loc('DLG_EP008_ARABI_02', "Unda qilichingni qalbing bilan ushla. Yo'l ochiq.", "O halde kılıcını kalbinle tut. Yol açık.", "Then hold your sword with your heart. The road is open.")
loc('DLG_EP008_ARABI_03', "Shuhrat - soya. Soya ortidan yurgan yiqiladi. O'ylab ko'r.", "Şan gölgedir. Gölgenin peşinden giden düşer. Düşün.", "Glory is a shadow. He who chases shadows falls. Think on it.")
g['ep008_arabi'] = {'id': 'ep008_arabi', 'start': 'a', 'nodes': {
    'a': node('Ibn Arabiy', 'DLG_EP008_ARABI_01', opts=[
        {'text_key': 'DLG_EP008_OPT_JUSTICE', 'next': 'b', 'honor': 6, 'set_flag': 'arabi_blessing'},
        {'text_key': 'DLG_EP008_OPT_GLORY', 'next': 'c', 'honor': -2}]),
    'b': node('Ibn Arabiy', 'DLG_EP008_ARABI_02'),
    'c': node('Ibn Arabiy', 'DLG_EP008_ARABI_03')}}
# EP009 Kurdoglu iplari - Gundogdu
loc('DLG_EP009_GUND_01', "Uka, Kurdo'g'lining odamlari hali obada. Kimligini bilasanmi?", "Kardeşim, Kurdoğlu'nun adamları hâlâ obada. Kim olduklarını biliyor musun?", "Brother, Kurdoglu's men are still in the tribe. Do you know who they are?")
loc('DLG_EP009_OPT_A', "Chorvador Hamza - qo'yni tunda sotgan.", "Çoban Hamza - koyunu gece satmış.", "Hamza the herder - he sold sheep at night.")
loc('DLG_EP009_OPT_B', "Ovchi Sung'ur - o'qlari dushmannikiga o'xshaydi.", "Avcı Sungur - okları düşmanınkine benziyor.", "Sungur the hunter - his arrows match the enemy's.")
loc('DLG_EP009_OPT_C', "Hali aniq emas.", "Henüz belli değil.", "Not yet certain.")
loc('DLG_EP009_GUND_02', "Ikkalasini ham kengashga olib boramiz.", "İkisini de kurultaya götürürüz.", "We bring them both before the council.")
loc('DLG_EP009_GUND_03', "Shoshma. Noto'g'ri ayblov obani bo'ladi.", "Acele etme. Yanlış suçlama obayı böler.", "Do not rush. A false charge splits the tribe.")
g['ep009_gundogdu'] = {'id': 'ep009_gundogdu', 'start': 'a', 'duel_threshold': 2, 'nodes': {
    'a': node("Gundo'g'di", 'DLG_EP009_GUND_01', opts=[
        {'text_key': 'DLG_EP009_OPT_A', 'next': 'a', 'requires_evidence': 'sheep_sale', 'duel_points': 1},
        {'text_key': 'DLG_EP009_OPT_B', 'next': 'b', 'requires_evidence': 'enemy_arrows', 'duel_points': 1},
        {'text_key': 'DLG_EP009_OPT_C', 'next': 'c'}]),
    'b': node("Gundo'g'di", 'DLG_EP009_GUND_02', flag='spies_named'),
    'c': node("Gundo'g'di", 'DLG_EP009_GUND_03')}}
# EP010 O'rmondagi qon - Turgut yarador
loc('DLG_EP010_TURGUT_01', "Bey... ular shimolga qochdi. Meni qoldir, quv!", "Bey... kuzeye kaçtılar. Beni bırak, kovala!", "Bey... they fled north. Leave me, give chase!")
loc('DLG_EP010_OPT_CHASE', "Chidab tur, qaytaman.", "Dayan, dönerim.", "Hold on, I will return.")
loc('DLG_EP010_OPT_STAY', "Avval yarangni bog'layman.", "Önce yaranı sararım.", "First I bind your wound.")
loc('DLG_EP010_TURGUT_02', "Bor! Vaqt yo'q!", "Git! Vakit yok!", "Go! There is no time!")
loc('DLG_EP010_TURGUT_03', "...Rahmat, bey. Endi bor.", "...Sağ ol bey. Şimdi git.", "...Thank you, bey. Now go.")
g['ep010_turgut'] = {'id': 'ep010_turgut', 'start': 'a', 'nodes': {
    'a': node('Turgut Alp', 'DLG_EP010_TURGUT_01', opts=[
        {'text_key': 'DLG_EP010_OPT_CHASE', 'next': 'b', 'honor': -1},
        {'text_key': 'DLG_EP010_OPT_STAY', 'next': 'c', 'honor': 4, 'set_flag': 'turgut_saved'}]),
    'b': node('Turgut Alp', 'DLG_EP010_TURGUT_02'),
    'c': node('Turgut Alp', 'DLG_EP010_TURGUT_03')}}
# EP011 Suv yo'li - Kopek (yashirin uchrashuv)
loc('DLG_EP011_KOPEK_01', "Ertug'rul... Sulton seni chaqirmadi. Kim yubordi?", "Ertuğrul... Sultan seni çağırmadı. Kim gönderdi?", "Ertugrul... The Sultan did not summon you. Who sent you?")
loc('DLG_EP011_OPT_LIE', "Karvon xabari bilan keldim.", "Kervan haberiyle geldim.", "I came with news of the caravan.")
loc('DLG_EP011_OPT_TRUTH', "Sening xatlaring mo'g'ullarga boryapti, Ko'pek.", "Mektupların Moğollara gidiyor Köpek.", "Your letters go to the Mongols, Kopek.")
loc('DLG_EP011_KOPEK_02', "Xabar? Yaxshi. Xizmatkorlar sizni kuzatib qo'yadi.", "Haber mi? İyi. Hizmetkârlar sizi uğurlar.", "News? Good. The servants will see you out.")
loc('DLG_EP011_KOPEK_03', "Jasur... va ahmoq. Qorovullar!", "Cesur... ve aptal. Muhafızlar!", "Brave... and foolish. Guards!")
g['ep011_kopek'] = {'id': 'ep011_kopek', 'start': 'a', 'nodes': {
    'a': node("Sa'd al-Din Ko'pek", 'DLG_EP011_KOPEK_01', opts=[
        {'text_key': 'DLG_EP011_OPT_LIE', 'next': 'b', 'honor': -2, 'set_flag': 'kopek_fooled'},
        {'text_key': 'DLG_EP011_OPT_TRUTH', 'next': 'c', 'honor': 3, 'set_flag': 'kopek_alarm'}]),
    'b': node("Sa'd al-Din Ko'pek", 'DLG_EP011_KOPEK_02'),
    'c': node("Sa'd al-Din Ko'pek", 'DLG_EP011_KOPEK_03')}}
# EP012 Bagras - qamal, No'yon elchisi
loc('DLG_EP012_NOYAN_01', "Qal'ani top, Ertug'rul. Ertaga tong otmaydi - No'yonning so'zi.", "Kaleyi teslim et Ertuğrul. Yarın şafak sökmez - Noyan'ın sözü.", "Surrender the castle, Ertugrul. No dawn tomorrow - Noyan's word.")
loc('DLG_EP012_OPT_REFUSE', "Qayilar tiz cho'kmaydi. Elchini qaytaring.", "Kayılar diz çökmez. Elçiyi geri gönderin.", "The Kayi do not kneel. Send the envoy back.")
loc('DLG_EP012_OPT_TIME', "Kengash bilan maslahatlashaman. Tonggacha vaqt ber.", "Kurultayla danışırım. Şafağa dek süre ver.", "I will consult the council. Give me until dawn.")
loc('DLG_EP012_NOYAN_02', "Unda devorlaring qabring bo'ladi.", "O halde duvarların mezarın olur.", "Then your walls shall be your grave.")
loc('DLG_EP012_NOYAN_03', "Tonggacha. Keyin - o't.", "Şafağa dek. Sonra - ateş.", "Until dawn. Then - fire.")
g['ep012_noyan'] = {'id': 'ep012_noyan', 'start': 'a', 'nodes': {
    'a': node("No'yon", 'DLG_EP012_NOYAN_01', opts=[
        {'text_key': 'DLG_EP012_OPT_REFUSE', 'next': 'b', 'honor': 5, 'set_flag': 'siege_defied'},
        {'text_key': 'DLG_EP012_OPT_TIME', 'next': 'c', 'honor': 0, 'set_flag': 'siege_delay'}]),
    'b': node("No'yon", 'DLG_EP012_NOYAN_02'),
    'c': node("No'yon", 'DLG_EP012_NOYAN_03')}}
for k, v in g.items():
    io.open(D + 'dialogue/' + k + '.json', 'w', encoding='utf-8').write(json.dumps(v, ensure_ascii=False, indent=1))
s = io.open(D + 'npc_loc.csv', encoding='utf-8').read()
for row in L:
    if row.split(',')[0] not in s: s += row + '\n'
io.open(D + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(s)
M = json.load(io.open(D + 'missions.json', encoding='utf-8'))
city = lambda i, n, u, v, yaw, k, w=False: {'id': i, 'name': n, 'u': u, 'v': v, 'yaw': yaw, 'kaftan': k, 'woman': w}
M['EP007'] = {'phases': [
    {'type': 'travel', 'n': 2},
    {'type': 'collect', 'n': 2, 'flags': ['vizier_letter', 'guard_witness'], 'loc': 'ui.obj.collect_evidence'},
    {'type': 'council', 'dialog': 'ep007_aziz', 'threshold': 2, 'npcs': [city('al_aziz', 'chr.al_aziz_np.name', -6, -7, 180, [0.55, 0.45, 0.12])]},
    {'type': 'stealth', 'guards': 3}, {'type': 'duel'}]}
M['EP008'] = {'phases': [
    {'type': 'travel', 'n': 2},
    {'type': 'defend', 'waves': 2},
    {'type': 'council', 'dialog': 'ep008_arabi', 'threshold': 0, 'npcs': [city('ibn_arabi', 'chr.ibn_arabi_np.name', -6, -7, 180, [0.85, 0.82, 0.75])]},
    {'type': 'travel', 'n': 2}, {'type': 'fight', 'waves': 1}]}
M['EP009'] = {'phases': [
    {'type': 'travel', 'n': 2},
    {'type': 'collect', 'n': 2, 'flags': ['sheep_sale', 'enemy_arrows'], 'loc': 'ui.obj.collect_evidence'},
    {'type': 'council', 'dialog': 'ep009_gundogdu', 'threshold': 2, 'npcs': [city('gundogdu_cut', 'chr.gundogdu.name', -56, 40, 0, [0.14, 0.18, 0.36])]},
    {'type': 'stealth', 'guards': 2}, {'type': 'fight', 'waves': 1}]}
M['EP010'] = {'phases': [
    {'type': 'travel', 'n': 3, 'timed': True},
    {'type': 'fight', 'waves': 1},
    {'type': 'council', 'dialog': 'ep010_turgut', 'threshold': 0, 'npcs': [city('turgut_wounded', 'chr.turgut.name', 34, -24, 90, [0.20, 0.22, 0.12])]},
    {'type': 'travel', 'n': 2, 'timed': True}, {'type': 'duel'}]}
M['EP011'] = {'phases': [
    {'type': 'travel', 'n': 2},
    {'type': 'stealth', 'guards': 3},
    {'type': 'council', 'dialog': 'ep011_kopek', 'threshold': 0, 'npcs': [city('kopek', 'chr.kopek_np.name', -6, -7, 180, [0.10, 0.10, 0.12])]},
    {'type': 'collect', 'n': 2, 'flags': ['kopek_letter', 'mongol_seal'], 'loc': 'ui.obj.collect_evidence'},
    {'type': 'travel', 'n': 2, 'timed': True}]}
M['EP012'] = {'phases': [
    {'type': 'council', 'dialog': 'ep012_noyan', 'threshold': 0, 'npcs': [city('noyan_envoy', 'chr.noyan_np.name', -8, -30, 0, [0.30, 0.10, 0.08])]},
    {'type': 'travel', 'n': 2, 'timed': True},
    {'type': 'defend', 'waves': 3},
    {'type': 'fight', 'waves': 2}]}
io.open(D + 'missions.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(M, ensure_ascii=False, indent=1))
print('dialogs', len(g), 'loc', len(L), 'missions', len([k for k in M if k.startswith('EP')]))
