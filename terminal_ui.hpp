#ifndef TERMINAL_UI_HPP
#define TERMINAL_UI_HPP

#include "tetris_game.hpp"

#include <optional>
#include <termios.h>

class TerminalSession {
 public:
  TerminalSession();
  ~TerminalSession();

  TerminalSession(const TerminalSession&) = delete;
  TerminalSession& operator=(const TerminalSession&) = delete;

  std::optional<char> pollInput() const;
  void render(const Grid& board, int score, bool gameOver) const;

 private:
  termios originalSettings_{};
  int originalFlags_ = 0;
};

#endif
