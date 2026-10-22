/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* M-UPGRADE entry: "step 1 lite".
 *
 * The updater talks SysEx (F0 pack7(00 59 cmd len24 body chk) F7):
 *   cmd 0x11  handshake -> we answer our package identity (FELUCCA_ID)
 *   F0 22 24 35 7F F7  upgrade -> ota_session(): pull parts of the package
 *     with cmd 0x30 read requests, check them, stage the package's update
 *     loader at 0xE0000 (outside Felucca's store), ask 0xE0000000 ("success"),
 *     then write the UPDATA_PARM record (flash 0xE4F00 + RAM 0x01C7FD88) and
 *     reset. The SPL runs the loader, which installs the package.
 * Nothing is committed before the host's "success"; any failure erases the
 * staging area and Felucca carries on.
 *
 * Portable core: the firmware (felucca.c) and the Mac test (ota_test.c)
 * supply these hooks:
 *   ota_wire_send(p, n)        one complete F0..F7 message to the host
 *   ota_frame_get(&p, &n)      next received SysEx (7-bit bytes between F0/F7), 0 if none
 *   ota_frame_done()           release it
 *   ota_now_ms(), ota_idle()   time; called while waiting (watchdog, USB)
 *   ota_erase(off) ota_prog(off, p, n) ota_fread(off, p, n)   flash, 0 = ok
 *   ota_show(step, code)       progress / result for the screen
 *   ota_commit(parm)           write RAM record + reset (never returns on hardware)
 */
#include "native_digest.h"
#define OTA_AREA 0x000E0000u                     /* staged loader, 5 x 4 KiB */
#define OTA_AREA_LEN 0x5000u
#define OTA_RES (OTA_AREA + OTA_AREA_LEN - 0x100u)  /* 0xE4F00 = 4K boundary - 256: SPL scan */
#define OTA_LDR_HCRC 0xEBAAu                     /* official update loader (usb_hid_ota.bin) */
#define OTA_LDR_DCRC 0x5881u
#define OTA_LDR_LEN 0x4DE1u
#define OTA_FRAME_MAX 640u                       /* 512-B response = 605 wire bytes */

static int ota_wire_send(const uint8_t *p, uint32_t n);
static int ota_frame_get(const uint8_t **p, uint32_t *n);
static void ota_frame_done(void);
static uint32_t ota_now_ms(void);
static void ota_idle(void);
static int ota_aborted(void);
static int ota_erase(uint32_t off);
static int ota_prog(uint32_t off, const void *p, uint32_t n);
static int ota_fread(uint32_t off, void *p, uint32_t n);
static void ota_show(uint32_t step, int32_t code);
static void ota_commit(const uint8_t *parm);

static uint32_t ota_rd16(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static uint32_t ota_rd24(const uint8_t *p) { return ota_rd16(p) | (uint32_t)p[2] << 16; }
static uint32_t ota_rd32(const uint8_t *p) { return ota_rd24(p) | (uint32_t)p[3] << 24; }
static void ota_wr16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void ota_wr32(uint8_t *p, uint32_t v) { ota_wr16(p, v); ota_wr16(p + 2, v >> 16); }

static uint32_t ota_crc16(const uint8_t *p, uint32_t n, uint32_t c)   /* poly 0x1021, init 0 */
{
    uint32_t i;
    while (n--) {
        c ^= (uint32_t)*p++ << 8;
        for (i = 0; i < 8u; i++)
            c = c & 0x8000u ? (c << 1) ^ 0x1021u : c << 1;
        c &= 0xFFFFu;
    }
    return c;
}

static void ota_jl_enc(uint8_t *p, uint32_t n)   /* UFW header cipher, key 0xFFFF */
{
    uint32_t k = 0xFFFFu;
    while (n--) {
        *p++ ^= (uint8_t)k;
        k = ((k << 1) ^ (k & 0x8000u ? 0x1021u : 0u)) & 0xFFFFu;
    }
}

/* 8-to-7, LSB first */
static uint32_t ota_pack7(const uint8_t *in, uint32_t n, uint8_t *out)
{
    uint32_t acc = 0, nb = 0, o = 0;
    while (n--) {
        acc |= (uint32_t)*in++ << nb;
        nb += 8u;
        while (nb >= 7u) {
            out[o++] = (uint8_t)(acc & 0x7Fu);
            acc >>= 7;
            nb -= 7u;
        }
    }
    if (nb)
        out[o++] = (uint8_t)(acc & 0x7Fu);
    return o;
}

static uint32_t ota_unpack7(const uint8_t *in, uint32_t n, uint8_t *out, uint32_t max)
{
    uint32_t acc = 0, nb = 0, o = 0;
    while (n--) {
        acc |= (uint32_t)(*in++ & 0x7Fu) << nb;
        nb += 7u;
        if (nb >= 8u) {
            if (o < max)
                out[o] = (uint8_t)acc;
            o++;
            acc >>= 8;
            nb -= 8u;
        }
    }
    return o;
}

static uint8_t ota_msg[8 + 32], ota_wire[2 + 48];
static uint8_t ota_dec[OTA_FRAME_MAX] __attribute__((section(".pool")));           /* decoded host message */

static int ota_send_msg(uint32_t cmd, const uint8_t *body, uint32_t n)   /* n <= 32 */
{
    uint32_t i, s = 0, w;
    ota_msg[0] = 0;
    ota_msg[1] = 0x59;
    ota_msg[2] = (uint8_t)cmd;
    ota_msg[3] = (uint8_t)n;
    ota_msg[4] = 0;
    ota_msg[5] = 0;
    for (i = 0; i < n; i++) {
        ota_msg[6 + i] = body[i];
        s += body[i];
    }
    ota_msg[6 + n] = (uint8_t)~s;
    ota_wire[0] = 0xF0;
    w = 1u + ota_pack7(ota_msg, 7u + n, ota_wire + 1);
    ota_wire[w++] = 0xF7;
    return ota_wire_send(ota_wire, w);
}

static void ota_reply_identity(void)
{
    static const char ID[] = FELUCCA_ID;         /* == the package marker string */
    uint8_t id[27];
    uint32_t i;
    for (i = 0; i < sizeof id; i++)
        id[i] = i < sizeof ID - 1u ? (uint8_t)ID[i] : 0;
    ota_send_msg(0x11, id, sizeof id);
}

/* decode the pending frame; returns its decoded length (0 = not ours) */
static uint32_t ota_take(void)
{
    const uint8_t *p;
    uint32_t n, d, s = 0, i;
    if (!ota_frame_get(&p, &n))
        return 0;
    d = ota_unpack7(p, n, ota_dec, sizeof ota_dec);
    ota_frame_done();
    if (d < 7u || d > sizeof ota_dec || ota_dec[0] != 0 || ota_dec[1] != 0x59 || ota_rd24(ota_dec + 3) + 7u != d)
        return 0;
    for (i = 6; i + 1u < d; i++)
        s += ota_dec[i];
    if ((uint8_t)~s != ota_dec[d - 1u])
        return 0;
    return d;
}

/* main loop, outside a session: answer handshakes */
static void ota_service(void)
{
    uint32_t d = ota_take();
#if defined(TRACK_FIRMWARE)
    if (d && beetle_transfer_handle(d)) return;
#endif
    if (d == 7u && ota_dec[2] == 0x11)
        ota_reply_identity();
}

/* one read request (3 tries x 5 s); data -> dst */
static uint32_t ota_deadline;                    /* 0 = none; else ota_now_ms() limit of the session */

static int ota_read(uint32_t addr, uint8_t *dst, uint32_t len)
{
    uint8_t q[8];
    uint32_t tries, t, d, i;
    if (ota_aborted() || (ota_deadline && (int32_t)(ota_now_ms() - ota_deadline) >= 0))
        return -1;                                  /* the whole session took too long: give up */
    q[0] = 0;
    ota_wr32(q + 1, addr);
    q[5] = (uint8_t)len;
    q[6] = (uint8_t)(len >> 8);
    q[7] = 0;
    for (tries = 0; tries < 3u; tries++) {
        if (ota_send_msg(0x30, q, 8))
            return -1;
        for (t = ota_now_ms(); ota_now_ms() - t < 5000u;) {
            ota_idle();
            if(ota_aborted() || (ota_deadline && (int32_t)(ota_now_ms()-ota_deadline)>=0))return -1;
            d = ota_take();
            if (!d)
                continue;
            if (d == 7u && ota_dec[2] == 0x11) {
                ota_reply_identity();
                continue;
            }
            if (d == len + 15u && ota_dec[2] == 0x30 && ota_rd24(ota_dec + 3) == len + 8u &&
                ota_rd32(ota_dec + 7) == addr && ota_rd24(ota_dec + 11) == len) {
                for (i = 0; i < len; i++)
                    dst[i] = ota_dec[14 + i];
                return 0;
            }
        }
    }
    return -1;
}

static int ota_memeq(const uint8_t *a, const uint8_t *b, uint32_t n)
{
    while (n--)
        if (*a++ != *b++)
            return 0;
    return 1;
}

/* JLFS head "LOADER.BIN" in front of the staged loader (vm_update_defrag), written last */
static int ota_stage_head(uint32_t len)
{
    static const char NAME[16] = "LOADER.BIN";
    uint8_t h[32];
    uint32_t i;
    for (i = 0; i < 32u; i++)
        h[i] = 0;
    ota_wr16(h + 2, 0xFFFFu);
    ota_wr32(h + 4, OTA_AREA);
    ota_wr32(h + 8, len + 32u);
    h[12] = 0x41u;
    h[13] = 0xFFu;
    ota_wr16(h + 14, 0xFFFFu);
    for (i = 0; i < 16u; i++)
        h[16 + i] = (uint8_t)NAME[i];
    ota_wr16(h, ota_crc16(h + 2, 30, 0));
    return ota_prog(OTA_AREA, h, 32);
}

/* UFW container validation, including distinct, bounded and non-overlapping
 * payload entries.  This prevents accepting arbitrary host-controlled offsets. */
static uint32_t ota_file_crc[2];
#define B1_OTA_APP_XIP 0x02000120u
#define B1_OTA_APP_FLASH 0x4000u
#define B1_OTA_APP_SLOT 0x8DFBCu
#define B1_OTA_FLASH_BYTES 0x93000u
#define B1_OTA_CHIP_KEY 0x980Fu

static int ota_ufw(uint8_t *hdr, uint32_t *fl_off, uint32_t *fl_len,
                   uint32_t *ota_off, uint32_t *ota_len)
{
    uint32_t nent, seen_flash=0, seen_ota=0, total, i, f_start=0, f_end=0, o_start=0, o_end=0;
    *fl_off = *fl_len = *ota_off = *ota_len = 0;
    if (ota_read(0, hdr, 512) || ota_read(512, hdr+512, 512)) return -1;
    ota_jl_enc(hdr, 0x40);
    nent = ota_rd16(hdr + 8);
    total = ota_rd32(hdr + 4);
    if (ota_crc16(hdr+2,0x3Eu,0) != ota_rd16(hdr) ||
        nent != 2u || total < 0x400u || total > 0x200000u ||
        !ota_memeq(hdr+16,(const uint8_t *)"AC791N",6) ||
        ota_crc16(hdr+0x40,nent*0x50u,0) != ota_rd16(hdr+2)) return -2;
    for (i=0; i<nent; ++i) {
        uint8_t *e=hdr+0x40u+i*0x50u;
        uint32_t typ, off, len;
        ota_jl_enc(e,0x50);
        typ=ota_rd16(e);off=ota_rd32(e+8);len=ota_rd32(e+12);
        if (off<0x400u || off>total || len>total-off ||
            len==0u || (off&0xffu) || ota_rd32(e+16)<len ||
            (ota_rd32(e+16)&31u)) return -3;
        if (typ==0u && !seen_flash++) {
            *fl_off=off; *fl_len=len; f_start=off; f_end=off+len;
            ota_file_crc[0]=ota_rd16(e+4);
        } else if (typ==100u && !seen_ota++) {
            *ota_off=off; *ota_len=len; o_start=off; o_end=off+len;
            ota_file_crc[1]=ota_rd16(e+4);
        } else return -3;
    }
    if (!seen_flash || !seen_ota || (f_start<o_end && o_start<f_end) ||
        *fl_len!=B1_OTA_FLASH_BYTES || *ota_len<64u ||
        *ota_len>OTA_AREA_LEN) return -3;
    return 0;
}

/* Each flash.bin app area XIP block has its own JieLi SFC XOR key.
 * The 91 tonewheel engine is independent of this code: only OTA CHECK uses it. */
static void b1_ota_xor32(uint8_t *p,uint32_t key)
{
    for(uint32_t i=0;i<32u;i++){
        p[i]^=(uint8_t)key;
        key=((key<<1) ^ ((key&0x8000u)?0x1021u:0u))&0xffffu;
    }
}

/* Validate the incoming app image independently of its protected boot bytes.
 * Requires the known AC791N/0x980F/0x02000120/0x4000 app-only layout.
 * In particular, a different boot image is NEVER staged or overwritten. */
static int b1_ota_app_layout(uint32_t fl_off,uint8_t *b)
{
    uint8_t top[0xA0], dir[64];
    static uint8_t isd[1024] __attribute__((section(".pool")));
    uint32_t isd_off,isd_size,app_size,app_slot,app_crc,isd_crc,head_crc;
    if(ota_read(fl_off,top,sizeof top))return -21;
    for(uint32_t i=0;i<sizeof top;i+=32u)b1_ota_xor32(top+i,0xffffu);
    head_crc=ota_crc16(top+2,30,0);
    if(ota_rd16(top)!=head_crc ||
       !ota_memeq(top+0x20+16,(const uint8_t *)"uboot.boot",10) ||
       !ota_memeq(top+0x40+16,(const uint8_t *)"isd_config.ini",14) ||
       !ota_memeq(top+0x60+16,(const uint8_t *)"app_dir_head",12) ||
       ota_rd32(top+0x60+4)!=B1_OTA_APP_FLASH) return -21;
    if(ota_crc16(top+0x40+2,30,0)!=ota_rd16(top+0x40) ||
       ota_crc16(top+0x60+2,30,0)!=ota_rd16(top+0x60))return -21;
    isd_off=ota_rd32(top+0x40+4);isd_size=ota_rd32(top+0x40+8);
    isd_crc=ota_rd16(top+0x40+2);
    if(isd_off<0xA0u || isd_size<32u || isd_size>sizeof isd ||
       isd_off>=B1_OTA_APP_FLASH || isd_size>B1_OTA_APP_FLASH-isd_off)return -21;
    for(uint32_t i=0;i<isd_size;){
        uint32_t n=isd_size-i;if(n>512u)n=512u;
        if(ota_read(fl_off+isd_off+i,isd+i,n))return -21;
        i+=n;
    }
    if(ota_crc16(isd,isd_size,0)!=isd_crc)return -21;
    /* JieLi's isd_config.ini encodes each key bit by XORing mirrored bytes. */
    uint32_t key=0,threshold=0;
    for(uint32_t i=0;i<16u;i++)threshold+=isd[i];
    threshold&=0xffu;
    if(threshold>=0xe0u)threshold=0xaau;
    else if(threshold<=0x10u)threshold=0x55u;
    for(uint32_t i=0;i<16u;i++){
        uint32_t v=isd[16+i]^isd[15-i];
        if(v<threshold)key|=1u<<i;
    }
    if(key!=B1_OTA_CHIP_KEY)return -22;
    if(ota_read(fl_off+B1_OTA_APP_FLASH,dir,sizeof dir))return -21;
    b1_ota_xor32(dir,B1_OTA_CHIP_KEY);
    b1_ota_xor32(dir+32u,B1_OTA_CHIP_KEY^(32u>>2));
    app_size=ota_rd32(dir+8);
    app_slot=ota_rd32(dir+32+8);
    app_crc=ota_rd16(dir+32+2);
    if(ota_rd16(dir)!=ota_crc16(dir+2,30,0) ||
       ota_rd16(dir+32)!=ota_crc16(dir+34,30,0) ||
       ota_rd32(dir+4)!=B1_OTA_APP_XIP ||
       !ota_memeq(dir+16,(const uint8_t *)"app_area_head",13) ||
       !ota_memeq(dir+32+16,(const uint8_t *)"app.bin",7) ||
       ota_rd32(dir+32+4)!=0x120u ||
       app_slot!=B1_OTA_APP_SLOT ||
       app_size<0x120u+B1_OTA_APP_SLOT ||
       app_size>B1_OTA_FLASH_BYTES-B1_OTA_APP_FLASH) return -23;
    /* Do not accept a self-consistent UFW wrapper containing a damaged XIP app. */
    uint32_t check=0;
    for(uint32_t off=0;off<B1_OTA_APP_SLOT;){
        uint32_t n=B1_OTA_APP_SLOT-off;
        if(n>512u)n=512u;
        if(ota_read(fl_off+B1_OTA_APP_FLASH+0x120u+off,b,n))return -24;
        for(uint32_t j=0;j<n;j+=32u){
            uint32_t part=n-j;if(part>32u)part=32u;
            if(part!=32u){
                uint8_t tail[32];
                for(uint32_t k=0;k<32u;k++)tail[k]=k<part?b[j+k]:0u;
                b1_ota_xor32(tail,B1_OTA_CHIP_KEY^((0x120u+off+j)>>2));
                for(uint32_t k=0;k<part;k++)b[j+k]=tail[k];
            }else b1_ota_xor32(b+j,B1_OTA_CHIP_KEY^((0x120u+off+j)>>2));
        }
        check=ota_crc16(b,n,check);off+=n;
        ota_idle();
    }
    return check==app_crc?0:-24;
}

/* Verify BOTH payload checksums from the external UFW directory before erase.
 * We never execute the external ota.bin; CRC only catches transmission errors. */
static int b1_ota_verify_payload(uint32_t off,uint32_t len,uint32_t expected,uint8_t *b)
{
    uint32_t crc=0;
    for(uint32_t i=0;i<len;){
        uint32_t n=len-i;if(n>512u)n=512u;
        if(ota_read(off+i,b,n))return -25;
        crc=ota_crc16(b,n,crc);i+=n;
        ota_idle();
    }
    return crc==expected?0:-25;
}

/* Cross-firmware family validation, B1 V4.6.
 * The incoming Felucca-family loader is parsed and CRC-checked, but is NOT
 * executed for the known original family. A different (but compatible)
 * version-tagged loader can be staged for cross-firmware transitions after
 * full structural and CRC checks. CRC16 only proves container consistency,
 * never publisher authenticity: the host (The Firmware Hub) MUST pin the
 * original package's SHA-256 before offering an update.
 * Vendor V15 FULL_FLASH continues through its separate strict pathway. */
#define ETCH_OTA_BODY_LEN 6898u
#define ETCH_OTA_OWNER (OTA_RES+0x80u)
/* Exact immutable loader bytes, compiled into the B1 XIP image.  This is
 * stricter than a checksum and avoids firmware-side SHA-256 discrepancies
 * that manifested as OTA -16 on real B1 hardware.  External firmware with
 * a different loader must use the independent UBOOT / vendor updater path. */
#include "b1_pinned_loader.h"
static const uint8_t etch_ota_owner[16]="BEETLE-OTA-STAGE";
static const uint8_t etch_ota_old_owner[16]="ETCH-OTA-STAGE-1";
static uint8_t ota_touched;
static int etch_ota_hash(uint32_t off,uint8_t *b,uint8_t from_host){
 for(uint32_t i=0;i<ETCH_OTA_BODY_LEN;){
  uint32_t n=ETCH_OTA_BODY_LEN-i;if(n>512u)n=512u;
  if(from_host?ota_read(off+i,b,n):ota_fread(off+i,b,n))return 0;
  if(!ota_memeq(b,b1_trusted_loader+i,n))return 0;
  i+=n;ota_idle();
 }
 return 1;
}
/* Validate the complete *incoming* versioned Felucca-family loader BEFORE
 * modifying any staging flash. V4.4 already verified the UFW entry CRC, but
 * was missing the loader's own data CRC and family tag check. A one-off SHA
 * whitelist is NOT used: legitimately recompiled family loaders differ.
 * Original known loader identity and stock V15 take their existing paths. */
#define B1_OTA_FELUCCA_MARK_LEN 16u
#define B1_OTA_LOADER_MAX (OTA_RES-OTA_AREA-32u)
static int b1_ota_compatible_loader(uint32_t off, uint32_t len,
                                    uint32_t declared_crc, uint8_t *buf)
{
    static const uint8_t tag[B1_OTA_FELUCCA_MARK_LEN] = "FELUCCA-LOADER-1";
    uint32_t crc = 0, i, n, j;
    int found = 0;
    /* Keep the last 256-byte OTA record sector reserved; the valid executable
     * entry is already checked against AC791N RAM at 0x01c0a800 upstream. */
    if (len < 128u || len > B1_OTA_LOADER_MAX)
        return -16;
    for (i = 0; i < len; i += n) {
        n = len - i;
        if (n > 512u) n = 512u;
        if (ota_read(off + 32u + i, buf, n)) return -16;
        crc = ota_crc16(buf, n, crc);
        /* The versioned loader marker lives in its prologue. Restrict it to
         * the first 512 bytes rather than trusting a trailing decoy string. */
        if (i == 0u)
            for (j = 0; j + B1_OTA_FELUCCA_MARK_LEN <= n; ++j)
                if (ota_memeq(buf + j, tag, B1_OTA_FELUCCA_MARK_LEN))
                    found = 1;
        ota_idle();
    }
    return found && crc == declared_crc ? 0 : -16;
}

/* No automatic claim of nonblank unknown bytes in this reserved region.
 * A complete pinned older loader or our ownership token permits a retry. */
static int etch_ota_available(uint8_t *b){
 uint8_t head[32];
 if(ota_fread(ETCH_OTA_OWNER,b,16))return 0;
 if(ota_memeq(b,etch_ota_owner,16)||ota_memeq(b,etch_ota_old_owner,16))return 1;
 if(ota_fread(OTA_AREA,head,32))return 0;
 if(ota_rd16(head)==ota_crc16(head+2,30,0)&&ota_rd32(head+4)==OTA_AREA&&ota_rd32(head+8)==ETCH_OTA_BODY_LEN+32&&ota_memeq(head+16,(const uint8_t*)"LOADER.BIN",10)&&etch_ota_hash(OTA_AREA+32,b,0))return 1;
 for(uint32_t i=0;i<OTA_AREA_LEN;i+=512){if(ota_fread(OTA_AREA+i,b,512))return 0;for(uint32_t j=0;j<512;j++)if(b[j]!=255)return 0;ota_idle();}
 return 1;
}
#ifndef ETCH_TEST_RESTORE_ONLY
#define ETCH_TEST_RESTORE_ONLY 0
#endif
/* Reference SHA-256 of the original M-VAVE V15 flash.bin (0x93000 B). */
static int etch_ota_stock(uint32_t off,uint8_t *b){
 static const uint8_t want[32]={0xc1,0x06,0x8a,0x40,0x3c,0x04,0xa6,0x37,0x8a,0x62,0x6a,0xfb,0x66,0xcc,0x21,0x7f,0x72,0xd1,0x3f,0x10,0xaa,0x20,0x0e,0x98,0x3b,0x7a,0x98,0x17,0xcc,0x1d,0xac,0x03};
 struct etch_sha256 h;uint8_t digest[32];etch_sha256_init(&h);
 for(uint32_t i=0;i<0x93000u;i+=512){if(ota_read(off+i,b,512))return 0;etch_sha256_add(&h,b,512);ota_idle();}
 etch_sha256_end(&h,digest);return ota_memeq(digest,want,32);
}
static int ota_stage(void)
{
    static uint8_t hdr[0x400] __attribute__((section(".pool")));
    static uint8_t b[512] __attribute__((section(".pool")));
    static uint8_t mine[512] __attribute__((section(".pool")));
    uint32_t i,n,ota_off,ota_len,fl_off,fl_len,len,c,outer_dcrc;
    int rc;
    uint8_t official_stock_restore=0, use_family_loader=0;
    ota_show(1,0);
    if((rc=ota_ufw(hdr,&fl_off,&fl_len,&ota_off,&ota_len))!=0)return rc;
    if((rc=b1_ota_verify_payload(fl_off,fl_len,ota_file_crc[0],b))!=0)return rc;
    if((rc=b1_ota_verify_payload(ota_off,ota_len,ota_file_crc[1],b))!=0)return rc;
    /* Validate both levels of the foreign loader wrapper but do NOT run it. */
    if(ota_read(ota_off,b,64u))return -6;
    len=ota_rd32(b+8);
    outer_dcrc=ota_rd16(b+2);
    if(ota_crc16(b+2,30,0)!=ota_rd16(b) ||
       ota_crc16(b+34,30,0)!=ota_rd16(b+32) ||
       !ota_memeq(b+16,(const uint8_t *)"usb_hid_ota.bin",15) ||
       !ota_memeq(b+48,(const uint8_t *)"usb_hid_ota.bin",15) ||
       ota_rd32(b+4)!=0x20u || ota_len!=len+0x20u ||
       ota_rd32(b+32+8)!=0x01C0A800u) return -7;
    /* Only exact, audited original V15 flash may use the full-flash writer.
     * Unrecognized full-flash images NEVER get this code path. */
    if(len==OTA_LDR_LEN && ota_rd16(b)==OTA_LDR_HCRC &&
       ota_rd16(b+2)==OTA_LDR_DCRC && etch_ota_stock(fl_off,b)) {
        official_stock_restore=1u;
    } else {
        /* APP_ONLY requires valid chip layout and fully decoded app CRC.
         * A foreign boot header does not matter because this loader does
         * not write 0x0000..0x3FFF. */
        if((rc=b1_ota_app_layout(fl_off,b))!=0)return rc;
        if((rc=b1_ota_compatible_loader(ota_off,len,outer_dcrc,b))!=0)return rc;
        /* Keep the original, proven loader for its identical input bytes.
         * A differently compiled, structurally compatible Felucca loader
         * is staged from the verified incoming package, as in BEETLE's
         * cross-firmware path. The Hub is responsible for SHA-256 pinning. */
        use_family_loader=(len!=ETCH_OTA_BODY_LEN)||!etch_ota_hash(ota_off+32u,b,1);
        if(!use_family_loader)len=ETCH_OTA_BODY_LEN;
    }
    ota_show(2,0);
    if(!etch_ota_available(b))return -15;
    ota_show(3,0);
    ota_touched=1;
    for(i=0;i<OTA_AREA_LEN;i+=0x1000u)if(ota_erase(OTA_AREA+i))return -8;
    if(ota_prog(ETCH_OTA_OWNER,etch_ota_owner,16)||
       ota_fread(ETCH_OTA_OWNER,mine,16)||
       !ota_memeq(mine,etch_ota_owner,16))return -10;
    /* Stage original trusted B1 loader for known APP_ONLY packages, the
     * fully validated incoming Felucca-family loader for transitions, or
     * the exact pinned original V15 loader on the FULL_FLASH restore path.
     * Never use an unknown loader family or a failed CRC. */
    c=0;
    for(i=0;i<len;i+=n){
        n=len-i;if(n>512u)n=512u;
        if(official_stock_restore || use_family_loader){
            if(ota_read(ota_off+32u+i,b,n))return -9;
        }else{
            for(uint32_t j=0;j<n;j++)b[j]=b1_trusted_loader[i+j];
        }
        c=ota_crc16(b,n,c);
        if(ota_prog(OTA_AREA+32u+i,b,n))return -10;
    }
    {
        uint32_t staged_crc = 0;
        for(i=0;i<len;i+=n){
            n=len-i;if(n>512u)n=512u;
            if(ota_fread(OTA_AREA+32u+i,b,n))return -11;
            staged_crc=ota_crc16(b,n,staged_crc);
            if(!official_stock_restore && !use_family_loader && !ota_memeq(b,b1_trusted_loader+i,n))return -18;
        }
        /* Not only the transmitted bytes, but the actual staged flash must
         * match their CRC before writing the commit record. */
        if(staged_crc!=c || ((official_stock_restore || use_family_loader) && c!=outer_dcrc))return -11;
    }
    if(ota_stage_head(len))return -12;
    ota_show(4,0);
    for(i=0;i<4u;i++)if(!ota_read(0xE0000000u,b,8) &&
       ota_memeq(b,(const uint8_t *)"success",8))return 0;
    return -13;
}

static int ota_unstage(void){
    if(!ota_touched)return 0;
    /* Ownership and commit sector last; a failed cleanup keeps retry identity. */
    for(uint32_t i=0;i<OTA_AREA_LEN;i+=0x1000u)if(ota_erase(OTA_AREA+i))return -17;
    ota_touched=0;return 0;
}

/* returns only when nothing was committed (refused, failed, or dry run) */
static int ota_session(void)
{
    static uint8_t parm[112];
    uint8_t back[112];
    uint32_t i;
    int rc;
    ota_touched=0;
    ota_deadline = ota_now_ms() + 300000u;          /* step 1 normally takes ~5 s; a stalled host gives up after 2 min */
    rc = ota_stage();
    if (rc) {
        if(ota_unstage())rc=-17;
        ota_show(9, rc);
        return rc;
    }
    /* 5./7. UPDATA_PARM (update_mode_api_v2 layout), flash copy first */
    for (i = 0; i < sizeof parm; i++)
        parm[i] = 0;
    ota_wr16(parm + 2, 0x5A0Du);
    ota_wr16(parm + 4, 0x5A01u);
    ota_wr16(parm + 6, 0x5441u);
    for (i = 0; i < 12u; i++)
        parm[8 + i] = (uint8_t)"ota-FM-1_015"[i];    /* the loader's own USB identity */
    ota_wr32(parm + 72, OTA_AREA);
    ota_wr16(parm, ota_crc16(parm + 2, 78, 0));
#if FELUCCA_OTA_DRYRUN
    ota_unstage();
    ota_show(9, 1);                                 /* 1 = dry run complete */
    return 1;
#endif
#ifndef FELUCCA_OTA_RAMONLY
#define FELUCCA_OTA_RAMONLY 0                       /* 1: RAM record only, no power-loss resume (a power
                                                     * cycle leaves a broken loader behind) */
#endif
    if (!FELUCCA_OTA_RAMONLY && (ota_prog(OTA_RES, parm, sizeof parm) || ota_fread(OTA_RES, back, sizeof back) ||
        !ota_memeq(back, parm, sizeof parm))) {
        rc=ota_unstage()?-17:-14;
        ota_show(9, rc);
        return rc;
    }
    ota_show(5, 0);
    ota_commit(parm);
    return 0;
}

/* Target does not erase staging on boot. A later explicit session accepts
 * a complete pinned loader or a verified ETCH owner token as a retry. */
