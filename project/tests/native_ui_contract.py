#!/usr/bin/env python3
"""Check the shipped MPC skin bindings, menu routes and original animation assets.
This is a skin contract check, not an MPC touchscreen emulator.
"""
import hashlib,json,sys
from pathlib import Path
from PIL import Image,ImageChops

work=Path('/Volumes/SSD/Development/BuildCache/rhythmer-os/mpc3-native')
package=work/'install-stage/Jazzy Rhythmer - VST - Rhythmer Engines'
skins=package/'Plugin Skins'
tui=json.loads((skins/'TUI.json').read_text())
tabs=tui['pageData']['tabs']
bindings=json.loads((work/'native-ui-bindings.json').read_text())
qlinks=json.loads((skins/'Q-Links.json').read_text())['Screen Mode Q-Links']['map']
assert len(tabs)==len(qlinks)==12
credits=json.loads((Path(__file__).resolve().parents[1]/'data/engine-credits.json').read_text())['entries']
for n,tab in enumerate(tabs[5:]):
    assert tab['tabName']=='ABOUT' and tab['fnKeyIndex']==5 and tab['fnKeySubIndex']==n
    assert qlinks[5+n]['Tab']==6 and qlinks[5+n]['SubTab']==n+1
    assert all(v==-1 for v in qlinks[5+n]['Q-Links'].values())
    for entry in tab['componentDefinition']['componentsData']:
        x,y,w,h=map(float,entry['bounds']['bounds'].split());assert x>=0 and y>=0 and x+w<=1280 and y+h<=628
about_text=json.loads((skins/'ABOUT-TEXT.json').read_text());assert len(about_text)==7
labels=[e['text']for page in about_text[1:6]for e in page['labels']]
assert all(row['name']in labels for row in credits)
for n,tab in enumerate(tabs[5:]):
 assert tab['qlinkBoundsData']==[]
 page=about_text[n];root=tab['componentDefinition'];assert root['backgroundData']['unfocussed']['image']==page['file']
 image_data=root['componentsData'][0]['componentData'];assert image_data['type']=='Image'and image_data['data']['image']==page['file']
 im=Image.open(skins/page['file']).convert('RGB');assert im.size==(1280,628)
 r,g,b=im.split();assert ImageChops.difference(r,g).getbbox()is None and ImageChops.difference(r,b).getbbox()is None
 assert sum(im.convert('L').point(lambda v:255 if v>128 else 0).histogram()[128:])>5000
assert bindings['parameter_count']==179
for t in tabs[:5]:
 panel=next(e for e in t['componentDefinition']['componentsData']if e['componentData']['type']=='choiceBrowser')
 assert 'Parameter 177'in str(panel['handle remapping'])
 assert panel['bounds']['additionalInvalidatingHandles']==['IndexedEnabling/1/2/Parameter 177']
orders=[list(range(6,14)),list(range(89,97)),list(range(97,105)),list(range(105,113)),list(range(6,14))]
for i,(tab,mapping,expected)in enumerate(zip(tabs,qlinks,orders)):
    q=mapping['Q-Links']
    physical=[13,9,5,1,14,10,6,2]
    assert [q['Q-Link '+str(k)]for k in physical]==expected,(i,q)
    assert mapping['Bank Direction']=='Column'
    for entry in tab['componentDefinition']['componentsData']:
        x,y,w,h=map(float,entry['bounds']['bounds'].split())
        assert x>=0 and y>=0 and x+w<=1280 and y+h<=628,(i,entry['bounds'])
    assert tab['qlinkBoundsData']==[]
play=tabs[0]['componentDefinition']['componentsData']
assert not any(e['componentData'].get('data',{}).get('text')=='ABOUT: CREDITS & LINKS'for e in play)
assert any(e['componentData']['name']=='Engine'and 'Parameter 1'in str(e['handle remapping'])for e in play)
art=[e for e in play if e['componentData']['type']=='Image' and e['componentData']['data'].get('imageType')=='FilmStrip']
assert len(art)==20
for entry in art:
    data=entry['componentData']['data']
    assert data['numFrames']==24 and data['handleName']=='Parameter 25'
    assert 'IndexedEnabling/'in ' '.join(entry['bounds']['additionalInvalidatingHandles'])
for index in bindings['entry_presets'].values():
    assert any('Parameter '+str(index) in str(e['handle remapping']) for e in play),index
kit=tabs[4]['componentDefinition']['componentsData']
for index in bindings['pad_sounds'].values():
    assert any('Parameter '+str(index) in str(e['handle remapping']) for e in kit),index
# A source menu must keep both its source condition AND the kit scope. A
# previous whole-tab scope replacement made all 26 menus overlap each other.
visible_sources=[i for i in range(27)if i!=24]
for source,index in bindings['pad_sounds'].items():
 entry=next(e for e in kit if f'Parameter {index}'in str(e['handle remapping']))
 conditions=entry['bounds']['additionalInvalidatingHandles']
 assert f'IndexedEnabling/{visible_sources.index(int(source))}/26/Parameter 23'in conditions
 assert 'IndexedEnabling/0/20/Parameter 1'in conditions
# Evaluate all declared source conditions: exactly one PAD SOUND menu per source.
import re
pad_menus=[e for e in kit if e['componentData']['name']=='Pad Sound']
for source in range(26):
 shown=[]
 for entry in pad_menus:
  ok=True
  for h in entry['bounds'].get('additionalInvalidatingHandles',[]):
   m=re.fullmatch(r'IndexedEnabling/(\d+)/(\d+)/Parameter (\d+)',h)
   if m:ok=ok and int(m[1])==({1:0,23:source}[int(m[3])])
  if ok:shown.append(entry)
 assert len(shown)==1,(source,len(shown))
replacements={r['key']:r['value']for r in tui['pageData']['replacementStrings']}
root=Path(__file__).resolve().parents[1];catalog=json.loads((root/'data/sample-catalog.json').read_text())
for engine,index in bindings['entry_presets'].items():
 source=catalog['entries'][int(engine)]['source_index']
 assert replacements[f'Parameter {index}']==[s['name']for s in catalog['sources'][source]['sounds']]
for source,index in bindings['pad_sounds'].items():
 assert replacements[f'Parameter {index}']==[s['name']for s in catalog['sources'][int(source)]['sounds']]
assert {e['componentData']['data']['buttonId']for e in kit if e['componentData']['type']=='Button' and e['componentData']['data'].get('numButtonsInGroup')==16}==set(range(16))
compact=next(d['value']for d in tui['pageData']['componentDefinitions']['localComponentDefinitions']if d['key']=='knobSide')
for child in compact['componentsData']:
    x,y,w,h=map(float,child['bounds']['bounds'].split());assert x>=0 and y>=0 and x+w<=278 and y+h<=100
# Poster PAD is a discrete menu; number and per-pad sound use the same
# Parameter 3 as the actual 4x4 grid. It only appears for Rhythmer Kit.
local={d['key']:d['value']for d in tui['pageData']['componentDefinitions']['localComponentDefinitions']}
for key,definition in local.items():
 if key.startswith('browserCard'):
  assert definition['actions']==[]
  assert 'ShowOverlay'not in str(definition)
rows=[e for e in local['choiceBrowser']['componentsData']if e['componentData']['type']=='Button'and e['componentData']['name'].startswith('Choose row')]
assert [e['componentData']['data']['handleName']for e in rows]==['Parameter '+str(i)for i in range(126,142)]
for mapping in qlinks[:5]:
 assert [mapping['Q-Links']['Q-Link '+str(i)]for i in [16,12,8,4]]==[124,174,175,125]
pad=local['posterPad']
assert len(local['choiceBrowser']['componentsData'])>16
assert any(e['componentData']['data'].get('handleName')=='Parameter 125'for e in local['choiceBrowser']['componentsData'])
subtitles=[e for e in pad['componentsData']if e['componentData']['name']=='Selected pad sound'];assert len(subtitles)==16
for i,e in enumerate(subtitles):
 assert e['componentData']['data']['handleName']=='Parameter '+str(73+i)
 assert e['bounds']['additionalInvalidatingHandles']==[f'IndexedEnabling/{i}/16/Parameter 3']
for tab in tabs[:5]:
 selects=[e for e in tab['componentDefinition']['componentsData']if e['componentData']['name']=='Pad']
 assert len(selects)==1
 e=selects[0];assert e['componentData']['type'].startswith('browserCard120x')
 component=local[e['componentData']['type']];assert component['actions']==[]
 assert any(x['componentData']['type']=='Button'and x['componentData']['data']['handleName']=='Parameter 120'for x in component['componentsData'])
 assert 'Parameter 3'in str(e['handle remapping'])
 assert 'IndexedEnabling/0/20/Parameter 1'in e['bounds']['additionalInvalidatingHandles']
assert next(r['value']for r in tui['pageData']['replacementStrings']if r['key']=='Parameter 3')==[f'P{i:02}'for i in range(1,17)]
for key in ['posterMenu','posterMenuWide','posterPad','posterPage','posterPageWide']:
 for entry in local[key]['componentsData']:
  x,y,w,h=map(float,entry['bounds']['bounds'].split());width=598 if key.endswith('Wide')else 278
  assert x>=0 and y>=0 and x+w<=width and y+h<=107,(key,entry['bounds'])
knob=Image.open(skins/'poster-knob.png').convert('RGB');assert knob.size==(58,58*127)
r,g,b=knob.split();assert ImageChops.difference(r,g).getbbox()is None and ImageChops.difference(r,b).getbbox()is None
assert len({hashlib.sha256(knob.crop((0,i*58,58,(i+1)*58)).tobytes()).hexdigest()for i in range(127)})==127
assert not any(e['componentData']['name'].startswith('Section seg ')for t in tabs[1:4]for e in t['componentDefinition']['componentsData'])
assert [t['tabName']for t in tabs[:6]]==['PLAY','EDIT','MOD','FX','KIT','ABOUT']
for tab in tabs[:5]:
 for entry in tab['componentDefinition']['componentsData']:
  for binding in entry['handle remapping']['map']:
   assert 'WithName['not in binding['value']
records=[]
for n in [n for n in range(22)if n not in (17,19)]:
    file=skins/f'art-{n}.png'
    assert file.read_bytes()==(work/'native-art'/file.name).read_bytes()
    im=Image.open(file).convert('RGB');assert im.size==(384,5952)
    r,g,b=im.split();assert ImageChops.difference(r,g).getbbox()is None and ImageChops.difference(r,b).getbbox()is None
    frames=[im.crop((0,f*248,384,(f+1)*248))for f in range(24)]
    assert len({hashlib.sha256(f.tobytes()).hexdigest()for f in frames})>12
    for frame in frames:
        # Ignore <=5/255 Lanczos ringing outside the original three-pixel geometry inset.
        bounds=frame.convert("L").point(lambda value:255 if value>5 else 0).getbbox();assert bounds is not None
        assert bounds[0]>0 and bounds[1]>0 and bounds[2]<384 and bounds[3]<248,(n,bounds)
    records.append(dict(art=n,frames=24,source_asset_match=True,monochrome=True,unclipped=True))
report=dict(passed=True,pages=5,about_pages=7,credited_engines=20,credited_kit_sources=7,parameters=179,source_preset_menus=len(bindings['entry_presets']),pad_sound_menus=len(bindings['pad_sounds']),animation_styles=20,animation_frames=480,q_link_maps='passed',poster_pad='16 numbered choices with matched sound labels; kit-only visibility; in-page browser',poster_knobs='127 monochrome Mac-style arc frames',native_tabs='Host-owned width; no custom tab sizing',about_rendering='7 static Mac-font images, root background plus image widget; no Invalid text handles',animations=records,scope='Packaged skin contract; physical touch/refresh requires MPC hardware')
if len(sys.argv)>1:Path(sys.argv[1]).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k!='animations'},indent=2))
