// GPL-3.0-only. Check embedded V3.4 firmware and non-bypassable verification.
const fs=require('fs'),vm=require('vm'),crypto=require('crypto'),assert=require('assert');
const path=require('path');
const root=path.join(__dirname,'..');
const html=fs.readFileSync(path.join(root,'B1-WebInstaller.html'),'utf8');
const body=html.match(/<script>\/\* SPDX-License-Identifier: GPL-3\.0-only([\s\S]*?)<\/script>/);
assert(body,'Missing installer JS');
const context={module:{exports:{}},Uint8Array,Array,Math,Number,Date,Promise,console,TextEncoder,crypto:crypto.webcrypto,atob:global.atob,setTimeout};
vm.runInNewContext('/* SPDX-License-Identifier: GPL-3.0-only'+body[1],context);
const api=context.module.exports;
const firmware=fs.readFileSync(path.join(root,'build/SikkenWorks-B1-V3.4-BEETLE-OTA.fwsc'));
const reference=fs.readFileSync(path.join(root,'build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc'));
const embedded=html.match(/<script id="firmware-embedded" type="application\/octet-stream">\s*([^<]+)\s*<\/script>/);
assert(embedded,'Missing base64 firmware');
assert(Buffer.from(embedded[1].trim(),'base64').equals(firmware),'Installer has stale FWSC');
assert.strictEqual(api.C.ID_ET,'FM-1_2134');
assert.strictEqual(api.C.HASH_ET,crypto.createHash('sha256').update(firmware).digest('hex'));
assert.strictEqual(api.productOf(new Uint8Array(firmware)),'FM-1_2134');
assert(api.isSupportedEtchIdentity('FM-1_2132'));
assert(api.isSupportedEtchIdentity('FM-1_2133'));
assert(api.isSupportedEtchIdentity('FM-1_2134'));
assert(api.freedomSourceAllowed('FM-1_2134'));
assert.strictEqual(api.firmwareRoute('FM-1_2132'),'KNOWN FM1 -> B1 / RESTORE');
assert.strictEqual(api.firmwareRoute('FM-1_2133'),'KNOWN FM1 -> B1 / RESTORE');
assert.strictEqual(api.firmwareRoute('FM-1_2134'),'KNOWN FM1 -> B1 / RESTORE');
(async()=>{
 const pkg=await api.verifyPackage(new Uint8Array(firmware),'etch');
 assert.strictEqual(pkg.product,'FM-1_2134');
 const restore=await api.verifyPackage(new Uint8Array(reference),'freedom','beetle');
 assert.strictEqual(restore.product,'FM-1_2101');
 const corrupted=Buffer.from(firmware);corrupted[19000]^=0x40;
 let failed=false;try{await api.verifyPackage(new Uint8Array(corrupted),'etch')}catch{failed=true}
 assert(failed,'Tampered firmware cannot pass SHA');
 console.log('B1 V3.4: embedded firmware == standalone, source IDs, BEETLE restore, corrupt-file rejection PASS');
})().catch(e=>{console.error(e);process.exit(1)});
