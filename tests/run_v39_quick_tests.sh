#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
sh tests/run_v38_quick_tests.sh
CC=${CC:-cc}
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware tests/native_v39_filter_exact.c -o build/native_v39_filter_exact
build/native_v39_filter_exact
