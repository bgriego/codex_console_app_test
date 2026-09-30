#ifndef TETRIS_GAME_HPP
#define TETRIS_GAME_HPP

#include <array>
#include <cstdint>
#include <random>
#include <vector>

constexpr int kBoardRows = 20;
constexpr int kBoardColumns = 10;

enum class PieceType { I, J, L, O, S, T, Z };

struct Cell {
  int row;
  int column;

  bool operator==(const Cell& other) const {
    return row == other.row && column == other.column;
  }
};

using Grid = std::array<std::array<char, kBoardColumns>, kBoardRows>;

struct GameTestAccess;

class SevenBag {
 public:
  explicit SevenBag(std::uint32_t seed);
  PieceType next();

 private:
  std::mt19937 generator_;
  std::array<PieceType, 7> pieces_;
  std::size_t nextIndex_;

  void refill();
};

class Game {
 public:
  explicit Game(std::uint32_t seed = std::random_device{}());

  bool moveLeft();
  bool moveRight();
  bool softDrop();
  bool rotateClockwise();
  bool rotateCounterClockwise();
  void advanceGravity();
  void reset();

  bool isGameOver() const;
  int score() const;
  PieceType activeType() const;
  std::vector<Cell> activeCells() const;
  Grid renderedBoard() const;

 private:
  struct ActivePiece {
    PieceType type;
    int rotation;
    Cell origin;
  };

  Grid locked_{};
  SevenBag bag_;
  ActivePiece active_{};
  int score_ = 0;
  bool gameOver_ = false;

  bool canPlace(const ActivePiece& piece) const;
  bool tryMove(int rowOffset, int columnOffset);
  bool tryRotate(int rotationOffset);
  void lockActivePiece();
 void clearCompletedRows();
 void spawnNextPiece();

  friend struct GameTestAccess;
};

#endif
