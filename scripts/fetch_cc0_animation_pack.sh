#!/usr/bin/env bash
set -euo pipefail

# KinemIQ / Quaternius 35-animation pack, CC0-1.0.
# Pinned release asset; verify the archive before packaging it.
readonly url="https://github.com/Kinemiq/free-game-animations/releases/download/v1.0/KinemIQ_Free_Animations_CC0_GLB.zip"
readonly expected_sha256="64ca5ab4a92c9da079c5e88ca509391b0a58a24ccf2a1307b16e2f0fc6b23ffc"
readonly output_dir="app/src/main/assets/Characters/AnimationPack"
readonly archive="${RUNNER_TEMP:-/tmp}/arut-kinemiq-animations.zip"

mkdir -p "$output_dir"
curl --fail --location --retry 3 --silent --show-error "$url" --output "$archive"
echo "$expected_sha256  $archive" | sha256sum --check -
unzip -oq "$archive" -d "$output_dir"
find "$output_dir" -type f -iname '*.glb' -printf '%P %s bytes\\n' | sort
count="$(find "$output_dir" -type f -iname '*.glb' | wc -l)"
if [[ "$count" -eq 0 ]]; then
  echo "Animation archive contained no GLB files" >&2
  exit 1
fi
echo "ANIMATION_PACK_FILES=$count"
