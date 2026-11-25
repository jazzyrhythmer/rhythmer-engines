/* Run with Node: node tests/control-model.test.js [result.json]
 * These tests verify reference control/state logic, not native MPC audio.
 */
const fs=require('fs'),path=require('path'),assert=require('assert');
const root=path.resolve(__dirname,'..');
const {ControlModel}=require('../src/control-model.js');
const c=JSON.parse(fs.readFileSync(path.join(root,'data/sample-catalog.json')));
let checks=0,programs=0,kitPads=0;const issues=[];
function check(value,message){checks++;if(!value)issues.push(message);}
const m=new ControlModel(c);
check(c.entries.length===22,'22 picker entries');check(c.sources.length===31,'31 source engines');
check(c.kits.length===69,'69 kits');
const ids=new Set();let parameters=0,aliases=0;
for(const source of c.sources){
 check(source.program_states?.length===source.sounds.length,source.name+' factory state coverage');
 const nativeIndices=new Set();for(const p of source.parameters){parameters++;check(!ids.has(p.id),'duplicate stable ID '+p.id);ids.add(p.id);check(!nativeIndices.has(p.index),'duplicate native index '+p.id);nativeIndices.add(p.index);}
 aliases+=(source.native_extra_aliases||[]).length;
 for(const ref of source.program_states||[]){check(ref.values.every(v=>Number.isFinite(v)&&v>=0&&v<=1),'finite source state '+source.id);}
}
check(parameters===4689,'all 4,689 source parameters retained');
for(const e of c.entries){
 m.selectEngine(e.id);
 check(m.primary().length===8,e.name+' eight primary controls');
 check(new Set(m.primary().map(p=>p.id)).size===8,e.name+' primary controls unique');
 for(const section of ['SOUND','MOD','FX','ALL']){
  const expected=m.controls(section,true).map(p=>p.id).sort();
  const actual=m.pages(section,true).flatMap(g=>g.parameters.map(p=>p.id)).sort();
  check(JSON.stringify(actual)===JSON.stringify(expected),e.name+' untruncated '+section+' pages');
 }
 if(e.kind!=='source')continue;
 for(let index=0;index<m.entrySource.sounds.length;index++){
  m.state.program=index;m.state.values={};programs++;
  const primary=m.primary();check(primary.length===8,e.name+' program '+index+' eight controls');
  for(const p of primary){check(m.active(p),e.name+' active primary '+p.id);check(m.nativeValue(p)!==null,e.name+' real source value '+p.id);}
 }
 m.state.program=0;
}
m.selectEngine('rhythmer-kit');
for(let kit=0;kit<c.kits.length;kit++){
 m.selectKit(kit);check(m.state.routes.length===16,'kit '+kit+' full pad assignment');
 for(let pad=0;pad<16;pad++){
  m.selectPad(pad);kitPads++;const route=m.state.routes[pad];check(!!c.sources[route.engine]?.sounds[route.sound],'valid kit '+kit+' pad '+pad);
  const ps=m.primary();check(ps.length===8,'kit '+kit+' pad '+pad+' eight controls');
  check(ps[0].scope==='common'&&ps[1].scope==='common','kit common filter first');
  const p=ps.find(p=>p.scope!=='common'),old=m.nativeValue(p),value=old===p.maximum?p.minimum:p.maximum;
  const originalOther=JSON.stringify(m.state.pads[(pad+1)%16].values);check(m.write(p,value),'write selected pad');
  check(JSON.stringify(m.state.pads[(pad+1)%16].values)===originalOther,'pad write isolation');
  const rev=m.contextRevision;m.selectPad((pad+1)%16);check(!m.write(p,p.minimum,rev),'stale pad gesture rejected');m.selectPad(pad);
 }
}
// Kit-only sources remain available inside Rhythmer Kit, with every program.
for(const source of c.sources.filter(s=>s.kit_only)){
 m.replacePadSource(source.source_index);
 for(let index=0;index<source.sounds.length;index++){
  m.state.routes[m.pad].sound=index;m.state.pads[m.pad].values={};programs++;
  check(m.primary().length===8,source.name+' kit-source '+index+' eight controls');
 }
}
// Exact state and Undo/Redo including focused pad and all engine edits.
m.selectPad(10);m.checkpoint();let p=m.primary().find(p=>p.scope!=='common');let before=m.nativeValue(p);m.write(p,p.maximum);check(m.undo(),'Undo available');check(m.nativeValue(p)===before,'Undo restores value');check(m.pad===10,'Undo selected pad');check(m.redo(),'Redo available');check(m.nativeValue(p)===p.maximum,'Redo restores value');
const saved=m.serialize(),other=new ControlModel(c);other.deserialize(saved);check(other.serialize()===saved,'complete state round trip');
// Program wrap and independent engine edit retention.
let f=c.entries.find(e=>e.name==='Frodhes');m.selectEngine(f.id);m.selectProgram(-1);check(m.state.program===56,'factory previous wraps to end');m.selectProgram(57);check(m.state.program===0,'factory next wraps to start');p=m.primary()[2];m.write(p,p.maximum);const stable=p.id;m.selectEngine(c.entries.find(e=>e.name==='Tenebrae').id);m.selectEngine(f.id);check(m.primary().some(p=>p.id===stable&&m.nativeValue(p)===p.maximum),'engine edits retained');
check(!c.entries.some(e=>['Acid Engine','Break Engine'].includes(e.name)),'dedicated Acid and Break engines excluded');
const report={checks,programs,kitPads,parameters,helperAliases:aliases,issues,audioValidated:false,mpc32BuildValidated:false};
if(process.argv[2])fs.writeFileSync(process.argv[2],JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify(report,null,2));if(issues.length)process.exitCode=1;
