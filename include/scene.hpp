#pragma once

#include <future>
#include <memory>
#include <thread>

#include "bot.hpp"
#include "game.hpp"
#include "imgui.h"
#include "utils.hpp"

enum class SceneType { NONE, MENU, GAME, QUIT };
enum class Gamemode {
  LOCAL_MULTIPLAYER,
  ONLINE_MULTIPLAYER,
  AGAINST_BOT,
  BOT_VS_BOT
};
enum class PlayerSide { X, O, RANDOM };

// Base class for all scenes, returns the next SceneType each frame
class Scene {
 public:
  Scene(AudioContext* audio_ctx, TextureContext* tex_ctx)
      : audio_ctx_(audio_ctx), tex_ctx_(tex_ctx) {}
  virtual SceneType draw() = 0;
  virtual ~Scene() = default;

 protected:
  void make_volume_control(float min_dim);

  AudioContext* audio_ctx_;
  TextureContext* tex_ctx_;
};

// Game mode selection, board configuration, and difficulty settings
class MenuScene : public Scene {
 public:
  MenuScene(AudioContext* audio_ctx, TextureContext* tex_ctx)
      : Scene(audio_ctx, tex_ctx) {}
  SceneType draw() override;

  Gamemode get_gamemode() const { return gamemode_; }
  PlayerSide get_player_side() const { return player_side_; }
  int get_board_dimension() const { return board_dimension_; };
  int get_in_a_row() const { return in_a_row_; }
  const char* get_server_address() const { return server_address_; }
  int get_depth1() const { return depth1_; }
  int get_depth2() const { return depth2_; }

 private:
  void make_gamemode_dropdown(const ImVec2& display_size, float min_dim);
  void make_dimension_slider(const ImVec2& display_size, float min_dim);
  void make_in_a_row_slider(const ImVec2& display_size, float min_dim);
  void make_server_input(const ImVec2& display_size, float min_dim);
  void make_difficulty_slider(const ImVec2& display_size, float min_dim,
                              bool is_first_slider);
  void make_player_side_buttons(const ImVec2& display_size, float min_dim);

  Gamemode gamemode_ = Gamemode::LOCAL_MULTIPLAYER;
  PlayerSide player_side_ = PlayerSide::X;
  int board_dimension_ = MIN_BOARD_DIMENSION;
  int in_a_row_ = MIN_TARGET;
  char server_address_[256] = "localhost:12345";
  int difficulty1_ = 1;
  int difficulty2_ = 1;
  int depth1_ = 1;
  int depth2_ = 1;
};

// Renders the board, scores, and action buttons,
// subclassed by PVPScene and PVBScene
class GameScene : public Scene {
 public:
  GameScene(unsigned board_dimension, unsigned in_a_row,
            AudioContext* audio_ctx, TextureContext* tex_ctx);
  SceneType draw() final;

 protected:
  virtual void pre_draw() {}
  virtual void on_reset() {}
  virtual void play_game_end_sfx();
  virtual std::string get_game_label() const { return ""; }
  virtual bool can_move() const { return true; }

  float bot_speed_ = 0.5f;
  virtual void draw_bot_speed_slider(const ImVec2& display_size,
                                     float min_dim) {}
  float get_min_bot_time() const { return 1.0f - bot_speed_; }

  // Makes move and updates tint times for any scoring cells
  void make_move_wrapper(unsigned r, unsigned c);
  void make_move_wrapper(unsigned loc) {
    make_move_wrapper(loc / game_.get_board_dimension(),
                      loc % game_.get_board_dimension());
  }

  Game game_;

  // tint_start_times_[r][c] = start time(s) of tint animation(s) for cell (r,
  // c)
  std::vector<std::vector<std::vector<double>>> tint_start_times_;

 private:
  void make_header_labels(const ImVec2& display_size, float min_dim);
  SceneType make_action_buttons(const ImVec2& display_size, float min_dim);
  void make_board_buttons(const ImVec2& display_size, float min_dim);

  struct CellEmphasis {
    ImVec4 tint;
    double size_factor;
  };
  CellEmphasis get_cell_emphasis(unsigned r, unsigned c);
};

// Two-player local game
class PVPLocalScene : public GameScene {
 public:
  PVPLocalScene(unsigned board_dimension, unsigned in_a_row,
                AudioContext* audio_ctx, TextureContext* tex_ctx);

 protected:
  std::string get_game_label() const override;
  bool can_move() const override {
    return game_.get_result() == Result::NOT_OVER;
  }
};

// Two-player online game
class PVPOnlineScene : public GameScene {
 public:
  PVPOnlineScene(const char* server_address, unsigned board_dimension,
                 unsigned in_a_row, AudioContext* audio_ctx,
                 TextureContext* tex_ctx);

 protected:
  void pre_draw() override {}
  void play_game_end_sfx() override {}
  std::string get_game_label() const override;
  bool can_move() const override {
    return game_.get_result() == Result::NOT_OVER &&
           game_.get_x_turn() == player_x_;
  }
  void on_reset() override {};

 private:
  const char* server_address_;
  bool player_x_;
  int sock_ = -1;
};

// Player vs bot, bot moves run asynchronously so the UI stays responsive
class PVBScene : public GameScene {
 public:
  PVBScene(unsigned board_dimension, unsigned in_a_row, unsigned depth,
           bool player_x, AudioContext* audio_ctx, TextureContext* tex_ctx);
  ~PVBScene();

 protected:
  void pre_draw() override;
  void play_game_end_sfx() override;
  std::string get_game_label() const override;
  bool can_move() const override {
    return game_.get_result() == Result::NOT_OVER &&
           game_.get_x_turn() == player_x_;
  }
  void on_reset() override { bot_thinking_ = false; }

 private:
  std::shared_ptr<Bot> bot_;
  std::future<unsigned> bot_future_;
  unsigned depth_;
  bool player_x_;
  bool bot_thinking_ = false;
  double bot_move_start_time_;
};

// Bot vs bot, bots run asynchronously so the UI stays responsive
class BVBScene : public GameScene {
 public:
  BVBScene(unsigned board_dimension, unsigned in_a_row, unsigned x_depth,
           unsigned o_depth, AudioContext* audio_ctx, TextureContext* tex_ctx);
  ~BVBScene();

 protected:
  void pre_draw() override;
  std::string get_game_label() const override;
  bool can_move() const override {
    return false;  // Human cannot make moves in this mode
  }
  void on_reset() override { bot_thinking_ = false; }

  void draw_bot_speed_slider(const ImVec2&, float) override;

 private:
  std::shared_ptr<Bot> x_bot_;
  std::shared_ptr<Bot> o_bot_;
  std::future<unsigned> bot_future_;
  bool bot_thinking_ = false;
  double bot_move_start_time_;
};
