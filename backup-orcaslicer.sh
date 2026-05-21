#!/bin/bash
# backup-orcaslicer.sh
# Syncs OrcaSlicer user profiles to ~/orcaslicer_profiles and pushes to GitHub.
# Run manually or via launchd (com.ken.orcaslicer-backup.plist).

set -euo pipefail

ORCA_SOURCE="$HOME/Library/Application Support/OrcaSlicer/user"
REPO="$HOME/orcaslicer_profiles"
LOG="$REPO/backup.log"

# Verify repo exists
if [ ! -d "$REPO/.git" ]; then
  echo "ERROR: $REPO is not a git repo. Clone it first:" >&2
  echo "  git clone git@github.com:ken-alt/orcaslicer_profiles.git ~/orcaslicer_profiles" >&2
  exit 1
fi

# Verify source exists
if [ ! -d "$ORCA_SOURCE" ]; then
  echo "ERROR: OrcaSlicer config not found at $ORCA_SOURCE" >&2
  exit 1
fi

# Sync profiles — exclude cache, logs, thumbnails, and temp files
rsync -a --delete \
  --exclude='cache/' \
  --exclude='log/' \
  --exclude='logs/' \
  --exclude='thumbnails/' \
  --exclude='thumbnail/' \
  --exclude='*.tmp' \
  --exclude='*.lock' \
  "$ORCA_SOURCE/" "$REPO/user/"

# Commit and push only if there are changes
cd "$REPO"
git add -A

if git diff --staged --quiet; then
  echo "$(date '+%Y-%m-%d %H:%M:%S') — No changes, nothing to commit." >> "$LOG"
  exit 0
fi

TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
git commit -m "Auto-backup: $TIMESTAMP"
git push

echo "$TIMESTAMP — Backup complete." >> "$LOG"
echo "OrcaSlicer backup pushed to GitHub."
