#pragma once

#include "bot.hpp"
#include "game.hpp"
#include "imgui.h"
#include "utils.hpp"

#include <future>
#include <memory>

struct ma_engine;
struct ma_sound;

enum class SceneType { NONE, MENU, GAME, QUIT };
enum class Gamemode { PLAYER_VS_PLAYER, PLAYER_VS_BOT };
enum class PlayerSide { X, O, RANDOM };

// Base class for all scenes, returns the next SceneType each frame
class Scene {
public:
  Scene(ma_engine *audio, ma_sound *bgm, unsigned int music_tex)
      : audio_(audio), bgm_(bgm), music_tex_(music_tex) {}
  virtual SceneType draw() = 0;
  virtual ~Scene() = default;

protected:
  void make_volume_control(float min_dim);

  ma_engine *audio_;
  ma_sound *bgm_;
  unsigned int music_tex_;
  float volume_ = 0.5f;
};

// Game mode selection, board configuration, and difficulty settings
class MenuScene : public Scene {
public:
  MenuScene(ma_engine *audio, ma_sound *bgm, unsigned int music_tex)
      : Scene(audio, bgm, music_tex) {}
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
  GameScene(unsigned board_dimension, unsigned in_a_row, unsigned int x_tex,
            unsigned int o_tex, ma_engine *audio, ma_sound *bgm,
            unsigned int music_tex);
  SceneType draw() final;

protected:
  virtual void pre_draw() {} // Called before rendering each frame
  virtual void on_reset() {} // Called before game_.reset()
  virtual std::string get_game_label() const {
    return "";
  } // Center status label text
  virtual bool can_move() const {
    return true;
  } // Whether the player can click a cell

  Game game_;
  unsigned int x_tex_, o_tex_;

private:
  void make_header_labels(const ImVec2 &display_size, float min_dim);
  void make_board_buttons(const ImVec2 &display_size, float min_dim);
  SceneType make_action_buttons(const ImVec2 &display_size, float min_dim);
};

// Two-player local game
class PVPScene : public GameScene {
public:
  PVPScene(unsigned board_dimension, unsigned in_a_row, unsigned int x_tex,
           unsigned int o_tex, ma_engine *audio, ma_sound *bgm,
           unsigned int music_tex);

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
           bool player_x, bool iddfs, unsigned int x_tex, unsigned int o_tex,
           ma_engine *audio, ma_sound *bgm, unsigned int music_tex);

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
