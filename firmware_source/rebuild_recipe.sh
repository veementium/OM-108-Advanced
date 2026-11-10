#!/bin/bash
# Regenerates ../assets/om108_mod.patch (v30) from the ORIGINAL recipe + the build in firmware_source/mod2_v30.bin (made by build.sh), and registers every
# earlier build listed in legacy_builds/builds.txt as recognised. Usage:  bash rebuild_recipe.sh /path/to/om108_backup_v30.bin
# AFTER a build is delivered, add its line to legacy_builds/builds.txt (name=file) and copy its mod2_v30.bin there, so the next recipe still knows it.
set -e
cd "$(dirname "$0")"
cp legacy_builds/om108_mod.patch.orig ../assets/om108_mod.patch
args=()
while IFS= read -r line; do [ -z "$line" ] && continue; args+=("${line%%=*}=$(pwd)/legacy_builds/${line#*=}"); done < legacy_builds/builds.txt
python3 make_patch.py "$1" "${args[@]}"
