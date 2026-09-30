# Terminal Tetris

A small single-player Tetris game for POSIX terminals. Empty board cells render
as `.`, while active and locked tetromino cells render as `#`.

## Requirements

- A POSIX terminal on Linux or macOS
- `g++` with C++17 support
- `bash`, `script`, and `timeout` to run the automated terminal smoke test

## Build and Run

```bash
g++ -std=c++17 -Wall -Wextra -Werror -pedantic main.cpp tetris_game.cpp \
  terminal_ui.cpp -o app
./app
```

The board is 10 columns by 20 rows. Pieces fall one row every 500 ms. Ctrl-C
exits and restores the terminal.

## Controls

| Key | Action |
| --- | --- |
| `A` | Move left |
| `D` | Move right |
| `S` | Move down one row |
| `E` | Rotate clockwise |
| `Q` | Rotate counter-clockwise |
| `R` | Restart after game over |

The game uses a shuffled seven-bag for piece selection. Invalid moves and
rotations, including rotations at a wall, leave the piece unchanged. Completed
rows score 100, 300, 500, or 800 points for one, two, three, or four rows
cleared at once.

## Test

```bash
./test_runner.sh
```

The test suite runs deterministic engine unit tests and a pseudo-terminal
smoke test that verifies rendering, input handling, and Ctrl-C cleanup.

## Getting Started

This repository is compatible with [cpp-container](https://github.com/ChicoState/cpp-container). If not already built on your machine, clone and build it.

Run the container:

```bash
docker run -v "$(pwd)":/usr/src -it cpp-container
```

Run the application interactively in a shell:

```bash
docker run -v "$(pwd)":/usr/src -it cpp-container sh
```

## Structure

* `.agents` - AI agent configurations and skills (in `/skills` subdirectory) for this project
* `.` - The root directory contains the C++ code for the application as well as necessary scripts
* `specs` - Specification documentation
* `tests` - Test code
