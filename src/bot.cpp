#include "bot.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>

Bot::Bot(unsigned board_dimension, unsigned in_a_row, unsigned depth,
         bool iterative_deepening)
    : board_dimension_(board_dimension), in_a_row_(in_a_row), depth_(depth),
      iterative_deepening_(iterative_deepening) {
  assert(board_dimension_ >= MIN_BOARD_DIMENSION &&
         board_dimension_ <= MAX_BOARD_DIMENSION);
  assert(in_a_row_ >= MIN_TARGET && in_a_row_ <= board_dimension_);

  // Set score for target in a row
  in_a_row_score_ = 1 << in_a_row;

  // Generate random zobrist table
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dist;
  zobrist_.resize(2);
  for (auto &by_player : zobrist_) {
    by_player.resize(board_dimension_ * board_dimension_);
    for (auto &by_loc : by_player) {
      by_loc.push_back(dist(gen)); // X
      by_loc.push_back(dist(gen)); // O
    }
  }
}

void Bot::set_depth(unsigned depth) { depth_ = depth; }

unsigned Bot::get_move(const std::vector<char> &board, char player,
                       bool print_stats) {
  assert(board.size() == board_dimension_ * board_dimension_);
  assert(player == PLAYER_X || player == PLAYER_O);

  // Initialize structures for get_moves_sorted call
  auto board_copy{board};
  auto empty_locs = get_empty_locs(board, board_dimension_);
  if (empty_locs.empty()) {
    // No moves
    return NO_MOVES;
  }
  auto board_center_of_mass =
      get_center_of_mass(board, board_dimension_, empty_locs);

  // Sort empty_locs by center of mass distance heuristic
  std::sort(empty_locs.begin(), empty_locs.end(),
            [&](const unsigned &a, const unsigned &b) {
              double ra = a / board_dimension_,
                     ca = a - (ra * board_dimension_);
              double rb = b / board_dimension_,
                     cb = b - (rb * board_dimension_);
              return dist_squared(ra, ca, board_center_of_mass.first,
                                  board_center_of_mass.second) <
                     dist_squared(rb, cb, board_center_of_mass.first,
                                  board_center_of_mass.second);
            });

  // Perform search
  auto start_time = GET_TIME;
  nodes_explored_ = 0;

  std::vector<std::pair<int64_t, unsigned int>> moves;
  if (iterative_deepening_) {
    // Uses searches of less depths for move ordering
    for (unsigned d = 1; d <= depth_; d++) {
      moves = get_moves_sorted(d, board_copy, board_center_of_mass, empty_locs,
                               player);
      assert(!moves.empty());
      assert(board_copy == board);

      // Reconstruct empty_locs according to previous search
      auto it = moves.begin();
      for (unsigned &loc : empty_locs) {
        loc = it->second;
        it++;
      }
    }
  } else {
    // Uses center of mass distance heuristic for move ordering
    moves = get_moves_sorted(depth_, board_copy, board_center_of_mass,
                             empty_locs, player);
    assert(!moves.empty());
    assert(board_copy == board);
  }

  // Count moves tied for best value, pick one randomly
  int64_t best_val = moves[0].first;
  size_t num_best = 1;
  while (num_best < moves.size() && moves[num_best].first == best_val) {
    num_best++;
  }

  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<size_t> dist(0, num_best - 1);
  const auto &[val, best_move] = moves[dist(rng)];

  // Print results
  if (print_stats) {
    std::cout << "Elapsed time   : " << GET_ELAPSED(start_time, GET_TIME)
              << " seconds\n";
    std::cout << "Depth          : " << depth_ << "\n";
    std::cout << "Nodes explored : " << nodes_explored_ << "\n";
    std::cout << "Value          : " << val << "\n";
  }

  return best_move;
}

std::vector<std::pair<int64_t, unsigned>>
Bot::get_moves_sorted(unsigned depth, std::vector<char> &board,
                      const std::pair<double, double> &board_center_of_mass,
                      std::vector<unsigned> &empty_locs, char player) {
  bool is_max = (player == PLAYER_X);
  std::vector<std::pair<int64_t, unsigned>> scored;
  scored.reserve(empty_locs.size());

  for (unsigned i = 0; i < empty_locs.size(); i++) {
    unsigned loc = empty_locs[i];

    // Make move
    board[loc] = player;
    std::swap(empty_locs[i], empty_locs.back());
    empty_locs.pop_back();

    // Use full bounds per move so each value is exact, not a cutoff
    int64_t v =
        minimax(depth - 1, board, empty_locs, INT64_MIN, INT64_MAX, !is_max);
    scored.push_back({v, loc});

    // Undo move
    board[loc] = EMPTY;
    empty_locs.push_back(loc);
    std::swap(empty_locs[i], empty_locs.back());
  }

  // Sort best-first, breaking ties by proximity to center of mass
  auto cmp = [&](const std::pair<int64_t, unsigned> &a,
                 const std::pair<int64_t, unsigned> &b) {
    if (a.first != b.first) {
      return is_max ? a.first > b.first : a.first < b.first;
    }
    double ra = a.second / board_dimension_,
           ca = a.second - (ra * board_dimension_);
    double rb = b.second / board_dimension_,
           cb = b.second - (rb * board_dimension_);
    return dist_squared(ra, ca, board_center_of_mass.first,
                        board_center_of_mass.second) <
           dist_squared(rb, cb, board_center_of_mass.first,
                        board_center_of_mass.second);
  };
  std::sort(scored.begin(), scored.end(), cmp);

  return scored;
}

int64_t Bot::minimax(unsigned depth, std::vector<char> &board,
                     std::vector<unsigned> &empty_locs, int64_t alpha,
                     int64_t beta, bool is_max) {
  nodes_explored_++;
  auto old_alpha = alpha;
  auto old_beta = beta;

  // Search transposition table
  uint64_t state = get_game_state(board, is_max);
  auto tt_pair = tt_.find(state);
  if (tt_pair != tt_.end()) {
    auto tt_entry = tt_pair->second;
    if (tt_entry.depth >= depth) {
      if (tt_entry.flag == TTFlag::EXACT) {
        return tt_entry.value;
      }
      if (tt_entry.flag == TTFlag::ALPHA) {
        alpha = std::max(alpha, tt_entry.value);
      } else if (tt_entry.flag == TTFlag::BETA) {
        beta = std::min(beta, tt_entry.value);
      }
      if (alpha >= beta) {
        return tt_entry.value;
      }
    }
  }

  // Reached full depth or no more moves
  if (depth == 0 || empty_locs.empty()) {
    return evaluate_board(board, empty_locs);
  }

  // Simulate moves
  int64_t value;
  if (is_max) {
    value = INT64_MIN;
    for (unsigned i = 0; i < empty_locs.size(); i++) {
      // Make and evaluate move
      unsigned loc = empty_locs[i];
      board[loc] = PLAYER_X;
      std::swap(empty_locs[i], empty_locs.back());
      empty_locs.pop_back();

      auto child = minimax(depth - 1, board, empty_locs, alpha, beta, !is_max);

      // Undo move
      board[loc] = EMPTY;
      empty_locs.push_back(loc);
      std::swap(empty_locs[i], empty_locs.back());

      // Analyze move value
      value = std::max(value, child);
      alpha = std::max(alpha, child);
      if (alpha >= beta) {
        break;
      }
    }
  } else {
    value = INT64_MAX;
    for (unsigned i = 0; i < empty_locs.size(); i++) {
      // Make and evaluate move
      unsigned loc = empty_locs[i];
      board[loc] = PLAYER_O;
      std::swap(empty_locs[i], empty_locs.back());
      empty_locs.pop_back();

      auto child = minimax(depth - 1, board, empty_locs, alpha, beta, !is_max);

      // Undo move
      board[loc] = EMPTY;
      empty_locs.push_back(loc);
      std::swap(empty_locs[i], empty_locs.back());

      // Analyze move value
      value = std::min(value, child);
      beta = std::min(beta, child);
      if (alpha >= beta) {
        break;
      }
    }
  }

  // Store in transposition table if no better entry exists
  auto existing = tt_.find(state);
  if (existing == tt_.end() || existing->second.depth <= depth) {
    TTEntry entry = {value, depth, TTFlag::EXACT};
    if (value <= old_alpha) {
      entry.flag = TTFlag::BETA;
    } else if (value >= old_beta) {
      entry.flag = TTFlag::ALPHA;
    }
    tt_[state] = entry;
  }

  return value;
}

uint64_t Bot::get_game_state(const std::vector<char> &board,
                             bool is_max) const {
  uint64_t hash = 0;
  unsigned p = is_max ? 0 : 1;

  for (unsigned loc = 0; loc < board.size(); loc++) {
    if (board[loc] == PLAYER_X) {
      hash ^= zobrist_[p][loc][0];
    } else if (board[loc] == PLAYER_O) {
      hash ^= zobrist_[p][loc][1];
    }
  }

  return hash;
}

int64_t Bot::evaluate_board(const std::vector<char> &board,
                            const std::vector<unsigned> &empty_locs) const {
  // Initialize eval based on actual scores of each player
  int64_t max_score = get_score(board, board_dimension_, in_a_row_, PLAYER_X);
  int64_t min_score = get_score(board, board_dimension_, in_a_row_, PLAYER_O);
  int64_t eval = in_a_row_score_ * (max_score - min_score);

  // Empty cell heuristic
  for (unsigned loc : empty_locs) {
    // Check effect of playing in this location for each player
    for (char player : {PLAYER_X, PLAYER_O}) {
      eval += evaluate_empty_loc(board, loc, player);
    }
  }

  return eval;
}

int64_t Bot::evaluate_empty_loc(const std::vector<char> &board, unsigned loc,
                                char player) const {
  int64_t eval = 0;

  unsigned r = loc / board_dimension_;
  unsigned c = loc - (r * board_dimension_);

  // Check rows, columns, and diagonals
  constexpr std::pair<int, int> dirs[] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
  for (auto [dr, dc] : dirs) {
    unsigned length = scan_direction(r, c, dr, dc, board, player);
    if (length >= in_a_row_) {
      // Compute additional score of playing this location
      eval += in_a_row_score_ * (length - in_a_row_ + 1);
    } else if (length >= 3) {
      // Compute heuristic of decent streak
      eval += in_a_row_score_ >> (in_a_row_ - length);
    }
  }

  // Account for the fact that the above is a hypothetical move
  eval >>= 1;

  if (player == PLAYER_X) {
    return eval;
  }
  return -eval;
}

unsigned Bot::scan_direction(int r, int c, int dr, int dc,
                             const std::vector<char> &board,
                             char player) const {
  // Scan backward
  int i = 1;
  while (i < in_a_row_) {
    int nr = r - dr * i;
    int nc = c - dc * i;

    if (nr < 0 || nc < 0 || nr >= (int)board_dimension_ ||
        nc >= (int)board_dimension_) {
      break;
    }
    if (board[nr * board_dimension_ + nc] != player) {
      break;
    }

    i++;
  }
  unsigned back = i - 1;

  // Scan forward
  i = 1;
  while (i < in_a_row_) {
    int nr = r + dr * i;
    int nc = c + dc * i;

    if (nr < 0 || nc < 0 || nr >= (int)board_dimension_ ||
        nc >= (int)board_dimension_) {
      break;
    }
    if (board[nr * board_dimension_ + nc] != player) {
      break;
    }

    i++;
  }
  unsigned forward = i - 1;

  // Include empty space at (r, c)
  return back + 1 + forward;
}
