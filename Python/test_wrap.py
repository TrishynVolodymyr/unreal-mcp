import json
import sys
sys.path.insert(0, '.')
from utils.unreal_connection_utils import send_unreal_command

BP = 'WBP_UI_Slider2'

# Get real node IDs from metadata
eg_nodes = {}
r = send_unreal_command('get_blueprint_metadata', {
    'blueprint_name': BP, 'fields': ['graph_nodes'], 'graph_name': 'EventGraph'
})
for n in r.get('result',{}).get('metadata',{}).get('graph_nodes',{}).get('graphs',[{}])[0].get('nodes',[]):
    eg_nodes[n['title']] = n['id']

uvd_nodes = {}
r = send_unreal_command('get_blueprint_metadata', {
    'blueprint_name': BP, 'fields': ['graph_nodes'], 'graph_name': 'UpdateValueDisplay'
})
for n in r.get('result',{}).get('metadata',{}).get('graph_nodes',{}).get('graphs',[{}])[0].get('nodes',[]):
    uvd_nodes[n['title']] = n['id']

# Handle duplicate titles (UpdateValueDisplay appears twice in EG)
eg_uvd_calls = [n for n in r.get('result',{}).get('metadata',{}).get('graph_nodes',{}).get('graphs',[{}])[0].get('nodes',[]) if n['title'] == 'UpdateValueDisplay']

# Re-get EG for UVD calls
r = send_unreal_command('get_blueprint_metadata', {
    'blueprint_name': BP, 'fields': ['graph_nodes'], 'graph_name': 'EventGraph'
})
eg_all = r.get('result',{}).get('metadata',{}).get('graph_nodes',{}).get('graphs',[{}])[0].get('nodes',[])
uvd_call_ids = [n['id'] for n in eg_all if n['title'] == 'UpdateValueDisplay']

print("EG nodes:", {k: v[:12] for k,v in eg_nodes.items()})
print("UVD nodes:", {k: v[:12] for k,v in uvd_nodes.items()})
print("UVD call IDs:", [x[:12] for x in uvd_call_ids])

# === EventGraph connections ===
PRECONSTRUCT = eg_nodes['Event Pre Construct']
ON_VALUE_CHANGED = eg_nodes['On Value Changed (Sld_Interactive)']
GET_LABEL = eg_nodes['Get SliderLabel']
GET_TXT_NAME = eg_nodes['Get Txt_Name']
SETTEXT_NAME = eg_nodes['SetText (Text)']
UVD_CALL1 = uvd_call_ids[0]
UVD_CALL2 = uvd_call_ids[1]

eg_conns = [
    {'source_node_id': PRECONSTRUCT, 'source_pin': 'then', 'target_node_id': SETTEXT_NAME, 'target_pin': 'execute'},
    {'source_node_id': SETTEXT_NAME, 'source_pin': 'then', 'target_node_id': UVD_CALL1, 'target_pin': 'execute'},
    {'source_node_id': GET_TXT_NAME, 'source_pin': 'Txt_Name', 'target_node_id': SETTEXT_NAME, 'target_pin': 'self'},
    {'source_node_id': GET_LABEL, 'source_pin': 'SliderLabel', 'target_node_id': SETTEXT_NAME, 'target_pin': 'InText'},
    {'source_node_id': ON_VALUE_CHANGED, 'source_pin': 'then', 'target_node_id': UVD_CALL2, 'target_pin': 'execute'},
    {'source_node_id': ON_VALUE_CHANGED, 'source_pin': 'Value', 'target_node_id': UVD_CALL2, 'target_pin': 'SliderValue'},
]

r1 = send_unreal_command('connect_blueprint_nodes', {
    'blueprint_name': BP, 'target_graph': 'EventGraph', 'connections': eg_conns
})
eg_results = r1.get('results', r1.get('result',{}).get('results',[]))
eg_ok = sum(1 for x in eg_results if x.get('success'))
print(f"\nEventGraph: {eg_ok}/{len(eg_conns)}")
for x in eg_results:
    if not x.get('success'):
        print(f"  FAIL: {x.get('error','')}")

# === UpdateValueDisplay connections ===
UVD_ENTRY = uvd_nodes['UpdateValueDisplay']
GET_MAX = uvd_nodes['Get MaxDisplayValue']
GET_SUFFIX = uvd_nodes['Get ValueSuffix']
MULTIPLY = uvd_nodes.get('IntPoint * IntPoint', uvd_nodes.get('float * float', ''))
ROUND = uvd_nodes['Round']
TOSTR = uvd_nodes['To String (Integer)']
APPEND = uvd_nodes['Append']
TOTEXT = uvd_nodes['To Text (String)']
GET_TXT_PCT = uvd_nodes['Get Txt_Percent']
SETTEXT_PCT = uvd_nodes['SetText (Text)']

uvd_conns = [
    {'source_node_id': UVD_ENTRY, 'source_pin': 'then', 'target_node_id': SETTEXT_PCT, 'target_pin': 'execute'},
    {'source_node_id': UVD_ENTRY, 'source_pin': 'SliderValue', 'target_node_id': MULTIPLY, 'target_pin': 'A'},
    {'source_node_id': GET_MAX, 'source_pin': 'MaxDisplayValue', 'target_node_id': MULTIPLY, 'target_pin': 'B'},
    {'source_node_id': MULTIPLY, 'source_pin': 'ReturnValue', 'target_node_id': ROUND, 'target_pin': 'A'},
    {'source_node_id': ROUND, 'source_pin': 'ReturnValue', 'target_node_id': TOSTR, 'target_pin': 'InInt'},
    {'source_node_id': TOSTR, 'source_pin': 'ReturnValue', 'target_node_id': APPEND, 'target_pin': 'A'},
    {'source_node_id': GET_SUFFIX, 'source_pin': 'ValueSuffix', 'target_node_id': APPEND, 'target_pin': 'B'},
    {'source_node_id': APPEND, 'source_pin': 'ReturnValue', 'target_node_id': TOTEXT, 'target_pin': 'InString'},
    {'source_node_id': TOTEXT, 'source_pin': 'ReturnValue', 'target_node_id': SETTEXT_PCT, 'target_pin': 'InText'},
    {'source_node_id': GET_TXT_PCT, 'source_pin': 'Txt_Percent', 'target_node_id': SETTEXT_PCT, 'target_pin': 'self'},
]

r2 = send_unreal_command('connect_blueprint_nodes', {
    'blueprint_name': BP, 'target_graph': 'UpdateValueDisplay', 'connections': uvd_conns
})
uvd_results = r2.get('results', r2.get('result',{}).get('results',[]))
uvd_ok = sum(1 for x in uvd_results if x.get('success'))
print(f"UpdateValueDisplay: {uvd_ok}/{len(uvd_conns)}")
for x in uvd_results:
    if not x.get('success'):
        print(f"  FAIL: {x.get('error','')}")

# Compile
r3 = send_unreal_command('compile_blueprint', {'blueprint_name': BP})
print(f"\nCompile: {r3.get('status')} {r3.get('result',{}).get('message','')}")
