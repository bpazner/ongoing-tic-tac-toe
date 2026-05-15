#pragma once

#include "bot.hpp"
#include "game.hpp"
#include "imgui.h"
#include "utils.hpp"

#include <future>
#include <memory>

enum class SceneType { NONE, MENU, GAME, QUIT };
enum class Gamemode { PLAYER_VS_PLAYER, PLAYER_VS_BOT };
enum class PlayerSide { X, O, RANDOM };

// Base class for all scenes, returns the next SceneType each frame
class Scene {
public:
  Scene(AudioContext *audio_ctx, TextureContext *tex_ctx)
      : audio_ctx_(audio_ctx), tex_ctx_(tex_ctx) {}
  virtual SceneType draw() = 0;
  virtual ~Scene() = default;

protected:
  void make_volume_control(float min_dim);

  AudioContext *audio_ctx_;
  TextureContext *tex_ctx_;
};

// Game mode selection, board configuration, and difficulty settings
class MenuScene : public Scene {
public:
  MenuScene(AudioContext *audio_ctx, TextureContext *tex_ctx)
      : Scene(audio_ctx, tex_ctx) {}
  SceneType draw() override;

  Gamemode get_gamemode() const { return gamemode_; }
  PlayerSide get_player_side() const { return player_side_; }
  int get_board_dimension() const { return board_dimension_; };
  int get_in_a_row() const { return in_a_row_; };
  int get_depth() const { return depth_; }

private:
  void make_gamemode_buttons(const ImVec2 &display_size, float min_dim);
  void make_dimension_slider(const ImVec2 &display_size, float min_dim);
  void make_in_a_row_slider(const ImVec2 &display_size, float min_dim);
  void make_difficulty_slider(const ImVec2 &display_size, float min_dim);
  void make_player_side_buttons(const ImVec2 &display_size, float min_dim);

  Gamemode gamemode_;
  PlayerSide player_side_ = PlayerSide::X;
  int board_dimension_ = MIN_BOARD_DIMENSION;
  int in_a_row_ = MIN_TARGET;
  int difficulty_ = 1;
  int depth_;
};

// Renders the board, scores, and action buttons,
// subclassed by PVPScene and PVBScene
class GameScene : public Scene {
public:
  GameScene(unsigned board_dimension, unsigned in_a_row,
            AudioContext *audio_ctx, TextureContext *tex_ctx);
  SceneType draw() final;

protected:
  virtual void pre_draw() {}
  virtual void on_reset() {}
  virtual std::string get_game_label() const { return ""; }
  virtual bool can_move() const { return true; }

  Game game_;

private:
  void make_header_labels(const ImVec2 &display_size, float min_dim);
  void make_board_buttons(const ImVec2 &display_size, float min_dim);
  SceneType make_action_buttons(const ImVec2 &display_size, float min_dim);
};

// Two-player local game
class PVPScene : public GameScene {
public:
  PVPScene(unsigned board_dimension, unsigned in_a_row, AudioContext *audio_ctx,
           TextureContext *tex_ctx);

protected:
  std::string get_game_label() const override;
  bool can_move() const override {
    return game_.get_result() == Result::NOT_OVER;
  }
};

// Player vs bot; bot moves run asynchronously so the UI stays responsive
class PVBScene : public GameScene {
public:
  PVBScene(unsigned board_dimension, unsigned in_a_row, unsigned depth,
           bool player_x, bool iddfs, AudioContext *audio_ctx,
           TextureContext *tex_ctx);

protected:
  void pre_draw() override;
  std::string get_game_label() const override;
  bool can_move() const override {
    return game_.get_result() == Result::NOT_OVER &&
           game_.get_x_turn() == player_x_;
  }
  void on_reset() override { bot_thinking_ = false; }

private:
  std::unique_ptr<Bot> bot_;
  std::future<unsigned> bot_future_;
  unsigned depth_;
  bool player_x_;
  bool bot_thinking_ = false;
};
