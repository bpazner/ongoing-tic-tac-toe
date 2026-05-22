#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// UI/Audio
constexpr float BASE_BGM_VOLUME = 0.5f;
constexpr float BASE_SFX_VOLUME = 2 * BASE_BGM_VOLUME;

struct ma_engine;
struct ma_sound;

struct AudioContext {
  ma_engine* engine = nullptr;
  ma_sound* bgm = nullptr;
  ma_sound* button = nullptr;
  ma_sound* slider = nullptr;
  ma_sound* tile = nullptr;
  ma_sound* game_end_win = nullptr;
  ma_sound* game_end_lose = nullptr;
  ma_sound* game_end_tie = nullptr;
  float bgm_volume = 0.5f;
  float sfx_volume = 0.5f;
};

struct TextureContext {
  unsigned int music_tex = 0;
  unsigned int sfx_tex = 0;
  unsigned int x_tex = 0;
  unsigned int o_tex = 0;
};

// Console colors
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"

// Timing
#define GET_TIME (std::chrono::high_resolution_clock::now())
#define GET_ELAPSED(a, b)                                                 \
  (std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count() / \
   1000.0)

// Game board
constexpr unsigned MIN_BOARD_DIMENSION = 3;
constexpr unsigned MAX_BOARD_DIMENSION = 8;
constexpr unsigned MIN_TARGET = 2;
constexpr char PLAYER_X = 'X';
constexpr char PLAYER_O = 'O';
constexpr char EMPTY = ' ';
constexpr unsigned NO_MOVES = ~0u;

// Helper functions

void print_progress_bar(
    const std::chrono::_V2::system_clock::time_point& start_time,
    double percent, unsigned width = 100, char full = '#', char empty = '_');

void print_board(const std::vector<char>& board, unsigned board_dimension);

std::vector<char> random_board(unsigned board_dimension,
                               double prob_x = 1.0 / 3,
                               double prob_y = 1.0 / 3);

int64_t get_score(const std::vector<char>& board, unsigned board_dimension,
                  unsigned in_a_row, char player);

std::vector<unsigned> get_empty_locs(const std::vector<char>& board,
                                     unsigned board_dimension);

// Returns (row, col) center of mass of non-empty cells on the board
std::pair<double, double> get_center_of_mass(
    const std::vector<char>& board, unsigned board_dimension,
    const std::vector<unsigned>& empty_locs);

double dist_squared(double r1, double c1, double r2, double c2);

// Increases depth according to the decrease in available moves (empty cells)
unsigned get_increased_depth(unsigned init_depth, unsigned n, unsigned n_prime);