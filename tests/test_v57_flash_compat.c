/* Both FM1 1-MiB JEDEC revisions; do not assume case/logo is a chip ID. */
#include <assert.h>
#include <stdio.h>
#include "b1_flash_compat.h"
int main(void){
 assert(b1_flash_supported_id(0x856014u));
 assert(b1_flash_supported_id(0xB36014u));
 assert(!b1_flash_supported_id(0u));
 assert(!b1_flash_supported_id(0xFFFFFFu));
 assert(!b1_flash_supported_id(0x856013u));
 assert(!b1_flash_supported_id(0xB36015u));
 puts("PASS: Puya/UCUN supported; unknown JEDEC safely fails closed");
 return 0;
}
