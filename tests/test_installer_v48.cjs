// B1 V4.8 standalone HF2 WebInstaller validation. No MIDI or network access.
const fs=require('fs'),crypto=require('crypto'),assert=require('assert'),path=require('path'),vm=require('vm');
const R=path.resolve(__dirname,'..'),html=fs.readFileSync(path.join(R,'B1-WebInstaller.html'),'utf8');
const fw=fs.readFileSync(path.join(R,'build/SikkenWorks-B1-V4.8.fwsc'));
const data=html.match(/<script id="firmware-embedded" type="application\/octet-stream">([\s\S]*?)<\/script>/);
assert(data);assert(fw.equals(Buffer.from(data[1].replace(/\s/g,''),'base64')));
const sha=crypto.createHash('sha256').update(fw).digest('hex');assert(html.includes("HASH_ET: '"+sha+"'"));
assert(html.includes("ID_ET: 'FM-1_481026'"));
const script=[...html.matchAll(/<script([^>]*)>([\s\S]*?)<\/script>/g)].find(m=>m[2].includes('const VERIFIED_RUNNING_FIRMWARE'));
assert(script);new vm.Script(script[2]);
assert(script[2].includes('function isApprovedOtaLoader'));
assert(script[2].includes('ota-FM-1_015') && script[2].includes('9\\d\\d'));
assert(html.includes('B1 V4.8'));
console.log('PASS: V4.8 SLOOP HF2 installer, JS syntax, embedded firmware bytes, SHA256, new B1 product ID, Felucca loader support');
