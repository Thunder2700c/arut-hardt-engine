#!/usr/bin/env bash
set -euo pipefail

# The source GLB is distributed under CC0-1.0 by KinemIQ.
# Pin the release and verify its published SHA-256 before packaging it.
readonly url="https://github.com/Kinemiq/free-game-animations/releases/download/v1.0/KinemIQ_Free_Animations_CC0_GLB.zip"
readonly expected_sha256="64ca5ab4a92c9da079c5e88ca509391b0a58a24ccf2a1307b16e2f0fc6b23ffc"
readonly archive="${RUNNER_TEMP:-/tmp}/kinemiq-cc0-glb.zip"
readonly destination="app/src/main/assets/Characters"

mkdir -p "$destination"
curl --fail --location --retry 3 --retry-all-errors "$url" --output "$archive"
echo "$expected_sha256  $archive" | sha256sum --check --status

entry="$(unzip -Z1 "$archive" | grep -Ei '(^|/)[^/]*mannequin[^/]*\.glb$' | head -n 1 || true)"
if [[ -z "$entry" ]]; then
  echo "Pinned CC0 pack contains no GLB with 'mannequin' in its filename." >&2
  unzip -Z1 "$archive" >&2
  exit 1
fi

unzip -p "$archive" "$entry" > "$destination/hero.glb"
python3 - "$destination/hero.glb" <<'PY'
import pathlib, struct, sys
path = pathlib.Path(sys.argv[1])
data = path.read_bytes()
if len(data) < 20 or data[:4] != b"glTF":
    raise SystemExit("Default character is not a valid GLB header")
version, length = struct.unpack_from("<II", data, 4)
if version != 2 or length != len(data):
    raise SystemExit("Default character GLB header/version/length mismatch")
print(f"Default CC0 mannequin packaged: {path} ({len(data)} bytes)")
PY
