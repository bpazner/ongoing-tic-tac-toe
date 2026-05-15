#include "utils.hpp"

#include <algorithm>
#include <iostream>
#include <random>

void print_progress_bar(
    const std::chrono::_V2::system_clock::time_point &start_time,
    double percent, unsigned width, char full, char empty) {
  unsigned num_full = (unsigned)std::round(percent * width);
  std::cout << "[" << std::string(num_full, full)
            << std::string(width - num_full, empty) << "|"
            << (100 * num_full / width)
            << "%] Elapsed Time: " << GET_ELAPSED(start_time, GET_TIME)
            << " sec\r";
}

void print_board(const std::vector<char> &board, unsigned board_dimension) {
  std::string col_labels = "   ";
  for (unsigned c = 0; c < board_dimension; c++) {
    col_labels += " " + std::to_string(c) + "  ";
  }
  std::cout << col_labels << "\n";

  std::string row_sep = "  +";
  for (unsigned i = 0; i < board_dimension; i++) {
    row_sep += "---+";
  }

  std::cout << row_sep << "\n";
  for (unsigned r = 0; r < board_dimension; r++) {
    std::cout << r << " |";
    for (unsigned c = 0; c < board_dimension; c++) {
      char cell = board[r * board_dimension + c];

      std::cout << " ";
      if (cell == PLAYER_X) {
        std::cout << RED;
      } else {
        std::cout << GREEN;
      }
      std::cout << cell << RESET << " |";
    }
    std::cout << "\n" << row_sep << "\n";
  }
}

std::vector<char> random_board(unsigned board_dimension, double prob_x,
                               double prob_y) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dist(0.0, 1.0);

  std::vector<char> board;
  for (unsigned r = 0; r < board_dimension; r++) {
    for (unsigned c = 0; c < board_dimension; c++) {
      double p = dist(gen);
      if (p <= prob_x) {
        board.push_back(PLAYER_X);
      } else if (p <= prob_x + prob_y) {
        board.push_back(PLAYER_O);
      } else {
        board.push_back(EMPTY);
      }
    }
  }

  return board;
}

int64_t get_score(const std::vector<char> &board, unsigned board_dimension,
                  unsigned in_a_row, char player) {
  int64_t score = 0;

  // Check rows
  for (unsigned r = 0; r < board_dimension; r++) {
    int length = 0;
    for (unsigned c = 0; c < board_dimension; c++) {
      if (board[r * board_dimension + c] == player) {
        length++;
      } else {
        score += std::max(0, length - (int)in_a_row + 1);
        length = 0;
      }
    }
    score += std::max(0, length - (int)in_a_row + 1);
  }

  // Check columns
  for (unsigned c = 0; c < board_dimension; c++) {
    int length = 0;
    for (unsigned r = 0; r < board_dimension; r++) {
      if (board[r * board_dimension + c] == player) {
        length++;
      } else {
        score += std::max(0, length - (int)in_a_row + 1);
        length = 0;
      }
    }
    score += std::max(0, length - (int)in_a_row + 1);
  }

  // Check down right diagonals from column 0
  for (unsigned r = 0; r < board_dimension; r++) {
    int length = 0;
    for (unsigned c = 0; r + c < board_dimension; c++) {
      if (board[(r + c) * board_dimension + c] == player) {
        length++;
      } else {
        score += std::max(0, length - (int)in_a_row + 1);
        length = 0;
      }
    }
    score += std::max(0, length - (int)in_a_row + 1);
  }

  // Check down right diagonals from row 0 (skip column 0)
  for (unsigned c = 1; c < board_dimension; c++) {
    int length = 0;
    for (unsigned r = 0; c + r < board_dimension; r++) {
      if (board[r * board_dimension + (c + r)] == player) {
        length++;
      } else {
        score += std::max(0, length - (int)in_a_row + 1);
        length = 0;
      }
    }
    score += std::max(0, length - (int)in_a_row + 1);
  }

  // Check down left diagonals from last column
  for (unsigned r = 0; r < board_dimension; r++) {
    int length = 0;
    for (unsigned i = 0; r + i < board_dimension; i++) {
      if (board[(r + i) * board_dimension + (board_dimension - 1 - i)] ==
          player) {
        length++;
      } else {
        score += std::max(0, length - (int)in_a_row + 1);
        length = 0;
      }
    }
    score += std::max(0, length - (int)in_a_row + 1);
  }

  // Check down left diagonals from row 0 (skip last column)
  for (unsigned c = 0; c < board_dimension - 1; c++) {
    int length = 0;
    for (unsigned r = 0; r <= c; r++) {
      if (board[r * board_dimension + (c - r)] == player) {
        length++;
      } else {
        score += std::max(0, length - (int)in_a_row + 1);
        length = 0;
      }
    }
    score += std::max(0, length - (int)in_a_row + 1);
  }

  return score;
}

std::vector<unsigned> get_empty_locs(const std::vector<char> &board,
                                     unsigned board_dimension) {
  std::vector<unsigned> locs;
  for (unsigned i = 0; i < board.size(); i++) {
    if (board[i] == EMPTY) {
      locs.push_back(i);
    }
  }
  return locs;
}

std::pair<double, double>
get_center_of_mass(const std::vector<char> &board, unsigned board_dimension,
                   const std::vector<unsigned> &empty_locs) {
  unsigned num_pieces = board.size() - empty_locs.size();
  if (num_pieces == 0) {
    double center = 0.5 * (board_dimension - 1);
    return {center, center};
  }

  double r_center = 0, c_center = 0;
  for (unsigned r = 0; r < board_dimension; r++) {
    for (unsigned c = 0; c < board_dimension; c++) {
      if (board[r * board_dimension + c] != EMPTY) {
        r_center += r;
        c_center += c;
      }
    }
  }

  return {r_center / num_pieces, c_center / num_pieces};
}

double dist_squared(double r1, double c1, double r2, double c2) {
  double dr = r1 - r2;
  double dc = c1 - c2;
  return dr * dr + dc * dc;
}

unsigned get_increased_depth(unsigned init_depth, unsigned n,
                             unsigned n_prime) {
  unsigned c = std::round(init_depth * std::log(n) / std::log(n_prime));
  // Round up to even number
  unsigned res = c + (c & 1);
  return res;
}
