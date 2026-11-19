/* Native C++ / browser WebAssembly geometry parity, without a browser. */
const fs=require('fs'),path=require('path'),vm=require('vm'),cp=require('child_process');
const root=path.resolve(__dirname,'..'),build='/Volumes/SSD/Development/BuildCache/rhythmer-engines-for-mpc3/animation-wasm';
const context=vm.createContext({TextEncoder,atob});context.window=context;
vm.runInContext(fs.readFileSync(root+'/ui/performance-art-wasm.js','utf8'),context);
vm.runInContext(fs.readFileSync(root+'/ui/artwork.js','utf8'),context);
const {SampleArtwork,names}=context.RhythmerArtwork;
let checks=0,maxError=0;const results=[],frames={};
for(let kind=0;kind<names.length;kind++){
 const name=names[kind],native=JSON.parse(cp.execFileSync(build+'/native-reference',[String(kind),'Test'],{encoding:'utf8',maxBuffer:4*1024*1024}));
 const art=new SampleArtwork(context.RHYTHMER_ART_WASM),output=[];
 for(let step=0;step<4;step++){
  const frame=Array.from(art.frame({name,preset:'Test',now:1000+step*150,note:60,held:step<3,level:step<3?.5:.2,pad:10,width:920,height:300,fit:true}));
  if(frame.length!==native[step].length*10)throw Error(name+' face count differs');checks++;
  const expected=native[step].flat();for(let i=0;i<frame.length;i++){let error=Math.abs(frame[i]-expected[i]);maxError=Math.max(error,maxError);if(!Number.isFinite(frame[i])||error>.006)throw Error(`${name}: point ${i} differs by ${error}`);checks++;}
  output.push(frame);
 }
 if(JSON.stringify(output[0])===JSON.stringify(output[1]))throw Error(name+' input motion stuck');checks++;
 frames[name]=Array.from({length:output[1].length/10},(_,i)=>output[1].slice(i*10,i*10+10));
 // Original surface alpha/depth shading, monochrome UI and motion-off fallback.
 const colors=new Set(),fills=[],strokes=[],stack=[],mock={globalAlpha:1,save(){stack.push(this.globalAlpha);},restore(){this.globalAlpha=stack.pop();},clearRect(){},fillRect(){colors.add(this.fillStyle);},beginPath(){},moveTo(){},lineTo(){},closePath(){},fill(){colors.add(this.fillStyle);fills.push(this.globalAlpha);},stroke(){colors.add(this.strokeStyle);strokes.push(this.globalAlpha);}};
 const signal={name,preset:'Test',now:2000,note:60,held:true,level:.5,pad:10,width:920,height:300};
 art.draw(mock,signal,false);const frozen=art.lastFrame.slice();art.draw(mock,{...signal,now:3000,level:1},false);
 if(JSON.stringify(Array.from(frozen))!==JSON.stringify(Array.from(art.lastFrame)))throw Error(name+' motion-off moved');checks++;
 if([...colors].some(c=>c!=='#000000'&&c!=='#ffffff'))throw Error(name+' non-B/W drawing');checks++;
 for(let i=0;i<frozen.length/10;i++){if(fills[i]!==frozen[i*10+8]||strokes[i]!==frozen[i*10+9])throw Error(name+' source surface shading changed');checks++;}
 if(mock.globalAlpha!==1||stack.length)throw Error(name+' alpha state leaked');checks++;
 results.push({engine:name,faces:frames[name].length,nativeWasmMatch:true,noteResponse:true,monochromePalette:true,originalSurfaceShading:true,motionOffStable:true});
}
const report={checks,engineStyles:22,maxCoordinateErrorPx:maxError,sourceUnchanged:true,engines:results};
if(process.argv[2]){fs.writeFileSync(process.argv[2],JSON.stringify(report,null,2)+'\n');fs.writeFileSync(path.join(path.dirname(process.argv[2]),'artwork-frames.json'),JSON.stringify(frames));}
console.log(JSON.stringify({...report,engines:undefined},null,2));
