#include "terminal_ui.hpp"
#include "tetris_game.hpp"

#include <chrono>
#include <cctype>
#include <csignal>
#include <optional>
#include <thread>

namespace {

volatile std::sig_atomic_t exitRequested = 0;

void requestExit(int) {
  exitRequested = 1;
}

bool handleInput(Game& game, char input) {
  const char key = static_cast<char>(
      std::tolower(static_cast<unsigned char>(input)));
  if (game.isGameOver()) {
    if (key == 'r') {
      game.reset();
      return true;
    }
    return false;
  }

  switch (key) {
    case 'a': game.moveLeft(); break;
    case 'd': game.moveRight(); break;
    case 's': game.softDrop(); break;
    case 'e': game.rotateClockwise(); break;
    case 'q': game.rotateCounterClockwise(); break;
    default: break;
  }
  return false;
}

}  // namespace

int main() {
  std::signal(SIGINT, requestExit);

  TerminalSession terminal;
  Game game;
  constexpr auto gravityInterval = std::chrono::milliseconds(500);
  auto nextGravity = std::chrono::steady_clock::now() + gravityInterval;

  while (exitRequested == 0) {
    bool restarted = false;
    while (const std::optional<char> input = terminal.pollInput()) {
      restarted = handleInput(game, *input) || restarted;
    }

    const auto now = std::chrono::steady_clock::now();
    if (restarted) {
      nextGravity = now + gravityInterval;
    }
    while (!game.isGameOver() && now >= nextGravity) {
      game.advanceGravity();
      nextGravity += gravityInterval;
    }
    terminal.render(game.renderedBoard(), game.score(), game.isGameOver());
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  return 0;
}
