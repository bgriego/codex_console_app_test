#include "terminal_ui.hpp"

#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

TerminalSession::TerminalSession() {
  if (tcgetattr(STDIN_FILENO, &originalSettings_) != 0) {
    throw std::runtime_error("could not read terminal settings");
  }
  originalFlags_ = fcntl(STDIN_FILENO, F_GETFL);
  if (originalFlags_ == -1) {
    throw std::runtime_error("could not read terminal input flags");
  }

  termios rawSettings = originalSettings_;
  rawSettings.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
  rawSettings.c_cc[VMIN] = 0;
  rawSettings.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &rawSettings) != 0 ||
      fcntl(STDIN_FILENO, F_SETFL, originalFlags_ | O_NONBLOCK) == -1) {
    tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings_);
    throw std::runtime_error("could not configure terminal input");
  }

  std::cout << "\033[?25l" << std::flush;
}

TerminalSession::~TerminalSession() {
  tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings_);
  fcntl(STDIN_FILENO, F_SETFL, originalFlags_);
  std::cout << "\033[?25h" << std::flush;
}

std::optional<char> TerminalSession::pollInput() const {
  char input = '\0';
  const ssize_t bytesRead = read(STDIN_FILENO, &input, 1);
  if (bytesRead == 1) {
    return input;
  }
  if (bytesRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
    throw std::runtime_error("could not read terminal input");
  }
  return std::nullopt;
}

void TerminalSession::render(const Grid& board, int score, bool gameOver) const {
  std::cout << "\033[2J\033[H";
  for (const auto& row : board) {
    for (const char cell : row) {
      std::cout << cell;
    }
    std::cout << '\n';
  }
  std::cout << "Score: " << score << '\n';
  if (gameOver) {
    std::cout << "Game over. Press R to restart or Ctrl-C to exit.\n";
  } else {
    std::cout << "A/D: move  S: drop  E/Q: rotate\n";
  }
  std::cout << std::flush;
}
