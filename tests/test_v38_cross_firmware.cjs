const fs=require('fs'),vm=require('vm'),crypto=require('crypto'),assert=require('node:assert/strict'),path=require('path');
const root=path.resolve(__dirname,'..');
const html=fs.readFileSync(path.join(root,'B1-WebInstaller.html'),'utf8');
const m=html.match(/<script>\/\* SPDX-License-Identifier: GPL-3\.0-only([\s\S]*?)<\/script>/);
assert(m);
const ctx={module:{exports:{}},Uint8Array,Array,Math,Number,Date,Promise,console,TextEncoder,crypto:crypto.webcrypto,atob:global.atob,setTimeout};
vm.runInNewContext('/* SPDX-License-Identifier: GPL-3.0-only'+m[1],ctx,{filename:'WebInstaller.js'});
const api=ctx.module.exports;
const fw=fs.readFileSync(path.join(root,'build/SikkenWorks-B1-V3.8-BEETLE-OTA.fwsc'));
const ref=fs.readFileSync(path.join(root,'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc'));
const enc=html.match(/<script id="firmware-embedded" type="application\/octet-stream">\s*([^<]+)\s*<\/script>/);assert(enc&&Buffer.from(enc[1],'base64').equals(fw));
(async()=>{
  let current=await api.verifyPackage(new Uint8Array(fw),'etch');assert.equal(current.product,'FM-1_2138');
  for (const [source, data] of [['BEETLE',ref], ['B1 V3.8',fw]]){
   let pkg=await api.verifyPackage(new Uint8Array(data),'generic');
   console.log('PASS compatible target:',source,pkg.product,pkg.hash.substring(0,16));
  }
  const changed=Buffer.from(fw);changed[7000]^=2;
  await assert.rejects(async()=>api.verifyPackage(new Uint8Array(changed),'generic'),/CRC mismatch/);
  const corrupt=Buffer.from(ref);corrupt[1180]^=5;
  await assert.rejects(async()=>api.verifyPackage(new Uint8Array(corrupt),'generic'),/CRC mismatch|header CRC/);
  const truncated=new Uint8Array(fw.subarray(0,32000));
  await assert.rejects(async()=>api.verifyPackage(truncated,'generic'),/outside image|truncated|header/);
  const other=await api.verifyPackage(new Uint8Array(fw),'generic');assert.equal(other.kind,'generic');
  assert.equal(api.isSupportedEtchIdentity('FM-1_2131'),true);
  assert.equal(api.isSupportedEtchIdentity('FM-1_2134'),true);
  assert.equal(api.isSupportedEtchIdentity('FM-1_2136'),true);
assert.equal(api.isSupportedEtchIdentity('FM-1_2137'),true);
assert.equal(api.isSupportedEtchIdentity('FM-1_2138'),true);
  assert.equal(api.isSupportedEtchIdentity('FM-1_9999'),false);
  assert.equal(Array.from(api.C.UBOOT).join(","),[0xF0,0x22,0x24,0x35,0x7D,0xF7].join(","));
  console.log('PASS V3.8 bridge tests: trusted generic FM1, CRC16, header, embedded firmware, UBOOT opcode');
})().catch(e=>{console.error(e);process.exit(1)});
