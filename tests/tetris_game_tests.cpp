#include "tetris_game.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

struct GameTestAccess {
  static void setLockedCell(Game& game, Cell cell) {
    game.locked_[cell.row][cell.column] = '#';
  }

  static void setActivePiece(Game& game, PieceType type, int rotation,
                             Cell origin) {
    game.active_ = {type, rotation, origin};
  }

  static void lockActivePiece(Game& game) {
    game.lockActivePiece();
  }

  static void spawnNextPiece(Game& game) {
    game.spawnNextPiece();
  }
};

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

void testSpawnedPieceHasFourCellsInsideBoard() {
  Game game(1);
  const std::vector<Cell> cells = game.activeCells();

  require(cells.size() == 4, "a spawned piece has four cells");
  for (const Cell& cell : cells) {
    require(cell.row >= 0 && cell.row < kBoardRows, "spawned row is in bounds");
    require(cell.column >= 0 && cell.column < kBoardColumns,
            "spawned column is in bounds");
  }
}

void testSevenBagContainsEveryPieceExactlyOnce() {
  SevenBag bag(7);
  std::set<PieceType> pieces;

  for (int index = 0; index < 7; ++index) {
    pieces.insert(bag.next());
  }

  require(pieces.size() == 7, "a seven-bag contains all seven tetrominoes");
}

void testMovementStopsAtWallWithoutChangingPiece() {
  Game game(1);
  while (game.moveLeft()) {
  }
  const std::vector<Cell> before = game.activeCells();

  require(!game.moveLeft(), "movement beyond the left wall is rejected");
  require(game.activeCells() == before,
          "rejected movement leaves the active piece unchanged");
}

void testSoftDropAndGravityMoveExactlyOneRow() {
  Game game(1);
  const std::vector<Cell> beforeDrop = game.activeCells();

  require(game.softDrop(), "soft drop moves a free piece");
  const std::vector<Cell> afterDrop = game.activeCells();
  for (std::size_t index = 0; index < beforeDrop.size(); ++index) {
    require(afterDrop[index].row == beforeDrop[index].row + 1,
            "one soft-drop keypress moves each cell down one row");
    require(afterDrop[index].column == beforeDrop[index].column,
            "soft drop does not move a cell sideways");
  }

  game.advanceGravity();
  const std::vector<Cell> afterGravity = game.activeCells();
  for (std::size_t index = 0; index < afterDrop.size(); ++index) {
    require(afterGravity[index].row == afterDrop[index].row + 1,
            "one gravity tick moves each cell down one row");
  }
}

void testClockwiseAndCounterClockwiseRotationsReverseEachOther() {
  Game game(1);
  GameTestAccess::setActivePiece(game, PieceType::T, 0, {5, 3});
  const std::vector<Cell> before = game.activeCells();

  require(game.rotateClockwise(), "clockwise rotation succeeds in open space");
  require(game.rotateCounterClockwise(),
          "counter-clockwise rotation succeeds in open space");
  require(game.activeCells() == before,
          "opposite rotations restore the original cells");
}

void testBlockedWallRotationLeavesPieceUnchanged() {
  Game game(1);
  GameTestAccess::setActivePiece(game, PieceType::I, 1, {0, 8});
  const std::vector<Cell> before = game.activeCells();

  require(!game.rotateCounterClockwise(),
          "rotation crossing the right wall is rejected without a kick");
  require(game.activeCells() == before,
          "rejected wall rotation leaves the active piece unchanged");
}

void testSoftDropLocksPieceAtFloor() {
  Game game(1);
  while (game.softDrop()) {
  }

  int occupied = 0;
  for (const auto& row : game.renderedBoard()) {
    occupied += static_cast<int>(std::count(row.begin(), row.end(), '#'));
  }
  require(occupied == 8,
          "locking at the floor preserves four settled and four active cells");
  require(!game.isGameOver(), "a floor lock starts the next piece");
}

void testSingleLineClearAwardsScore() {
  Game game(1);
  for (int column = 0; column < 8; ++column) {
    GameTestAccess::setLockedCell(game, {19, column});
  }
  GameTestAccess::setActivePiece(game, PieceType::O, 0, {18, 8});

  GameTestAccess::lockActivePiece(game);

  require(game.score() == 100, "clearing one row awards 100 points");
}

void testTwoLineClearAwardsScoreAndCompactsBoard() {
  Game game(1);
  for (int row = 18; row < 20; ++row) {
    for (int column = 0; column < 8; ++column) {
      GameTestAccess::setLockedCell(game, {row, column});
    }
  }
  GameTestAccess::setActivePiece(game, PieceType::O, 0, {18, 8});

  GameTestAccess::lockActivePiece(game);

  require(game.score() == 300, "clearing two rows awards 300 points");
  int occupied = 0;
  for (const auto& row : game.renderedBoard()) {
    occupied += static_cast<int>(std::count(row.begin(), row.end(), '#'));
  }
  require(occupied == 4, "cleared rows leave only the newly spawned piece");
}

void testBlockedSpawnEndsGameAndResetRestoresPlay() {
  Game game(1);
  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < kBoardColumns; ++column) {
      GameTestAccess::setLockedCell(game, {row, column});
    }
  }

  GameTestAccess::spawnNextPiece(game);
  require(game.isGameOver(), "a blocked next spawn ends the game");

  game.reset();
  require(!game.isGameOver(), "reset starts a playable game");
  require(game.score() == 0, "reset clears the score");
}

}  // namespace

int main() {
  testSpawnedPieceHasFourCellsInsideBoard();
  testSevenBagContainsEveryPieceExactlyOnce();
  testMovementStopsAtWallWithoutChangingPiece();
  testSoftDropAndGravityMoveExactlyOneRow();
  testClockwiseAndCounterClockwiseRotationsReverseEachOther();
  testBlockedWallRotationLeavesPieceUnchanged();
  testSoftDropLocksPieceAtFloor();
  testSingleLineClearAwardsScore();
  testTwoLineClearAwardsScoreAndCompactsBoard();
  testBlockedSpawnEndsGameAndResetRestoresPlay();
  std::cout << "tetris_game_tests passed\n";
  return 0;
}
