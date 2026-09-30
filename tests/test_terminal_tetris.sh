#!/bin/bash

set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
app_binary="$(mktemp /tmp/tetris_app.XXXXXX)"
session_log="$(mktemp /tmp/tetris_session.XXXXXX)"
trap 'rm -f "$app_binary" "$session_log"' EXIT

g++ -std=c++17 -Wall -Wextra -Werror -pedantic \
  "$project_root/main.cpp" "$project_root/tetris_game.cpp" \
  "$project_root/terminal_ui.cpp" -o "$app_binary"

{
  sleep 0.2
  printf 'adseq'
  sleep 0.2
  printf '\003'
} | timeout -k 1s 5s script -qec "$app_binary" "$session_log" > /dev/null

grep -Eq '[.#]{10}' "$session_log"
grep -q 'Score:' "$session_log"
grep -Fq $'\033[?25h' "$session_log"
