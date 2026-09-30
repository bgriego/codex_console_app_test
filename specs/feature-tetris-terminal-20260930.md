# Spec: Terminal Tetris

**Status:** Approved by the user on 2026-09-30 and implemented locally. The
unit suite, pseudo-terminal smoke test, and warnings-as-errors build pass.

## Feature Description

Create a playable, single-player Tetris program for the terminal. The playfield renders empty cells as `.` and settled or falling tetromino cells as `#`. A falling piece advances automatically on a timed interval while the player moves and rotates it with the requested keyboard controls. The game uses the standard seven tetrominoes on a 10-column by 20-row board, clears completed rows, tracks score, ends cleanly when a new piece cannot spawn, and permits restarting after game over.

## Objective

Deliver a small, reliable Tetris game for a person running the repository locally in a POSIX terminal. Success is a responsive interactive session that renders the specified grid, applies every requested control correctly, advances via 500 ms gravity, follows the confirmed seven-bag/no-wall-kick rules, and exits without damaging the terminal session.

## Tech Stack

- Language: C++17, compiled with the repository's GNU C++ toolchain.
- Runtime: POSIX-compatible terminal on Linux or macOS.
- Standard library: `std::array`, `std::chrono`, `std::random`, and `std::thread` where needed.
- Terminal facilities: POSIX `termios` and `fcntl`, plus ANSI escape sequences.
- Test tooling: a repository-local C++ assertion harness and a Bash pseudo-terminal smoke test using the installed `script` and `timeout` utilities.
- Dependencies: none beyond the C++ standard library and POSIX system interfaces.

## Commands

These commands describe the completed feature; the implementation updates `test_runner.sh` to provide the test command.

```bash
# Build the interactive game
g++ -std=c++17 -Wall -Wextra -Werror -pedantic main.cpp tetris_game.cpp terminal_ui.cpp -o app

# Run all unit and pseudo-terminal end-to-end tests
./test_runner.sh

# Run the pseudo-terminal end-to-end test alone
tests/test_terminal_tetris.sh

# Play interactively in a real POSIX terminal
./app
```

## Project Structure

```text
main.cpp                       interactive application loop
tetris_game.hpp/.cpp           terminal-independent Tetris rules and game state
terminal_ui.hpp/.cpp            POSIX input handling and ANSI renderer
tests/tetris_game_tests.cpp     engine unit tests
tests/test_terminal_tetris.sh   pseudo-terminal end-to-end smoke test
test_runner.sh                  test-suite entry point
README.md                       user-facing build, run, and controls documentation
specs/                          approved specifications and plans
```

## Code Style

- Use C++17 standard-library types and RAII; do not add a third-party library for this feature.
- Use `PascalCase` for types, `camelCase` for functions and variables, and `constexpr` for fixed board and timing values.
- Keep game rules in `tetris_game.cpp`; `main.cpp` coordinates the loop only, while `terminal_ui.cpp` owns terminal state and rendering.
- Return a boolean from movement/rotation attempts to make acceptance explicit and directly testable.

```cpp
bool Game::moveLeft() {
  const Position candidate{active_.origin.row, active_.origin.column - 1};
  if (!canPlace(active_.shape, active_.rotation, candidate)) {
    return false;
  }
  active_.origin = candidate;
  return true;
}
```

## Boundaries

- Always: compile with `-Wall -Wextra -Werror -pedantic`; run the complete test suite; preserve terminal settings and cursor visibility on every exit path; keep unit tests deterministic.
- Ask first: adding third-party dependencies, changing the build/CI toolchain, expanding platform support beyond POSIX, or adding gameplay features outside this specification.
- Never: reinterpret `Q` as quit; add wall kicks, hold/preview, levels, hard drop, pause, colors, or persistent scoring; commit generated binaries or terminal-session logs; remove tests to make a build pass.

## User Story

As a terminal user,
I want to play Tetris with responsive keyboard controls and automatic falling pieces,
So that I can play a familiar game directly from the command line.

## Problem Statement

The repository currently builds a no-op C++ program. It has no game state, terminal input handling, timed update loop, renderer, or automated tests. An interactive implementation placed entirely in `main.cpp` would be difficult to verify and would couple the rules to platform-specific terminal behavior.

## Solution Statement

Implement a deterministic, terminal-independent Tetris game engine and cover its rules with unit tests. Wrap that engine in a small POSIX terminal adapter that enables raw, non-blocking input for the interactive session and restores terminal settings on exit. The application loop polls input, advances gravity using `std::chrono::steady_clock`, renders an ANSI-cleared frame, and sleeps briefly to avoid busy-waiting. No third-party library is required.

### Gameplay decisions

- Board: 10 columns by 20 visible rows.
- Pieces: I, J, L, O, S, T, and Z tetrominoes; spawn order uses a shuffled seven-bag, so every consecutive bag contains each tetromino exactly once.
- Display: a cell occupied by either the active or locked piece is `#`; every other board cell is `.`.
- Controls while playing: `A`/`a` moves left, `D`/`d` moves right, `S`/`s` performs a soft drop, `E`/`e` rotates clockwise, and `Q`/`q` rotates counter-clockwise.
- Gravity: one attempted downward move every 500 ms; each soft-drop keypress performs one immediate attempted downward move. The loop must use elapsed time rather than assumed frame count.
- Collision: a move or rotation is accepted only if all resulting cells are in board bounds and do not overlap locked cells. A rotation that collides, including at a wall, is ignored; this initial implementation does not use wall kicks.
- Locking and clearing: when a downward move is blocked, lock the active piece, remove every full row, shift upper rows down, update score, then spawn the next piece.
- Game over: if the next piece cannot spawn, show a game-over frame. `R`/`r` starts a new game; Ctrl-C exits the program. `Q` remains reserved for counter-clockwise rotation as requested.

## Relevant Files

Use these files to implement the feature:

- `main.cpp` — replace the no-op entry point with initialization and the interactive game loop only.
- `test_runner.sh` — update the current compile-and-run script so it runs the non-interactive unit suite and terminal end-to-end test without leaving a production binary in the repository.
- `README.md` — document build/run instructions, supported terminal assumptions, controls, and the game-over restart behavior.

### New Files

- `tetris_game.hpp` — declares board dimensions, piece representation, game state, and testable game operations.
- `tetris_game.cpp` — implements spawning, collision checks, movement, rotations, locking, row clearing, scoring, and reset behavior.
- `terminal_ui.hpp` — declares the raw-terminal session guard, input polling, and frame-rendering interface.
- `terminal_ui.cpp` — implements POSIX `termios` setup/restoration, non-blocking character input, and ANSI terminal rendering.
- `tests/tetris_game_tests.cpp` — self-contained C++ unit tests for engine rules; uses a small assertion harness rather than a new test framework.
- `tests/test_terminal_tetris.sh` — pseudo-terminal end-to-end smoke test that starts the compiled application, sends representative controls, verifies a rendered board and clean exit, and captures diagnostic output on failure.

## Implementation Plan

### Phase 1: Foundation

Define a small engine API with no terminal dependencies. Represent locked board cells separately from the active tetromino, and centralize collision validation so movement, rotation, gravity, and spawning share identical rules. Add the unit-test harness and wire it into the existing test script before building interactive behavior.

### Phase 2: Core Implementation

Implement all standard pieces and game-state transitions: movement, clockwise/counter-clockwise rotation, soft drop, gravity step, locking, row removal, score update, next-piece spawning, game-over detection, and reset. Add a POSIX terminal adapter whose destructor restores terminal state even when the game loop exits early.

### Phase 3: Integration

Build the timed `main.cpp` loop around the tested engine. Render the board and status every frame, poll controls without blocking gravity, support restarting only from game over, and document the program. Add a pseudo-terminal smoke test to exercise interactive startup, rendering, input, and cleanup together.

## Step by Step Tasks

IMPORTANT: Execute every step in order, top to bottom.

### 1. Define the engine contract and test harness

- Create `tetris_game.hpp` with a `Game` API that exposes reset, left/right movement, soft drop, clockwise/counter-clockwise rotation, gravity tick, game-over state, score, and a read-only board snapshot suitable for rendering and tests.
- Implement an injectable or seedable seven-bag generator so tests can verify exact piece order without depending on chance.
- Create `tests/tetris_game_tests.cpp` with a simple assertion-based executable and an initial failing test for a known valid spawn.
- Update `test_runner.sh` to compile the engine test executable with `g++ -std=c++17 -Wall -Wextra -Werror -pedantic` and run it.

### 2. Implement board and tetromino rules

- Add all seven tetromino definitions with rotation states, spawn location, and board-boundary/locked-cell collision checks in `tetris_game.cpp`.
- Implement left, right, soft-drop, gravity tick, and both rotations through a single candidate-placement validation path.
- Keep a blocked rotation as a no-op, including a rotation at either wall; do not implement kick tables in this scope.
- Extend unit tests for every piece spawn, accepted and rejected horizontal movement, floor collision, both rotation directions, and blocked rotations.

### 3. Implement locking, clearing, scoring, and game lifecycle

- Lock a piece only after a failed downward move, clear all completed rows in one lock event, compact the board, and calculate the documented score increment.
- Award 100/300/500/800 points for clearing 1/2/3/4 rows in one lock respectively; document this fixed, no-level scoring table in the implementation and README.
- Spawn a replacement piece after locking; set game over when its spawn cells are unavailable.
- Implement reset so it clears board, score, active piece, and game-over state.
- Add unit tests for single and multiple simultaneous line clears, compaction, score changes, spawn-blocked game over, and reset.

### 4. Create the terminal adapter

- Create `terminal_ui.hpp` and `terminal_ui.cpp` using POSIX `termios`, `fcntl`, and ANSI escape sequences; state the POSIX/Linux/macOS terminal requirement in the README.
- Implement an RAII terminal-session guard that saves settings, enables character-at-a-time non-blocking input, hides the cursor for the session, and restores settings plus cursor visibility on destruction.
- Implement a renderer that clears/repositions the terminal cursor and prints exactly 20 rows of 10 `.`/`#` cells plus a score/control status line.
- Implement case-insensitive polling for the requested five controls; leave Ctrl-C under normal signal handling for exit.

### 5. Integrate the game loop

- Replace `main.cpp` with an application loop that initializes the terminal session and `Game`, polls available input, performs elapsed-time gravity ticks using `std::chrono::steady_clock`, renders frames, and briefly sleeps between iterations.
- Route `A`, `D`, `S`, `E`, and `Q` to their exact engine operations; do not repurpose `Q` for quit.
- On game over, stop applying movement controls, display restart instructions, and accept `R`/`r` to reset the game.
- Ensure every exit path restores the terminal through the session guard.

### 6. Add terminal end-to-end coverage and documentation

- Create `tests/test_terminal_tetris.sh`, using an available pseudo-terminal utility such as `script`, to compile/start the game in a PTY, send a short sequence containing `a`, `d`, `s`, `e`, `q`, and Ctrl-C, and assert the captured output includes a 10-cell grid row and score text.
- Make the test fail with the captured transcript if the game hangs, fails to render, or exits unsuccessfully; use a timeout so CI cannot stall.
- Update `test_runner.sh` to build the application separately from unit tests and run both test layers.
- Update `README.md` with exact build/test/run commands, controls, board symbols, requirements, and restart/exit instructions.

### 7. Run the validation suite

- Execute every command in the Validation Commands section and fix all failures before review.
- Manually run the game in an interactive terminal and verify gravity, each requested control, row clearing, game-over display, terminal restoration, and restart.

## Testing Strategy

### Unit Tests

- Verify every tetromino spawns inside the board and has four occupied cells.
- Verify movement/soft drop succeeds in free space and fails without mutating state on wall, floor, or locked-cell collision.
- Verify `E` and `Q` produce clockwise and counter-clockwise rotation respectively, while rejected rotations leave the active piece unchanged.
- Verify locking writes the active piece to the settled board, full rows clear, non-full rows compact downward, and each supported simultaneous-clear count applies the correct score.
- Verify a blocked spawn sets game over and reset returns the game to a playable empty-board state.
- Verify each generated seven-bag contains every tetromino exactly once and that seedable/injectable generation produces reproducible sequences.

### End-to-End Tests

- Run the compiled binary through a pseudo-terminal, send representative requested controls, verify grid/status output, and terminate with Ctrl-C within the timeout.
- Confirm a normal interactive shutdown restores terminal settings and cursor visibility by checking command output and manually using the shell after exit.

### Edge Cases

- Input arriving faster than the render loop or in mixed case.
- Gravity and a one-row soft-drop keypress occurring near the same time.
- Rotation at the left/right wall, floor, or next to locked cells.
- Clearing more than one non-adjacent row in the same lock operation.
- A piece locking at the top of the board and a replacement piece that cannot spawn.
- Terminal output redirected or a terminal too small for the frame; document that an interactive POSIX terminal of at least 10 columns plus status width is required.

## Success Criteria

- Running the application in a POSIX terminal displays a 10×20 grid whose empty cells are `.` and occupied cells are `#`.
- An active tetromino descends automatically exactly once every 500 ms, based on elapsed time rather than frame count, without requiring input.
- `S`, `A`, `D`, `E`, and `Q` (in either case) perform a one-row soft drop, left, right, clockwise rotation, and counter-clockwise rotation respectively; illegal moves/rotations, including wall-adjacent rotations, leave state unchanged.
- Each generated bag contains one instance of every tetromino before any tetromino can recur.
- The game locks pieces, clears complete rows, updates score, detects blocked spawning as game over, and allows `R` to restart after game over.
- Unit tests cover the engine rules and the pseudo-terminal test covers start, rendering, requested inputs, and clean termination.
- The test script, compilation with warnings treated as errors, and the end-to-end test complete successfully.

## Validation Commands

Execute every command to validate the feature works correctly with zero regressions.

```bash
./test_runner.sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic main.cpp tetris_game.cpp terminal_ui.cpp -o app
tests/test_terminal_tetris.sh
```

For the final manual interactive check, run the application in a real terminal (not redirected), exercise `A`, `D`, `S`, `E`, and `Q`, then press Ctrl-C and confirm the prompt is usable:

```bash
./app
```

## Notes

- This plan intentionally targets POSIX terminals because raw, non-blocking character input has no portable standard C++ interface. A Windows console implementation is out of scope unless requested.
- Wall kicks, hold pieces, previews, levels, persistent high scores, pause, and colored rendering are deliberately excluded to preserve a minimal initial game.
- `app` is a generated artifact and should remain ignored rather than committed.

## Open Questions

None. The user has explicitly confirmed board size, controls, one-row soft drop behavior, 500 ms gravity, seven-bag generation, blocked wall rotations without kicks, and the initial feature boundary.
