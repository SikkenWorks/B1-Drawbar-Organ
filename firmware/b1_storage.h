/* SPDX-License-Identifier: GPL-3.0-only
 * B1 V1.7: 12 named, independently validated projects in two copy-on-write
 * verified candidate flash sectors. This is NOT BEETLE's 0x97000..0xE0000 project partition.
 */
#ifndef B1_STORAGE_H
#define B1_STORAGE_H
#include <stdint.h>
#include "b1_performance.h"
#define B1_PROJECT_NAME_MAX 16u
#define B1_STORAGE_SECTOR 4096u
/* Scan ONLY the unused tail between Felucca's autosave and global store.
 * Never erase unknown/foreign bytes; do not touch BEETLE, OTA or boot sectors. */
#define B1_STORAGE_SCAN_START 0x000E7000u
#define B1_STORAGE_SCAN_END   0x000F0000u
#ifndef B1_STORAGE_DYNAMIC
#define B1_STORAGE_DYNAMIC 0
#endif
#define B1_STORAGE_A 0x000E8000u
#define B1_STORAGE_B 0x000E9000u
#define B1_STORAGE_MAGIC 0x364A3142u /* B1J6 */
#define B1_STORAGE_COMMIT 0x5A19B116u
#define B1_STORAGE_SCHEMA 3u /* V4.9: append live autosave without changing 12 existing slots */
#define B1_STORAGE_DATA_OFFSET 256u
#define B1_STORAGE_UNMOUNTED 0u
#define B1_STORAGE_READY 1u
#define B1_STORAGE_EMPTY 2u
#define B1_STORAGE_LOCKED 3u
#define B1_STORAGE_ERROR 4u
struct b1_project_archive {
 uint32_t mask;
 char names[B1_PROJECT_SLOTS][B1_PROJECT_NAME_MAX+1u];
 struct b1_project projects[B1_PROJECT_SLOTS];
 /* V4.9: live startup scene, independent of the twelve NAMED slots.
  * Schema-2 data ends at offsetof(live); no existing slot is moved. */
 struct b1_project live;
 uint32_t live_valid;
};
struct b1_storage {
 struct b1_project_archive archive;
 uint32_t generation;
 int8_t active_bank;
 uint8_t state;
 uint32_t sector[2]; /* Actual verified pair (not fixed E8000/E9000). */
};
/* The callback implementation is kept independent of hardware for power-cut
 * simulation and strict bounds testing. All callbacks use absolute addresses. */
struct b1_storage_io {
 void *ctx;
 int (*read)(void*,uint32_t,uint8_t*,uint32_t);
 int (*erase)(void*,uint32_t);
 int (*program)(void*,uint32_t,const uint8_t*,uint32_t);
};
uint32_t b1_storage_crc(const void *src,uint32_t size);
int b1_storage_open(struct b1_storage *s,const struct b1_storage_io *io);
/* DANGEROUS AND EXPLICIT: erase only two sectors inside B1's fenced
 * journal scan region. Never call without a two-step on-device YES.
 * Refuses reclaim if any valid B1 journal is present. */
int b1_storage_reclaim_confirmed(struct b1_storage *s,const struct b1_storage_io *io,
                                  uint8_t two_step_confirmed);
int b1_storage_commit(struct b1_storage *s,const struct b1_storage_io *io,
                      const struct b1_project_archive *candidate);
uint8_t b1_storage_name_valid(const char name[B1_PROJECT_NAME_MAX+1u]);
void b1_storage_default_name(uint8_t slot,char name[B1_PROJECT_NAME_MAX+1u]);
#endif
