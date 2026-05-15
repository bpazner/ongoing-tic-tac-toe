#pragma once

#include "utils.hpp"

#include <unordered_map>

class Bot {
public:
  Bot(unsigned board_dimension = MIN_BOARD_DIMENSION,
      unsigned in_a_row = MIN_TARGET, unsigned depth = 2,
      bool iterative_deepening = false);
  ~Bot() = default;

  // Returns index of best move as row * dimension + col
  // Use even depth_ for balanced play
  unsigned get_move(const std::vector<char> &board, char player,
                    bool print_stats = true);

  void set_depth(unsigned depth);

private:
  // Returns (value, loc) pairs sorted best-first for <player>
  // Takes <depth> parameter due to optional iterative deepening method
  std::vector<std::pair<int64_t, unsigned>>
  get_moves_sorted(unsigned depth, std::vector<char> &board,
                   const std::pair<double, double> &board_center_of_mass,
                   std::vector<unsigned> &empty_locs, char player);

  // Max player: 'X', min player: 'O'
  int64_t minimax(unsigned depth, std::vector<char> &board,
                  std::vector<unsigned> &empty_locs, int64_t alpha,
                  int64_t beta, bool is_max);
  uint64_t get_game_state(const std::vector<char> &board, bool is_max) const;

  int64_t evaluate_board(const std::vector<char> &board,
                         const std::vector<unsigned> &empty_locs) const;
  // Called by evaluate_board, heuristic score contribution of one empty cell
  int64_t evaluate_empty_loc(const std::vector<char> &board, unsigned loc,
                             char player) const;
  // Called by evaluate_empty_loc
  // computes max streak length through (r, c) in direction (dr, dc)
  unsigned scan_direction(int r, int c, int dr, int dc,
                          const std::vector<char> &board, char player) const;

  unsigned board_dimension_, in_a_row_, depth_;
  bool iterative_deepening_;
  int64_t in_a_row_score_;

  unsigned nodes_explored_ = 0;

  // zobrist_[player][loc][piece]
  // player: 0 = X's turn, 1 = O's turn
  // loc:    range(board_dimension^2)
  // piece:  0 = X, 1 = O
  std::vector<std::vector<std::vector<uint64_t>>> zobrist_;

  // Transposition table
  enum class TTFlag : uint8_t { EXACT, ALPHA, BETA };
  struct TTEntry {
    int64_t value;
    uint32_t depth;
    TTFlag flag;
  };
  std::unordered_map<uint64_t, TTEntry> tt_;
};