# -*- coding: utf-8 -*-
import json, io, os
D = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/dialogue/'
def lin(id_, speaker, keys, end_flag=None):
    nodes = {}
    for i, k in enumerate(keys):
        n = {'speaker': speaker, 'text_key': k, 'next': ('n%d' % (i + 1)) if i + 1 < len(keys) else 'end'}
        if end_flag and i == len(keys) - 1: n['set_flag'] = end_flag
        nodes['n%d' % i] = n
    return {'id': id_, 'start': 'n0', 'nodes': nodes}

g = {}
g['npc_hayme_ana'] = {'id': 'npc_hayme_ana', 'start': 'a', 'nodes': {
    'a': {'speaker': 'Hayma Ona', 'text_key': 'DLG_NPC_HAYME_01', 'next': 'b'},
    'b': {'speaker': 'Hayma Ona', 'text_key': 'DLG_NPC_HAYME_02', 'options': [
        {'text_key': 'DLG_NPC_HAYME_OPT_PROMISE', 'next': 'c', 'honor': 4, 'set_flag': 'hayme_promise'},
        {'text_key': 'DLG_NPC_HAYME_OPT_ANGRY', 'next': 'd', 'honor': -3}]},
    'c': {'speaker': 'Hayma Ona', 'text_key': 'DLG_NPC_HAYME_03', 'next': 'end'},
    'd': {'speaker': 'Hayma Ona', 'text_key': 'DLG_NPC_HAYME_04', 'next': 'end'}}}
g['npc_halime'] = lin('npc_halime', 'Halima Sulton', ['DLG_NPC_HALIME_01', 'DLG_NPC_HALIME_02'])
g['npc_turgut'] = {'id': 'npc_turgut', 'start': 'a', 'nodes': {
    'a': {'speaker': 'Turgut Alp', 'text_key': 'DLG_NPC_TURGUT_01', 'options': [
        {'text_key': 'DLG_NPC_TURGUT_OPT_TRAIN', 'next': 'b', 'set_flag': 'give_arrows'},
        {'text_key': 'DLG_NPC_TURGUT_OPT_LATER', 'next': 'c'}]},
    'b': {'speaker': 'Turgut Alp', 'text_key': 'DLG_NPC_TURGUT_02', 'next': 'end'},
    'c': {'speaker': 'Turgut Alp', 'text_key': 'DLG_NPC_TURGUT_03', 'next': 'end'}}}
g['npc_deli_demir'] = lin('npc_deli_demir', 'Deli Demir', ['DLG_NPC_DEMIR_01', 'DLG_NPC_DEMIR_02'], 'sword_sharpened')
g['npc_bamsi'] = lin('npc_bamsi', 'Bamsi Bayrak', ['DLG_NPC_BAMSI_01', 'DLG_NPC_BAMSI_02'])
g['npc_gundogdu'] = lin('npc_gundogdu', "Gundo'g'di", ['DLG_NPC_GUNDOGDU_01', 'DLG_NPC_GUNDOGDU_02'])
g['npc_merchant'] = {'id': 'npc_merchant', 'start': 'a', 'nodes': {
    'a': {'speaker': 'Savdogar Yusuf', 'text_key': 'DLG_NPC_MERCHANT_01', 'options': [
        {'text_key': 'DLG_NPC_MERCHANT_OPT_NEWS', 'next': 'b'},
        {'text_key': 'DLG_NPC_MERCHANT_OPT_BYE', 'next': 'c'}]},
    'b': {'speaker': 'Savdogar Yusuf', 'text_key': 'DLG_NPC_MERCHANT_02', 'next': 'end'},
    'c': {'speaker': 'Savdogar Yusuf', 'text_key': 'DLG_NPC_MERCHANT_03', 'next': 'end'}}}
g['npc_caravan'] = lin('npc_caravan', 'Karvonboshi Umar', ['DLG_NPC_CARAVAN_01', 'DLG_NPC_CARAVAN_02'])
for k, v in g.items():
    io.open(D + k + '.json', 'w', encoding='utf-8').write(json.dumps(v, ensure_ascii=False, indent=1))
print(len(g), 'dialog yozildi')
