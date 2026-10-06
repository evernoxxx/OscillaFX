#!/bin/bash
# One-command build + DSP regression test. Pass -DOSCILLA_SANITIZE=ON for ASan/UBSan.
set -e
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug "$@" >/dev/null
cmake --build build -j8 2>&1 | grep -E "error|Error" || true
MallocScribble=1 MallocPreScribble=1 ./build/dsp_offline
# ../presets/*.fac must be exactly what tools/make_preset.cpp writes.
generated=$(mktemp -d); trap 'rm -rf "$generated"' EXIT
./build/make_preset "$generated" >/dev/null
diff -r "$generated" ../presets && echo "presets match make_preset"
MallocScribble=1 MallocPreScribble=1 ./build/preset_tuning
