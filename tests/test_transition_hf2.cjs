// Offline VM test: no MIDI access, no flash transport, no network.
const fs=require('fs'),vm=require('vm'),crypto=require('crypto'),assert=require('assert'),path=require('path');
async function run(version){
 const html=fs.readFileSync(`/mnt/data/b1_installer_hf2/SikkenWorks-B1-V${version}-SLOOP-Transition-Installer-HF2.html`,'utf8');
 const script=[...html.matchAll(/<script([^>]*)>([\s\S]*?)<\/script>/g)].find(m=>m[2].includes('const VERIFIED_RUNNING_FIRMWARE'))[2];
 const body=script.replace('document.addEventListener(\'DOMContentLoaded\',()=>{','root.__testAcceptConnection=acceptConnection; root.__testVerifiedPackage=verifyPackage; document.addEventListener(\'DOMContentLoaded\',()=>{');
 let domready,accessCount=0; const nodes=new Map();
 function el(id){if(!nodes.has(id)){ const e={id,hidden:false,checked:false,disabled:false,value:'',textContent:id==='firmware-embedded'?html.match(/<script id="firmware-embedded" type="application\/octet-stream">([\s\S]*?)<\/script>/)[1]:'',innerText:'',files:[],style:{},dataset:{},listeners:{},classList:{toggle(){}},addEventListener(type,fn){this.listeners[type]=fn;},dispatchEvent(evt){this.listeners[evt.type]?.(evt);},click(){this.listeners.click?.();},appendChild(){},remove(){},getContext(){return {clearRect(){},beginPath(){},moveTo(){},lineTo(){},stroke(){}}}};nodes.set(id,e)}return nodes.get(id)}
 const doc={getElementById:el,querySelectorAll(){return[]},createElement(tag){return el('temp-'+tag)},addEventListener(type,fn){if(type==='DOMContentLoaded')domready=fn;}};
 const ctx={document:doc,console,Uint8Array,ArrayBuffer,Date,Math,Set,Promise,String,Number,Array,Error,RegExp,Boolean,parseInt,JSON,navigator:{requestMIDIAccess:()=>{accessCount++;throw Error('Test does not allow MIDI')}},setTimeout:(fn,_ms)=>{return setImmediate(fn)},clearTimeout,performance:{now:()=>0},atob:s=>Buffer.from(s,'base64').toString('binary'),isSecureContext:true,confirm:()=>false,crypto:crypto.webcrypto,module:{exports:{}}};ctx.window=ctx;vm.createContext(ctx);vm.runInContext(body,ctx,{timeout:15000,filename:'installer.js'});assert(typeof domready==='function'); domready(); await new Promise(r=>setTimeout(r,500));
 assert.equal(el('connect').disabled,false,'embedded original firmware must be approved');
 const fwsc=fs.readFileSync(`/mnt/data/SikkenWorks-B1-V${version}-BEETLE-OTA.fwsc`);
 const inHtml=Buffer.from(el('firmware-embedded').textContent.replace(/\s/g,''),'base64');assert(fwsc.equals(inHtml),'must not modify .fwsc');
 const result=await ctx.__testVerifiedPackage(new Uint8Array(fwsc),'etch');assert.equal(result.hash,crypto.createHash('sha256').update(fwsc).digest('hex'));assert.equal(result.product,version==='4.2'?'FM-1_2144':'FM-1_2145');
 const fakeLink=()=>({sent:[],send(x){this.sent.push(x);return true},close(){return Promise.resolve()}});
 const d={id:{text:'FM-1_900',loader:false},link:fakeLink()};
 ctx.__testAcceptConnection(d);assert.equal(el('install').disabled,false,'SLOOP should be accepted with Advanced OFF');assert.equal(el('freedom-enable').checked,false);
 const adv=el('freedom-enable'); adv.checked=true;adv.dispatchEvent({type:'change'});adv.checked=false;adv.dispatchEvent({type:'change'});assert.equal(el('install').disabled,false,'source must not depend on Advanced Mode');
 for(const s of ['SLOOP','Clipper']) assert(body.toLowerCase().includes(s.toLowerCase()),'Felucca-derived source named '+s+' in UI help or MIDI port detection');
 const E=ctx.ETCHOta.Engine;
 for (const loader of ['ota-FM-1_900','ota-FM-1_906','ota-FM-1_015']){
  let waits=0, stages=[],checks=0;const engine=new E({},()=>{},(...args)=>{stages.push(args[0])});
  engine.serve=async(_link,_img,finish)=>{checks++;assert([0xE0000000,0xF0000000].includes(finish));return 16};
  engine.waitFor=async(predicate)=>{waits++; const id=waits===1?{text:loader,loader:true}:{text:result.product,loader:false}; assert(predicate(id),'expected recognized loader or target ID');return {id,link:fakeLink()}};
  await engine.install({id:{text:'FM-1_900',loader:false},link:fakeLink()},result);
  assert.equal(checks,2);assert(stages.includes('check')&&stages.includes('write')&&stages.includes('reboot'));
 }
 for(const loader of ['ota-FM-1_123','ota-FM-1_666','ota-OTHER_900']){
  const engine=new E({},()=>{},()=>{});let checks=0,flash=false;engine.serve=async()=>{checks++;if(checks===2)flash=true;return 3};engine.waitFor=async(predicate)=>{const id={text:loader,loader:true};assert.equal(predicate(id),false);return null};
  await assert.rejects(engine.install({id:{text:'FM-1_900',loader:false},link:fakeLink()},result),/No compatible OTA loader|instead of entering/); assert.equal(flash,false);
 }
 assert.equal(accessCount,0,'Test should not access a real MIDI device');
 console.log(`PASS V${version} SLOOP connects without Advanced; loader 900/906/015; unsupported rejected before FLASH; SHA-256 and bytes unchanged`);
}
(async()=>{await run('4.2');await run('4.3')})().catch(err=>{console.error(err);process.exitCode=1});
