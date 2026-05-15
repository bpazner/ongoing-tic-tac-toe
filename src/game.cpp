#include "game.hpp"

#include <algorithm>
#include <cassert>
#include <sstream>

Game::Game(unsigned board_dimension, unsigned in_a_row)
    : board_dimension_(board_dimension), in_a_row_(in_a_row), x_turn_(true) {
  assert(board_dimension_ >= MIN_BOARD_DIMENSION &&
         board_dimension_ <= MAX_BOARD_DIMENSION);
  assert(in_a_row_ >= MIN_TARGET && in_a_row_ <= board_dimension_);

  board_ = std::vector<char>(board_dimension_ * board_dimension_, EMPTY);
}

void Game::reset() {
  std::fill(board_.begin(), board_.end(), EMPTY);
  x_turn_ = true;
  x_score_ = 0;
  o_score_ = 0;
  result_ = Result::NOT_OVER;
}

void Game::update_result() {
  // Check if not over
  for (auto cell : board_) {
    if (cell == EMPTY) {
      result_ = Result::NOT_OVER;
      return;
    }
  }

  // Compare scores
  if (x_score_ > o_score_) {
    result_ = Result::X_WIN;
  } else if (o_score_ > x_score_) {
    result_ = Result::O_WIN;
  } else {
    result_ = Result::TIE;
  }
}

Result Game::run_player_vs_bot(bool player_x, Bot &bot,
                               unsigned bot_init_depth) {
  print();
  unsigned n = board_dimension_ * board_dimension_;
  for (unsigned moves = 0; moves < n; moves++) {
    if (get_x_turn() == player_x) {
      // Prompt player for location
      unsigned row, col;
      std::cout << "Enter \"<row> <col>\" or \"exit\": ";

      // Handle errors
      while (true) {
        std::string line;
        std::getline(std::cin, line);
        if (line == "exit") {
          return get_result() == Result::NOT_OVER ? Result::TIE : get_result();
        }

        std::istringstream iss(line);
        if (!(iss >> row >> col)) {
          std::cout << "Invalid input. Please enter two numbers.\n";
          continue;
        }
        if (row >= board_dimension_ || col >= board_dimension_) {
          std::cout << "Out of bounds. Try again.\n";
          continue;
        }
        if (get_cell(row, col) != EMPTY) {
          std::cout << "That cell is taken. Try again.\n";
          continue;
        }
        break;
      }

      // Make move
      make_move(row, col);
    } else {
      // Determine depth of bot and get move
      char bot_player = player_x ? PLAYER_O : PLAYER_X;
      unsigned bot_n = player_x ? n - 1 : n;
      unsigned n_empty = n - moves;
      bot.set_depth(get_increased_depth(bot_init_depth, bot_n, n_empty));
      unsigned loc = bot.get_move(get_board(), bot_player);

      // Play move
      make_move(loc);
    }

    // Print game
    print();
  }

  // Output result
  Result result = get_result();
  if (result == Result::X_WIN) {
    std::cout << RED "X" RESET " wins!\n";
  } else if (result == Result::O_WIN) {
    std::cout << GREEN "O" RESET " wins!\n";
  } else {
    std::cout << "It's a tie...\n";
  }
  return result;
}

Result Game::run_bot_vs_bot(Bot &bot, unsigned x_init_depth,
                            unsigned o_init_depth, bool print_game) {
  auto start_time = GET_TIME;
  if (print_game) {
    print();
  }
  unsigned n = board_dimension_ * board_dimension_;
  for (unsigned moves = 0; moves < n; moves++) {
    unsigned n_empty = n - moves;

    // Determine new depth, get move
    if (get_x_turn()) {
      bot.set_depth(get_increased_depth(x_init_depth, n, n_empty));
      unsigned loc = bot.get_move(get_board(), PLAYER_X, print_game);
      make_move(loc);
    } else {
      bot.set_depth(get_increased_depth(o_init_depth, n - 1, n_empty));
      unsigned loc = bot.get_move(get_board(), PLAYER_O, print_game);
      make_move(loc);
    }

    // Print game
    if (print_game) {
      print();
    }
  }

  // Output result
  Result result = get_result();
  if (print_game) {
    if (result == Result::X_WIN) {
      std::cout << RED "X" RESET " wins!\n";
    } else if (result == Result::O_WIN) {
      std::cout << GREEN "O" RESET " wins!\n";
    } else {
      std::cout << "It's a tie...\n";
    }
    std::cout << "Total elapsed time: " << GET_ELAPSED(start_time, GET_TIME)
              << " seconds\n";
  }

  return result;
}

void Game::print() const {
  print_board(board_, board_dimension_);

  // Print scores
  std::cout << RED "X" RESET ": " << x_score_ << "\n";
  std::cout << GREEN "O" RESET ": " << o_score_ << "\n\n";
}
