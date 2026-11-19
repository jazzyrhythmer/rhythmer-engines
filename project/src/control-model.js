/* SPDX-License-Identifier: GPL-3.0-or-later
 * Shared, deterministic source-ID bindings. No synthetic factory patches.
 * This UI model does not implement audio or substitute for MPC DSP validation.
 */
(function(root){
'use strict';
const clone=x=>JSON.parse(JSON.stringify(x));
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
class ControlModel {
 constructor(catalog){this.catalog=catalog;this.entryId=catalog.entries.find(e=>e.kind==='kit').id;this.sessions={};this.undoStack=[];this.redoStack=[];this.contextRevision=0;this.pad=0;this.ensure();}
 get entry(){return this.catalog.entries.find(e=>e.id===this.entryId);}
 ensure(){if(!this.sessions[this.entryId]){this.sessions[this.entryId]={program:0,values:{},kit:0,routes:clone(this.catalog.kits[0].routes),pads:Array.from({length:16},()=>({values:{}})),common:{cutoff:100,resonance:0,type:0,level:100},userName:''};}return this.sessions[this.entryId];}
 get state(){return this.ensure();}
 get entrySource(){if(this.entry.kind==='kit')return this.catalog.sources[this.state.routes[this.pad].engine];if(this.entry.kind==='break')return this.catalog.break_source;return this.entry.kind==='source'?this.catalog.sources[this.entry.source_index]:null;}
 get sound(){return this.entrySource?.sounds[this.entry.kind==='kit'?this.state.routes[this.pad].sound:this.state.program]||null;}
 get reference(){return this.entrySource?.program_states?.[this.entry.kind==='kit'?this.state.routes[this.pad].sound:this.state.program]||null;}
 get source(){if(this.reference)return this.catalog.sources[this.reference.source];return this.catalog.sources.find(s=>s.folder===this.sound?.source_engine)||this.entrySource;}
 get soundName(){return this.entry.kind==='kit'?this.catalog.kits[this.state.kit]?.name||'User Kit':this.sound?.name||'Load a loop';}
 get values(){return this.entry.kind==='kit'?this.state.pads[this.pad].values:this.state.values;}
 checkpoint(){this.undoStack.push(this.snapshot());if(this.undoStack.length>80)this.undoStack.shift();this.redoStack=[];}
 snapshot(){return {entryId:this.entryId,pad:this.pad,sessions:clone(this.sessions)};}
 restore(x){if(!this.catalog.entries.some(e=>e.id===x.entryId))throw Error('Unknown engine');this.entryId=x.entryId;this.pad=clamp(Number(x.pad)||0,0,15);this.sessions=clone(x.sessions);this.ensure();this.contextRevision++;}
 undo(){if(!this.undoStack.length)return false;let old=this.undoStack.pop();this.redoStack.push(this.snapshot());this.restore(old);return true;}
 redo(){if(!this.redoStack.length)return false;let next=this.redoStack.pop();this.undoStack.push(this.snapshot());this.restore(next);return true;}
 selectEngine(id){if(id===this.entryId)return;if(!this.catalog.entries.some(e=>e.id===id))throw Error('Unknown engine');this.entryId=id;this.ensure();this.contextRevision++;}
 selectPad(index){this.pad=clamp(index,0,15);this.contextRevision++;}
 selectProgram(index){if(!this.entrySource)return;const n=this.entrySource.sounds.length;index=((index%n)+n)%n;this.checkpoint();if(this.entry.kind==='kit'){this.state.routes[this.pad].sound=index;delete this.state.routes[this.pad].mix_values;this.state.pads[this.pad].values={};}else{this.state.program=index;this.state.values={};}this.state.userName='';this.contextRevision++;}
 selectKit(index){index=((index%this.catalog.kits.length)+this.catalog.kits.length)%this.catalog.kits.length;this.checkpoint();this.state.kit=index;this.state.routes=clone(this.catalog.kits[index].routes);this.state.pads=Array.from({length:16},()=>({values:{}}));this.state.userName='';this.contextRevision++;}
 replacePadSource(sourceIndex){if(this.entry.kind!=='kit')throw Error('Not a kit');if(!this.catalog.sources[sourceIndex])throw Error('Unknown source');this.checkpoint();this.state.routes[this.pad]={engine:sourceIndex,sound:0};this.state.pads[this.pad].values={};this.contextRevision++;}
 nativeValue(p){if(p.scope==='common')return this.state.common[p.key];if(p.key in this.values)return this.values[p.key];const normalized=(this.entry.kind==='kit'?this.state.routes[this.pad].mix_values?.[String(p.index)]:undefined)??this.reference?.values?.[p.index]??this.sound?.values?.[String(p.index)];if(Number.isFinite(normalized)){const v=p.minimum+normalized*(p.maximum-p.minimum);return p.type==='int'||p.type==='enum'||p.options?.length?Math.round(v):v;}return p.default_known?p.default:null;}
 uiValue(p){return this.nativeValue(p)??(p.minimum+(p.maximum-p.minimum)/2);}
 active(p){if(p.scope==='common')return true;const s=this.source;if(!s)return false;if(p.hidden_in_sample||p.is_program_selector||!p.name.trim())return false;let label=this.label(p);if(!label.trim()||/^[- ]+$/.test(label)||/^(Unused|Inactive Parameter)/i.test(label))return false;
 if(['pad','focused_pad','ui_current_pad','ui_focus_level'].includes(p.key))return false;
 if(this.sound?.native_key===p.key)return false;
 if(p.dynamic_labels){let sel=s.parameters.find(x=>x.index===p.dynamic_selector);let value=sel?this.nativeValue(sel):null;if(value!==null&&p.dynamic_labels[Math.round(value)]==='-')return false;}
 const note=this.sound?.note??60;let allowed=this.sound?.parameter_indices||s.voice_parameters?.[String(note)];if(allowed&&!allowed.includes(p.index))return false;
 const voice=note-35;const pref='p'+String(voice).padStart(2,'0')+'_';if(s.drums&&/^p\d\d_/.test(p.key)&&!p.key.startsWith(pref))return false;
 if(s.focus_voice&&s.drums){if(/^v\d_/.test(p.key)&&Number(p.key[1])!==voice)return false;if(/^syn\d+$/.test(p.key)&&Number(p.key.slice(3))!==voice)return false;}
 if(p.active_models){let key=(s.model_control_prefix||'')+String(voice);let selector=s.parameters.find(x=>x.key===key);let model=selector?this.nativeValue(selector):null;if(model!==null&&!p.active_models.includes(Math.round(model)))return false;}
 return true;}
 label(p){if(p.scope==='common')return p.label;let name=p.label||p.name||p.key;const live=this.reference?.names?.[p.index];if(this.source?.drums||this.entry.name==='Acid Engine')if(live)name=live;const strip=this.entrySource?.strip_voice_prefix;if(strip&&name.startsWith(strip))name=name.slice(strip.length);if(p.dynamic_labels){let sel=this.source?.parameters.find(x=>x.index===p.dynamic_selector);let v=sel?this.nativeValue(sel):null;if(v!==null)name=p.dynamic_labels[clamp(Math.round(v),0,p.dynamic_labels.length-1)]||name;}const words={CUTOF:'Cutoff',RESO:'Resonance',ENVMD:'Env Amount',ACCNT:'Accent',TUNE:'Tune',DECAY:'Decay',WAVE:'Wave',GATE:'Gate',SDEC:'Sweep Decay'};if(this.entry.name==='Acid Engine')name=words[name]||name;return name.replace(/Frequncy/g,'Cutoff').replace(/Resnance/g,'Resonance');}
 commonControls(){if(this.entry.kind!=='kit'&&!this.source?.output_filter)return [];return ['cutoff','resonance','type'].map((key,i)=>({key,id:this.entry.id+'/rack/'+key,scope:'common',label:(this.entry.kind==='kit'?'Kit ':'Output ')+['Cutoff','Resonance','Filter Type'][i],name:key,index:-3+i,minimum:0,maximum:i===2?2:100,default:i===0?100:0,default_known:true,type:'int',unit:i<2?'%':'',options:i===2?['LP','BP','HP']:[],section:'SOUND',group:this.entry.kind==='kit'?'KIT FILTER':'OUTPUT FILTER',native_ui_order:-100}));}
 controls(section='ALL',raw=false){if(!this.source)return [];return [...this.commonControls(),...this.source.parameters].filter(p=>(raw||this.active(p))&&(section==='ALL'||p.section===section)).sort((a,b)=>(a.native_ui_order??a.edit_group_order??1000)-(b.native_ui_order??b.edit_group_order??1000)||a.index-b.index);}
 groups(section='ALL',raw=false,query=''){let groups=[];let q=query.toLowerCase();for(const p of this.controls(section,raw)){if(q&&!`${p.label} ${p.name} ${p.group} ${p.key}`.toLowerCase().includes(q))continue;let g=groups.find(g=>g.name===p.group);if(!g)groups.push(g={name:p.group,parameters:[]});g.parameters.push(p);}return groups;}
 pages(section='ALL',raw=false,query=''){let out=[];for(const g of this.groups(section,raw,query)){for(let i=0;i<g.parameters.length;i+=8)out.push({name:g.name,part:Math.floor(i/8)+1,parts:Math.ceil(g.parameters.length/8),parameters:g.parameters.slice(i,i+8)});}return out;}
 primary(){let controls=this.controls(),out=[];const add=p=>{if(p&&this.active(p)&&!out.some(x=>x.id===p.id))out.push(p);};
 // Resolve repeated drum candidates against the current source voice first.
 for(const p of this.commonControls().slice(0,2))add(p);
 const profile=this.source?.primary_candidates||[];
 // Explicit profiles take priority; repeated voice candidates resolve below.
 if(['Frodhes','Tenebrae','FM-106','FMini-1','Prophet','Guru-600','Aphex','Elixir'].includes(this.source?.name))for(const key of profile.slice(0,8))add(controls.find(p=>p.key===key&&p.scope!=='common'));
 if(this.entry.kind!=='kit')for(const keys of this.source?.quick_controls||[])for(const key of keys){let p=controls.find(p=>p.key===key&&p.scope!=='common');if(p){add(p);break;}}
 // Envelope and output are preferable to program/settings controls.
 if(this.entry.kind==='kit'){for(const role of ['volume','tune','decay','pan'])add(controls.find(p=>p.kit_mix_role===role));}
 for(const key of this.source?.primary_candidates||[])add(controls.find(p=>p.key===key));
 for(const p of controls)add(p);
 return out.slice(0,8);}
 write(p,value,revision=this.contextRevision){if(revision!==this.contextRevision)return false;if(!this.source||!(p.scope==='common'||this.source.parameters.some(x=>x.id===p.id))||!this.active(p))return false;value=clamp(Number(value),p.minimum,p.maximum);if(!Number.isFinite(value))return false;if(p.options?.length||p.type==='int'||p.type==='enum')value=Math.round(value);if(p.scope==='common')this.state.common[p.key]=value;else this.values[p.key]=value;if(p.key==='break_perf_6'&&(this.values.break_perf_7??100)>value*10)this.values.break_perf_7=value*10;return true;}
 format(p){let v=this.nativeValue(p);if(v===null)return 'SOURCE VALUE';if(p.options?.length)return p.options[clamp(Math.round(v-p.minimum),0,p.options.length-1)]||String(v);if(p.display_native_percent||p.display_percentage)return Math.round(p.display_native_percent?v*100:(v-p.minimum)/Math.max(1,p.maximum-p.minimum)*100)+' %';return Number(v.toFixed(3))+(p.unit?' '+p.unit:'');}
 serialize(){return JSON.stringify({schema:1,...this.snapshot()});}
 deserialize(text){let value=JSON.parse(text);if(value.schema!==1)throw Error('Unsupported preset format');this.checkpoint();this.restore(value);}
}
const api={ControlModel,clone,clamp};if(typeof module!=='undefined'&&module.exports)module.exports=api;root.RhythmerControls=api;
})(typeof globalThis!=='undefined'?globalThis:this);
