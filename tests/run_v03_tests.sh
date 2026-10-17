#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
CC=${CC:-cc}
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_b1_test.c -o build/b1-native-regression
build/b1-native-regression
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_v03_test.c -o build/b1-native-fx
build/b1-native-fx
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c tests/native_tonewheel_test.c -o build/b1-native-tonewheel
build/b1-native-tonewheel
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_ui.c firmware/b1_engine.c firmware/b1_performance.c tests/native_drawbar_direction_test.c -o build/b1-native-drawbar-direction
build/b1-native-drawbar-direction
python3 tools/actual_ui_screenshots.py >/dev/null
printf 'B1 V2.3: host tests and 240x240 screen frame generation PASS\n'

$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_chord_attack_test.c -o build/b1-native-chord-attack
build/b1-native-chord-attack

$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_amp_modes_test.c -o build/b1-native-amp-modes
build/b1-native-amp-modes

$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_ui.c firmware/b1_engine.c firmware/b1_performance.c tests/native_scale_led_test.c -o build/b1-native-scale-led
build/b1-native-scale-led

python3 tests/test_audio_v12.py

$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_startup.c firmware/b1_ui.c firmware/b1_engine.c firmware/b1_performance.c tests/native_startup_test.c -o build/b1-native-startup
build/b1-native-startup
python3 tests/test_ui_captures.py

$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_guide_mapping_v14_test.c -o build/b1-native-guide-v14
build/b1-native-guide-v14

$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_stability_v15_test.c -o build/b1-native-stability-v15
build/b1-native-stability-v15

# Durable project storage must survive resets and interrupted NOR operations.
$CC -std=c11 -O1 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c firmware/b1_storage.c tests/native_storage_v16_test.c -o build/b1-native-storage-v16
build/b1-native-storage-v16

# V1.7 hardware hotfix: simulated input, chord assignment, safe audio and dynamic NOR.
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_v17_hotfix_test.c -o build/b1-native-v17-hotfix
build/b1-native-v17-hotfix
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -DB1_STORAGE_DYNAMIC=1 -I firmware firmware/b1_engine.c firmware/b1_performance.c firmware/b1_storage.c tests/native_storage_v17_dynamic_test.c -o build/b1-native-v17-storage
build/b1-native-v17-storage

# B1 V1.9 fresh project must sound from ALL keys, release cleanly, avoid full-scale saturation.
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_v19_hardware_regression.c -o build/b1-v19-hardware-regression
build/b1-v19-hardware-regression

# V2.0: signed key-click gain, MIDI note-off silence, full/safe filter cleanups.
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_keyclick_silence_v20.c -o build/b1-v20-click-silence
build/b1-v20-click-silence

$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_v22_pro_audio_test.c -o build/b1-v22-pro-audio
build/b1-v22-pro-audio

# V2.3: project V6, 4 reverb algorithms, delay and interpolated scanner.
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c firmware/b1_performance.c tests/native_v23_fx_project.c -o build/b1-v23-fx-project
build/b1-v23-fx-project
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -DB1_STORAGE_DYNAMIC=1 -I firmware firmware/b1_engine.c firmware/b1_performance.c firmware/b1_storage.c tests/native_v23_v5_migration.c -o build/b1-v23-migration
build/b1-v23-migration
$CC -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I firmware firmware/b1_engine.c tests/native_v23_max_fx_stress.c -o build/b1-v23-max-fx
build/b1-v23-max-fx
