#!/bin/bash

set -euo pipefail

test_binary="$(mktemp /tmp/tetris_game_tests.XXXXXX)"
trap 'rm -f "$test_binary"' EXIT

g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I. tetris_game.cpp \
  tests/tetris_game_tests.cpp -o "$test_binary"
"$test_binary"

bash tests/test_terminal_tetris.sh
