#!/usr/bin/env python3
"""Source patch for the cross-firmware UFW bridge. Only use on vetted local template."""
from pathlib import Path
p=Path(__file__).resolve().parent.parent/'B1-WebInstaller.html'
s=p.read_text()
if 'const AUDIT_LOADER_SHA' in s: print('Already patched'); raise SystemExit
s=s.replace('<button class="choice" id="type-stock"', '<button class="choice" id="type-generic" type="button"><span class="label">COMPATIBLE FM1 PACKAGE</span><b>OTHER CUSTOM .FWSC</b><small>STRUCTURE + LOADER AUDIT</small></button><button class="choice" id="type-stock"')
s=s.replace('<div id="b1-file-field"', '''<div class="file-field" id="generic-file-field" hidden>
        <label for="generic-file">Choose the original FM1 firmware package (.fwsc) — BEETLE-compatible loader only</label>
        <input id="generic-file" type="file" accept=".fwsc" />
        <p class="soft">The package must contain a structurally valid UFW image, the exact preserved FM1 boot header, and the same byte-pinned BEETLE OTA loader. Other firmware formats require the separate UBOOT handoff below. No blind flashing.</p>
      </div><div id="b1-file-field"''')
s=s.replace('External firmware / UBOOT recovery handoff</summary>', 'Universal firmware switching / UBOOT</summary>')
s=s.replace('This does not install or approve arbitrary firmware. It requests the B1 app to enter the existing FM1 UBOOT mode. After USB reconnects, close this page and use the external firmware\'s documented updater.', 'For ANY different loader or boot-image layout, B1 requests entry to the independent FM1 UBOOT mode. After reconnecting, use the target firmware\'s own compatible updater. This button does NOT write or approve the target firmware.')
s=s.replace('const FREEDOM_PROFILES =', '''/* Structural audit for third-party packages that use the same trusted FM1
 * loader and preserved flash header. No version allowlist is used for the
 * TARGET firmware. The device-side OTA gate still checks loader bytes.
 * Unsupported format or loader -> independent UBOOT, never a forced write. */
const AUDIT_LOADER_SHA = 'd0b1a05c63030b58e5be06b93751e6cc375d3d34eec584109f97fd31f0a42a78';
function crc16Jl(bytes){let c=0;for(const x of bytes){c^=x<<8;for(let k=0;k<8;k++)c=(c&0x8000)?((c<<1)^0x1021)&0xffff:(c<<1)&0xffff;}return c;}
function decryptJl(bytes){let k=0xffff,out=new Uint8Array(bytes.length);for(let i=0;i<bytes.length;i++){out[i]=bytes[i]^(k&255);k=((k<<1)^((k&0x8000)?0x1021:0))&0xffff;}return out;}
function u16(x,o){return x[o]|x[o+1]<<8;}
function verifyFwscLayout(raw){
 const logical=logicalImage(raw);
 if(logical.length<0x400)throw new Error('FWSC UFW header truncated.');
 const h=decryptJl(logical.subarray(0,64)),count=u16(h,8);
 if(count<1||count>11||crc16Jl(h.subarray(2,64))!==u16(h,0))throw new Error('UFW header CRC or entry count invalid.');
 if(crc16Jl(logical.subarray(64,64+count*80))!==u16(h,2))throw new Error('UFW entry-table CRC mismatch.');
 const entries=[];
 for(let i=0;i<count;i++){
  const ent=decryptJl(logical.subarray(64+i*80,64+(i+1)*80));
  const type=u16(ent,0),crc=u16(ent,4),off=u32(ent,8),len=u32(ent,12);
  if(len===0||off<1024||off>logical.length||len>logical.length-off)throw new Error('UFW entry outside image: '+type);
  const body=logical.subarray(off,off+len);
  if(crc16Jl(body)!==crc)throw new Error('UFW CRC mismatch for entry type '+type);
  if(entries.some(e=>e.type===type))throw new Error('Duplicate UFW entry type '+type);
  entries.push({type,off,len,body});
 }
 const flash=entries.find(e=>e.type===0),ota=entries.find(e=>e.type===100);
 if(!flash||!ota||flash.len!==0x93000)throw new Error('This custom FM1 package has an incompatible flash layout. Use UBOOT and its original installer.');
 if(ota.len!==6930||u32(ota.body,8)!==6898)throw new Error('This custom FM1 package uses a different OTA loader. Use the independent UBOOT route and the firmware installer.');
 return {flash,ota};
}
async function auditGenericPackage(raw){
 if(!(raw instanceof Uint8Array))raw=new Uint8Array(raw);
 const product=productOf(raw);
 if(!/^FM-1_\\d+$/.test(product))throw new Error('Not an M-VAVE FM1 .fwsc image.');
 const {flash,ota}=verifyFwscLayout(raw);
 const actualLoader=await digestSHA(ota.body.subarray(32));
 if(actualLoader!==AUDIT_LOADER_SHA)throw new Error('This firmware has a different OTA loader; B1 will not flash it through the app. Use ENTER UBOOT and the firmware original updater.');
 const headHash=await digestSHA(flash.body.subarray(0,0x4000));
 if(headHash!==C.KNOWN_HEAD_HASH)throw new Error('The target has different protected boot bytes. Use the independent UBOOT route and its original installer.');
 return {kind:'generic',raw,product,hash:await digestSHA(raw),image:logicalImage(raw),freedomProfile:null};
}
const FREEDOM_PROFILES =''')
s=s.replace("if(kind!=='etch'&&kind!=='stock'&&kind!=='freedom')", "if(kind==='generic')return auditGenericPackage(raw);\n  if(kind!=='etch'&&kind!=='stock'&&kind!=='freedom')")
s=s.replace("if(type!=='etch'&&type!=='stock'&&type!=='freedom')", "if(type!=='etch'&&type!=='stock'&&type!=='freedom'&&type!=='generic')")
s=s.replace("$('type-stock').classList.toggle('selected',type==='stock');", "$('type-stock').classList.toggle('selected',type==='stock');$('type-generic').classList.toggle('selected',type==='generic');")
s=s.replace("$('stock-field').hidden=type!=='stock';", "$('stock-field').hidden=type!=='stock';$('generic-file-field').hidden=type!=='generic';")
s=s.replace("$('b1-file-field').hidden=type!=='etch';", "$('b1-file-field').hidden=type!=='etch';")
s=s.replace("'FIRMWARE FREEDOM / APPROVED TARGET';", "type==='generic'?'COMPATIBLE FM1 CUSTOM FIRMWARE':'FIRMWARE FREEDOM / APPROVED TARGET';")
s=s.replace("if(type==='freedom')$('firmware-sub').textContent=", "if(type==='generic')$('firmware-sub').textContent='Cross-firmware route: upload any original .fwsc with the same protected FM1 boot sector and pinned OTA loader. Other formats must use the UBOOT handoff; no format check is disabled.';\n if(type==='freedom')$('firmware-sub').textContent=")
s=s.replace("type==='stock'?'02 / RESTORE STOCK':'02 / INSTALL B1'", "type==='stock'?'02 / RESTORE STOCK':type==='generic'?'02 / SWITCH FIRMWARE':'02 / INSTALL B1'")
s=s.replace("$('stock-file').value='';", "$('stock-file').value='';$('generic-file').value='';")
s=s.replace(" }else if(choice==='stock'){", " }else if(choice==='generic'){\n   const file=$('generic-file').files[0];if(!file){setMessage('Select an FM1 .fwsc file or choose UBOOT for an incompatible loader.');return;}\n   if(!file.name.toLowerCase().endsWith('.fwsc'))throw new Error('Original FM1 .fwsc required.');\n   raw=new Uint8Array(await file.arrayBuffer());\n }else if(choice==='stock'){")
s=s.replace("const selectedProfile=type==='freedom'?$('freedom-target').value:null;", "const selectedProfile=type==='freedom'?$('freedom-target').value:null;")
s=s.replace("if(!isSupportedEtchIdentity(initial.id.text))", "if(!isSupportedEtchIdentity(initial.id.text))")
s=s.replace("if(!isSupportedEtchIdentity(dev.id.text))throw new Error('Unrecognised running FM1 firmware. Use its documented recovery method; no command sent.');", "if(!/^FM-1_\\d+$/.test(dev.id.text))throw new Error('The firmware does not identify as an FM-1 running application. No UBOOT request sent.');")
s=s.replace("if(!isSupportedEtchIdentity(dev.id.text)) {", "if(!isSupportedEtchIdentity(dev.id.text)) {")
s=s.replace("'Current source identity is not recognized.'", "'Current source identity is not recognized.'")
s=s.replace(" $('type-stock').addEventListener('click'", " $('type-generic').addEventListener('click',()=>{if($('freedom-enable').checked){$('freedom-enable').checked=false;$('freedom-controls').hidden=true;}choosePackage('generic');});\n $('type-stock').addEventListener('click'")
s=s.replace(" $('stock-file').addEventListener('change'", " $('generic-file').addEventListener('change',()=>loadSelected().catch(e=>{approvedPackage=null;$('connect').disabled=true;$('install').disabled=true;setMessage(e.message,'error');}));\n $('stock-file').addEventListener('change'")
s=s.replace('verifyPackage,FREEDOM_PROFILES,freedomSourceAllowed', 'verifyPackage,verifyFwscLayout,auditGenericPackage,FREEDOM_PROFILES,freedomSourceAllowed')
p.write_text(s)
print('Bridge UI and package audit patched')
