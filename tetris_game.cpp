#include "tetris_game.hpp"

#include <algorithm>
#include <array>

namespace {

using Shape = std::array<Cell, 4>;

Grid emptyGrid() {
  Grid grid{};
  for (auto& row : grid) {
    row.fill('.');
  }
  return grid;
}

Shape shapeFor(PieceType type, int rotation) {
  const int turn = (rotation % 4 + 4) % 4;

  switch (type) {
    case PieceType::I:
      return turn % 2 == 0 ? Shape{{{0, 0}, {0, 1}, {0, 2}, {0, 3}}}
                           : Shape{{{0, 1}, {1, 1}, {2, 1}, {3, 1}}};
    case PieceType::O:
      return Shape{{{0, 0}, {0, 1}, {1, 0}, {1, 1}}};
    case PieceType::T:
      switch (turn) {
        case 0: return Shape{{{0, 1}, {1, 0}, {1, 1}, {1, 2}}};
        case 1: return Shape{{{0, 1}, {1, 1}, {1, 2}, {2, 1}}};
        case 2: return Shape{{{1, 0}, {1, 1}, {1, 2}, {2, 1}}};
        default: return Shape{{{0, 1}, {1, 0}, {1, 1}, {2, 1}}};
      }
    case PieceType::J:
      switch (turn) {
        case 0: return Shape{{{0, 0}, {1, 0}, {1, 1}, {1, 2}}};
        case 1: return Shape{{{0, 1}, {0, 2}, {1, 1}, {2, 1}}};
        case 2: return Shape{{{1, 0}, {1, 1}, {1, 2}, {2, 2}}};
        default: return Shape{{{0, 1}, {1, 1}, {2, 0}, {2, 1}}};
      }
    case PieceType::L:
      switch (turn) {
        case 0: return Shape{{{0, 2}, {1, 0}, {1, 1}, {1, 2}}};
        case 1: return Shape{{{0, 1}, {1, 1}, {2, 1}, {2, 2}}};
        case 2: return Shape{{{1, 0}, {1, 1}, {1, 2}, {2, 0}}};
        default: return Shape{{{0, 0}, {0, 1}, {1, 1}, {2, 1}}};
      }
    case PieceType::S:
      return turn % 2 == 0 ? Shape{{{0, 1}, {0, 2}, {1, 0}, {1, 1}}}
                           : Shape{{{0, 0}, {1, 0}, {1, 1}, {2, 1}}};
    case PieceType::Z:
      return turn % 2 == 0 ? Shape{{{0, 0}, {0, 1}, {1, 1}, {1, 2}}}
                           : Shape{{{0, 1}, {1, 0}, {1, 1}, {2, 0}}};
  }

  return Shape{};
}

int scoreForLines(int lines) {
  constexpr std::array<int, 5> kScores{{0, 100, 300, 500, 800}};
  return kScores.at(static_cast<std::size_t>(lines));
}

}  // namespace

SevenBag::SevenBag(std::uint32_t seed)
    : generator_(seed), pieces_{}, nextIndex_(pieces_.size()) {}

PieceType SevenBag::next() {
  if (nextIndex_ == pieces_.size()) {
    refill();
  }
  return pieces_[nextIndex_++];
}

void SevenBag::refill() {
  pieces_ = {{PieceType::I, PieceType::J, PieceType::L, PieceType::O,
              PieceType::S, PieceType::T, PieceType::Z}};
  std::shuffle(pieces_.begin(), pieces_.end(), generator_);
  nextIndex_ = 0;
}

Game::Game(std::uint32_t seed) : bag_(seed) {
  reset();
}

bool Game::moveLeft() {
  return !gameOver_ && tryMove(0, -1);
}

bool Game::moveRight() {
  return !gameOver_ && tryMove(0, 1);
}

bool Game::softDrop() {
  if (gameOver_) {
    return false;
  }
  if (tryMove(1, 0)) {
    return true;
  }
  lockActivePiece();
  return false;
}

bool Game::rotateClockwise() {
  return !gameOver_ && tryRotate(1);
}

bool Game::rotateCounterClockwise() {
  return !gameOver_ && tryRotate(-1);
}

void Game::advanceGravity() {
  softDrop();
}

void Game::reset() {
  locked_ = emptyGrid();
  score_ = 0;
  gameOver_ = false;
  spawnNextPiece();
}

bool Game::isGameOver() const {
  return gameOver_;
}

int Game::score() const {
  return score_;
}

PieceType Game::activeType() const {
  return active_.type;
}

std::vector<Cell> Game::activeCells() const {
  const Shape shape = shapeFor(active_.type, active_.rotation);
  std::vector<Cell> cells;
  cells.reserve(shape.size());
  for (const Cell& cell : shape) {
    cells.push_back({active_.origin.row + cell.row,
                     active_.origin.column + cell.column});
  }
  return cells;
}

Grid Game::renderedBoard() const {
  Grid grid = locked_;
  if (!gameOver_) {
    for (const Cell& cell : activeCells()) {
      grid[cell.row][cell.column] = '#';
    }
  }
  return grid;
}

bool Game::canPlace(const ActivePiece& piece) const {
  const Shape shape = shapeFor(piece.type, piece.rotation);
  for (const Cell& cell : shape) {
    const int row = piece.origin.row + cell.row;
    const int column = piece.origin.column + cell.column;
    if (row < 0 || row >= kBoardRows || column < 0 ||
        column >= kBoardColumns || locked_[row][column] == '#') {
      return false;
    }
  }
  return true;
}

bool Game::tryMove(int rowOffset, int columnOffset) {
  ActivePiece candidate = active_;
  candidate.origin.row += rowOffset;
  candidate.origin.column += columnOffset;
  if (!canPlace(candidate)) {
    return false;
  }
  active_ = candidate;
  return true;
}

bool Game::tryRotate(int rotationOffset) {
  ActivePiece candidate = active_;
  candidate.rotation += rotationOffset;
  if (!canPlace(candidate)) {
    return false;
  }
  active_ = candidate;
  return true;
}

void Game::lockActivePiece() {
  for (const Cell& cell : activeCells()) {
    locked_[cell.row][cell.column] = '#';
  }
  clearCompletedRows();
  spawnNextPiece();
}

void Game::clearCompletedRows() {
  int writeRow = kBoardRows - 1;
  int cleared = 0;
  for (int readRow = kBoardRows - 1; readRow >= 0; --readRow) {
    const bool complete = std::all_of(
        locked_[readRow].begin(), locked_[readRow].end(),
        [](char value) { return value == '#'; });
    if (complete) {
      ++cleared;
      continue;
    }
    locked_[writeRow--] = locked_[readRow];
  }
  while (writeRow >= 0) {
    locked_[writeRow--].fill('.');
  }
  score_ += scoreForLines(cleared);
}

void Game::spawnNextPiece() {
  active_ = {bag_.next(), 0, {0, 3}};
  if (!canPlace(active_)) {
    gameOver_ = true;
  }
}
