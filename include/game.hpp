#pragma once

#include "bot.hpp"
#include "utils.hpp"

enum class Result { X_WIN, O_WIN, TIE, NOT_OVER };

// Manages board state, scores, and turn order for one game
class Game {
public:
  Game(unsigned board_dimension = MIN_BOARD_DIMENSION,
       unsigned in_a_row = MIN_TARGET);

  // Resets board, scores, turn, and result to initial state
  void reset();

  unsigned get_board_dimension() const { return board_dimension_; }
  unsigned get_in_a_row() const { return in_a_row_; }
  std::vector<char> get_board() const { return board_; }
  bool get_x_turn() const { return x_turn_; }
  int64_t get_x_score() const { return x_score_; }
  int64_t get_o_score() const { return o_score_; }
  char get_cell(unsigned r, unsigned c) const {
    return board_[r * board_dimension_ + c];
  }
  // Places the current player's piece, updates scores and result,
  // and advances turn
  void make_move(unsigned loc) {
    board_[loc] = x_turn_ ? PLAYER_X : PLAYER_O;
    x_turn_ = !x_turn_;
    x_score_ = get_score(board_, board_dimension_, in_a_row_, PLAYER_X);
    o_score_ = get_score(board_, board_dimension_, in_a_row_, PLAYER_O);
    update_result();
  }
  void make_move(unsigned r, unsigned c) {
    make_move(r * board_dimension_ + c);
  }

  Result get_result() const { return result_; }
  // Sets result_ based on current board state,
  // NOT_OVER if any empty cell remains
  void update_result();

  // Interactive CLI loop, <player_x> true means the human plays as X
  Result run_player_vs_bot(bool player_x, Bot &bot, unsigned bot_init_depth);
  Result run_bot_vs_bot(Bot &bot, unsigned x_init_depth, unsigned o_init_depth,
                        bool print_game = false);

  void print() const;

private:
  unsigned board_dimension_, in_a_row_;
  std::vector<char> board_;

  bool x_turn_;
  unsigned x_score_ = 0, o_score_ = 0;

  Result result_ = Result::NOT_OVER;
};