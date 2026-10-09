#!/usr/bin/env bash

set -u
cd "$HOME/surveying_robot" || exit 1

mkdir -p "$HOME/.local/state/surveying_robot"
LOG="$HOME/.local/state/surveying_robot/auto_sync.log"

echo "Auto-sync started: $(date)" >> "$LOG"

inotifywait -m -r \
  -e close_write,create,moved_to,delete \
  --format '%w%f' \
  testing docs/reports 2>> "$LOG" |
while IFS= read -r changed_file; do
    sleep 3

    git add -- testing docs/reports

    if ! git diff --cached --quiet; then
        git commit -m "Update robot testing data: $(date '+%Y-%m-%d %H:%M:%S')" >> "$LOG" 2>&1

        if [ "$?" -eq 0 ]; then
            git push origin main >> "$LOG" 2>&1
        fi
    fi
done
