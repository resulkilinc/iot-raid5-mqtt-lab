#!/usr/bin/env bash
# Copy CSV off the RAID mount, checksum, optional restore demo.
set -euo pipefail
SRC="${1:-/mnt/raid5/gomulu-iot/data/sensor_samples.csv}"
DEST_DIR="${2:-/var/tmp/iot-lab-backup}"
mkdir -p "$DEST_DIR"
stamp="$(date -u +%Y%m%d-%H%M%S)"
base="$(basename "$SRC")"
cp -a "$SRC" "$DEST_DIR/${base%.csv}_$stamp.csv"
(
  cd "$DEST_DIR"
  sha256sum "${base%.csv}_$stamp.csv" > "checksums_$stamp.txt"
  sha256sum -c "checksums_$stamp.txt"
)
echo "Backup OK -> $DEST_DIR"
