#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
CC=${CC:-cc}
mkdir -p build
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware tests/native_v37_master_gain.c -o build/native_v37_master_gain
build/native_v37_master_gain
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware tests/native_lcd_damage_v37_test.c -o build/native_lcd_damage_v37_test
build/native_lcd_damage_v37_test
for name in native_v29_pcm_golden native_v33_saw_mode native_v33_saw_stress; do
 $CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/$name.c -o build/$name
 build/$name
done
for name in native_v30_realtime_exact native_v31_division_equivalence; do
 $CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware tests/$name.c -o build/$name
 build/$name
done
