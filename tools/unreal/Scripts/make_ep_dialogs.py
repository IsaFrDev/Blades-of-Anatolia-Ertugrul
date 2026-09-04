# -*- coding: utf-8 -*-
import json, io
D = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
L = []  # loc rows
def loc(k, uz, tr, en): L.append('"%s","%s","%s","%s"' % (k, uz.replace('"', "'"), tr.replace('"', "'"), en.replace('"', "'")))
def node(sp, key, nxt=None, opts=None, flag=None):
    n = {'speaker': sp, 'text_key': key}
    if opts: n['options'] = opts
    else: n['next'] = nxt or 'end'
    if flag: n['set_flag'] = flag
    return n
g = {}
# ---- EP003: o'layotgan Templar serjanti ----
loc('chr.templar_sgt.name', 'Templar serjanti', 'Tapınakçı çavuşu', 'Templar sergeant')
loc('DLG_EP003_SGT_01', "...Ille scit. Senex scit... U biladi. Chol biladi.", "...Ille scit. Senex scit... O biliyor. İhtiyar biliyor.", "...Ille scit. Senex scit... He knows. The old man knows.")
loc('DLG_EP003_ERT_ASK', "Kim? Qaysi chol? Gapir!", "Kim? Hangi ihtiyar? Konuş!", "Who? Which old man? Speak!")
loc('DLG_EP003_ERT_MERCY', "Suv ber unga. Dushman bo'lsa ham odam.", "Ona su verin. Düşman da olsa insan.", "Give him water. Enemy or not, he is a man.")
loc('DLG_EP003_SGT_02', "Qal'ada... Titus... u hammani biladi...", "Kalede... Titus... o herkesi tanır...", "In the castle... Titus... he knows everyone...")
loc('DLG_EP003_SGT_03', "Rahmat... turk. Titus. Qal'a. Bu nomni unutma.", "Sağ ol... Türk. Titus. Kale. Bu adı unutma.", "Thank you... Turk. Titus. The castle. Do not forget that name.")
g['ep003_sergeant'] = {'id': 'ep003_sergeant', 'start': 'a', 'nodes': {
    'a': node('Templar serjanti', 'DLG_EP003_SGT_01', opts=[
        {'text_key': 'DLG_EP003_ERT_ASK', 'next': 'b', 'honor': -2},
        {'text_key': 'DLG_EP003_ERT_MERCY', 'next': 'c', 'honor': 5, 'set_flag': 'sergeant_mercy'}]),
    'b': node('Templar serjanti', 'DLG_EP003_SGT_02', flag='titus_named'),
    'c': node('Templar serjanti', 'DLG_EP003_SGT_03', flag='titus_named')}}
# ---- EP004: otaning ismi - Sulaymon Shoh bilan ----
loc('DLG_EP004_SHAH_01', "O'g'lim, obada gap-so'z ko'p: mening ismim bilan soxta muhr yurganmish. Nima topding?", "Oğlum, obada söylenti çok: adımla sahte mühür dolaşıyormuş. Ne buldun?", "My son, the tribe is full of talk: a false seal bears my name. What did you find?")
loc('DLG_EP004_OPT_LETTER', "Mana xat - qo'lyozma Kurdo'g'liniki.", "İşte mektup - el yazısı Kurdoğlu'nun.", "Here is the letter - the hand is Kurdoglu's.")
loc('DLG_EP004_OPT_SEAL', "Mana muhr - sizniki emas, nusxa.", "İşte mühür - sizin değil, kopya.", "Here is the seal - not yours, a copy.")
loc('DLG_EP004_OPT_NOTHING', "Hali dalil yo'q, ota.", "Henüz delil yok baba.", "No proof yet, father.")
loc('DLG_EP004_SHAH_02', "Yaxshi. Bu dalil bilan kengashda gapiraman.", "İyi. Bu delille kurultayda konuşurum.", "Good. With this proof I will speak at the council.")
loc('DLG_EP004_SHAH_03', "Dalilsiz so'z - shamol. Yana izla.", "Delilsiz söz rüzgârdır. Yine ara.", "Words without proof are wind. Search again.")
g['ep004_father'] = {'id': 'ep004_father', 'start': 'a', 'duel_threshold': 1, 'nodes': {
    'a': node('Sulaymon Shoh', 'DLG_EP004_SHAH_01', opts=[
        {'text_key': 'DLG_EP004_OPT_LETTER', 'next': 'b', 'requires_evidence': 'father_letter', 'duel_points': 1, 'honor': 3},
        {'text_key': 'DLG_EP004_OPT_SEAL', 'next': 'b', 'requires_evidence': 'tribe_seal', 'duel_points': 1, 'honor': 3},
        {'text_key': 'DLG_EP004_OPT_NOTHING', 'next': 'c'}]),
    'b': node('Sulaymon Shoh', 'DLG_EP004_SHAH_02', flag='father_convinced'),
    'c': node('Sulaymon Shoh', 'DLG_EP004_SHAH_03')}}
# ---- EP005: qorli dovon - Bamsi ----
loc('DLG_EP005_BAMSI_01', "Bey, bo'ron kuchaymoqda. Ovdan go'sht bor, lekin dovondan o'tish kerak - kechga qolsak muzlaymiz.", "Bey, fırtına artıyor. Avdan et var ama geçitten geçmeliyiz - geceye kalırsak donarız.", "Bey, the storm is growing. We have meat from the hunt, but we must cross the pass before night or we freeze.")
loc('DLG_EP005_OPT_GO', "Yuramiz. Hech kim ortda qolmaydi.", "Yürüyoruz. Kimse geride kalmaz.", "We move. No one is left behind.")
loc('DLG_EP005_OPT_REST', "Avval olov yoqamiz, yigitlar isinsin.", "Önce ateş yakalım, yiğitler ısınsın.", "First a fire, let the men warm up.")
loc('DLG_EP005_BAMSI_02', "Bo'pti! Turgut, oldinga!", "Tamam! Turgut, öne!", "Good! Turgut, take the lead!")
loc('DLG_EP005_BAMSI_03', "Olov... ha, qorin ham, qo'l ham isiydi. Lekin tez bo'lsin.", "Ateş... evet, karın da el de ısınır. Ama çabuk olsun.", "A fire... yes, belly and hands both warm. But be quick.")
g['ep005_bamsi'] = {'id': 'ep005_bamsi', 'start': 'a', 'nodes': {
    'a': node('Bamsi Bayrak', 'DLG_EP005_BAMSI_01', opts=[
        {'text_key': 'DLG_EP005_OPT_GO', 'next': 'b', 'honor': 2},
        {'text_key': 'DLG_EP005_OPT_REST', 'next': 'c', 'honor': 1, 'set_flag': 'camp_fire'}]),
    'b': node('Bamsi Bayrak', 'DLG_EP005_BAMSI_02'),
    'c': node('Bamsi Bayrak', 'DLG_EP005_BAMSI_03')}}
# ---- EP006: Sultan Han - Turgut bilan tergov ----
loc('DLG_EP006_TURGUT_01', "Bey, karvonsaroyda o'lik topildi. Nima topding?", "Bey, kervansarayda ölü bulundu. Ne buldun?", "Bey, a dead man was found at the caravanserai. What did you find?")
loc('DLG_EP006_OPT_TRACKS', "Izlar - otliq, shimolga ketgan.", "İzler - atlı, kuzeye gitmiş.", "Tracks - a rider, gone north.")
loc('DLG_EP006_OPT_CLOTH', "Yirtilgan mato - mo'g'ul chopon.", "Yırtık kumaş - Moğol kaftanı.", "Torn cloth - a Mongol coat.")
loc('DLG_EP006_OPT_DAGGER', "Xanjar - Ko'pekning muhri bilan.", "Hançer - Köpek'in mührüyle.", "A dagger - with Kopek's mark.")
loc('DLG_EP006_OPT_NONE', "Hali kam. Yana qaraymiz.", "Henüz az. Yine bakarız.", "Not enough yet. We look again.")
loc('DLG_EP006_TURGUT_02', "Demak Ko'pek va mo'g'ullar... Bey, bu urush degani.", "Demek Köpek ve Moğollar... Bey, bu savaş demek.", "So Kopek and the Mongols... Bey, this means war.")
loc('DLG_EP006_TURGUT_03', "Xo'p. Har toshni ag'daramiz.", "Peki. Her taşı çeviririz.", "Fine. We turn every stone.")
g['ep006_turgut'] = {'id': 'ep006_turgut', 'start': 'a', 'duel_threshold': 2, 'nodes': {
    'a': node('Turgut Alp', 'DLG_EP006_TURGUT_01', opts=[
        {'text_key': 'DLG_EP006_OPT_TRACKS', 'next': 'a', 'requires_evidence': 'tracks', 'duel_points': 1},
        {'text_key': 'DLG_EP006_OPT_CLOTH', 'next': 'a', 'requires_evidence': 'torn_cloth', 'duel_points': 1},
        {'text_key': 'DLG_EP006_OPT_DAGGER', 'next': 'b', 'requires_evidence': 'dagger', 'duel_points': 2, 'set_flag': 'kopek_suspected'},
        {'text_key': 'DLG_EP006_OPT_NONE', 'next': 'c'}]),
    'b': node('Turgut Alp', 'DLG_EP006_TURGUT_02'),
    'c': node('Turgut Alp', 'DLG_EP006_TURGUT_03')}}
for k, v in g.items():
    io.open(D + 'dialogue/' + k + '.json', 'w', encoding='utf-8').write(json.dumps(v, ensure_ascii=False, indent=1))
s = io.open(D + 'npc_loc.csv', encoding='utf-8').read()
for row in L:
    if row.split(',')[0] not in s: s += row + '\n'
io.open(D + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(s)
# ---- missions.json ----
M = json.load(io.open(D + 'missions.json', encoding='utf-8'))
M['EP003'] = {'phases': [
    {'type': 'travel', 'n': 2, 'timed': True},
    {'type': 'defend', 'waves': 2},
    {'type': 'council', 'dialog': 'ep003_sergeant', 'threshold': 0, 'npcs': [
        {'id': 'templar_sgt', 'name': 'chr.templar_sgt.name', 'u': -8, 'v': -30, 'yaw': 0, 'kaftan': [0.75, 0.72, 0.68]}]},
    {'type': 'fight', 'waves': 1}]}
M['EP004'] = {'phases': [
    {'type': 'travel', 'n': 2},
    {'type': 'collect', 'n': 2, 'flags': ['father_letter', 'tribe_seal'], 'loc': 'ui.obj.collect_evidence'},
    {'type': 'council', 'dialog': 'ep004_father', 'threshold': 1, 'npcs': [
        {'id': 'suleyman_shah', 'name': 'chr.suleyman_shah.name', 'u': -6, 'v': -7, 'yaw': 180, 'kaftan': [0.32, 0.10, 0.10]}]},
    {'type': 'stealth', 'guards': 2}]}
M['EP005'] = {'phases': [
    {'type': 'hunt', 'n': 2},
    {'type': 'travel', 'n': 3},
    {'type': 'council', 'dialog': 'ep005_bamsi', 'threshold': 0, 'npcs': [
        {'id': 'bamsi_cut', 'name': 'chr.bamsi.name', 'u': -7, 'v': -112, 'yaw': 90, 'kaftan': [0.36, 0.26, 0.10]}]},
    {'type': 'fight', 'waves': 1}]}
M['EP006'] = {'phases': [
    {'type': 'travel', 'n': 2},
    {'type': 'collect', 'n': 3, 'flags': ['tracks', 'torn_cloth', 'dagger'], 'loc': 'ui.obj.collect_evidence'},
    {'type': 'council', 'dialog': 'ep006_turgut', 'threshold': 2, 'npcs': [
        {'id': 'turgut_cut', 'name': 'chr.turgut.name', 'u': 34, 'v': -24, 'yaw': 90, 'kaftan': [0.20, 0.22, 0.12]}]},
    {'type': 'stealth', 'guards': 2},
    {'type': 'fight', 'waves': 1}]}
io.open(D + 'missions.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(M, ensure_ascii=False, indent=1))
print('dialogs', len(g), 'loc rows', len(L), 'missions', len([k for k in M if k.startswith('EP')]))
