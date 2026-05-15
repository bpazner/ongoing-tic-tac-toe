#include "scene.hpp"

#include "miniaudio.h"
#include <algorithm>
#include <chrono>
#include <future>

// Plays click sound and returns true if the button was clicked
static constexpr const char *CLICK_SOUND = "../assets/audio/button.mp3";
static bool sound_button(ma_engine *audio, const char *label,
                         const ImVec2 &size = {0, 0}) {
  bool clicked = ImGui::Button(label, size);
  if (clicked && audio) {
    ma_engine_play_sound(audio, CLICK_SOUND, nullptr);
  }
  return clicked;
}

static constexpr const char *SLIDER_SOUND = "../assets/audio/slider.mp3";
static bool sound_slider(ma_engine *audio, const char *label, int *v, int v_min,
                         int v_max) {
  bool changed = ImGui::SliderInt(label, v, v_min, v_max);
  if (changed && audio) {
    ma_engine_play_sound(audio, SLIDER_SOUND, nullptr);
  }
  return changed;
}

//////////////////////////////////////////////////////////////
// SCENE (Base)
//////////////////////////////////////////////////////////////

void Scene::make_volume_control(float min_dim) {
  float pad = min_dim * 0.02f;
  float icon_size = min_dim * 0.04f;
  float slider_w = min_dim * 0.2f;

  // Music icon in top left
  ImGui::SetCursorPos({pad, pad});
  ImGui::Image((ImTextureID)(intptr_t)music_tex_, {icon_size, icon_size});

  // Set slider's knob width
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, slider_w / 16);

  // Slider vertically centered with icon
  ImGui::SameLine(0, min_dim * 0.01f);
  ImGui::SetCursorPos(
      {2 * pad + icon_size, pad + (icon_size - ImGui::GetFrameHeight()) / 2});

  // White knob on dark track, no value label
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 1));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.3f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.4f, 0.4f, 1));
  ImGui::SetNextItemWidth(slider_w);
  if (ImGui::SliderFloat("##volume", &volume_, 0.0f, 1.0f, "") && bgm_) {
    ma_sound_set_volume(bgm_, volume_ * BASE_BGM_VOLUME / 0.5f);
  }
  ImGui::PopStyleVar(1);
  ImGui::PopStyleColor(4);
}

//////////////////////////////////////////////////////////////
// MENU SCENE
//////////////////////////////////////////////////////////////

SceneType MenuScene::draw() {
  // Create menu window
  ImGui::SetNextWindowPos({0, 0});
  ImVec2 display_size = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowSize(display_size);
  ImGui::Begin("Menu", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

  float min_dim = std::min(display_size.x, display_size.y);

  // Music icon and volume slider in top left
  make_volume_control(min_dim);

  // Gamemode buttons
  make_gamemode_buttons(display_size, min_dim);

  // Board dimension slider
  make_dimension_slider(display_size, min_dim);

  // In-a-row slider
  make_in_a_row_slider(display_size, min_dim);

  // Bot-only options
  if (gamemode_ == Gamemode::PLAYER_VS_BOT) {
    make_difficulty_slider(display_size, min_dim);
    make_player_side_buttons(display_size, min_dim);
  }

  // Play and quit buttons centered side by side
  ImVec2 btn_size = {min_dim * 0.15f, min_dim * 0.07f};
  float btn_gap = min_dim * 0.02f;
  float btn_y = gamemode_ == Gamemode::PLAYER_VS_BOT ? 0.77f : 0.57f;
  float btn_y_px = display_size.y * btn_y;
  float btn_total_w = btn_size.x * 2 + btn_gap;
  float btn_x = (display_size.x - btn_total_w) / 2;

  SceneType next_scene = SceneType::NONE;

  ImGui::SetCursorPos({btn_x + btn_size.x + btn_gap, btn_y_px});
  if (sound_button(audio_, "Play", btn_size)) {
    next_scene = SceneType::GAME;
  }
  ImGui::SetCursorPos({btn_x, btn_y_px});
  if (sound_button(audio_, "Quit", btn_size)) {
    next_scene = SceneType::QUIT;
  }

  ImGui::End();
  return next_scene;
}

void MenuScene::make_gamemode_buttons(const ImVec2 &display_size,
                                      float min_dim) {
  ImVec2 mode_btn_size = {min_dim * 0.4f, min_dim * 0.08f};
  float mode_gap = min_dim * 0.01f;
  float mode_total_w = mode_btn_size.x * 2 + mode_gap;
  float mode_x = (display_size.x - mode_total_w) / 2;
  float mode_y = display_size.y * 0.25f;

  auto highlight = [](bool active) {
    if (active) {
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
  };
  auto pop = [](bool active) {
    if (active) {
      ImGui::PopStyleColor();
    }
  };

  bool pvp = gamemode_ == Gamemode::PLAYER_VS_PLAYER;
  bool pvb = gamemode_ == Gamemode::PLAYER_VS_BOT;

  // Player vs player button
  ImGui::SetCursorPos({mode_x, mode_y});
  highlight(pvp);
  if (sound_button(audio_, "Player vs Player", mode_btn_size)) {
    gamemode_ = Gamemode::PLAYER_VS_PLAYER;
  }
  pop(pvp);

  // Player vs bot button
  ImGui::SetCursorPos({mode_x + mode_btn_size.x + mode_gap, mode_y});
  highlight(pvb);
  if (sound_button(audio_, "Player vs Bot", mode_btn_size)) {
    gamemode_ = Gamemode::PLAYER_VS_BOT;
  }
  pop(pvb);
}

void MenuScene::make_dimension_slider(const ImVec2 &display_size,
                                      float min_dim) {
  // Make slider (min dimension - max dimension)
  float dim_slider_w = min_dim * 0.4f;
  float dim_slider_x = (display_size.x - dim_slider_w) / 2;
  float dim_label_w = ImGui::CalcTextSize("Board Dimension").x;
  ImGui::SetCursorPos(
      {(display_size.x - dim_label_w) / 2, display_size.y * 0.35f});
  ImGui::Text("Board Dimension");
  ImGui::SetCursorPosX(dim_slider_x);
  ImGui::SetNextItemWidth(dim_slider_w);
  sound_slider(audio_, "##board_dimension", &board_dimension_,
               MIN_BOARD_DIMENSION, MAX_BOARD_DIMENSION);
}

void MenuScene::make_in_a_row_slider(const ImVec2 &display_size,
                                     float min_dim) {
  // Clamp in-a-row to current dimension
  in_a_row_ = std::min(in_a_row_, board_dimension_);

  // Make slider (min target - current dimension)
  float target_slider_w = min_dim * 0.4f;
  float target_slider_x = (display_size.x - target_slider_w) / 2;
  float target_label_w = ImGui::CalcTextSize("Target In-a-row").x;
  ImGui::SetCursorPos(
      {(display_size.x - target_label_w) / 2, display_size.y * 0.45f});
  ImGui::Text("Target In-a-row");
  ImGui::SetCursorPosX(target_slider_x);
  ImGui::SetNextItemWidth(target_slider_w);
  sound_slider(audio_, "##in_a_row", &in_a_row_, MIN_TARGET, board_dimension_);
}

void MenuScene::make_difficulty_slider(const ImVec2 &display_size,
                                       float min_dim) {
  // Make slider (1 - 5), depth scales as difficulty * 2
  float slider_w = min_dim * 0.4f;
  float slider_x = (display_size.x - slider_w) / 2;
  float label_w = ImGui::CalcTextSize("Bot Difficulty").x;
  ImGui::SetCursorPos({(display_size.x - label_w) / 2, display_size.y * 0.55f});
  ImGui::Text("Bot Difficulty");
  ImGui::SetCursorPosX(slider_x);
  ImGui::SetNextItemWidth(slider_w);

  // Get difficulty and set depth
  sound_slider(audio_, "##bot_difficulty", &difficulty_, 1, 5);
  depth_ = std::round(3 * difficulty_ /
                      std::log(1.0 * board_dimension_ * board_dimension_));
}

void MenuScene::make_player_side_buttons(const ImVec2 &display_size,
                                         float min_dim) {
  // Three buttons: X, O, Random
  ImVec2 btn_size = {min_dim * 0.13f, min_dim * 0.06f};
  float gap = min_dim * 0.01f;
  float total_w = btn_size.x * 3 + gap * 2;
  float start_x = (display_size.x - total_w) / 2;
  float y = display_size.y * 0.67f;

  float label_w = ImGui::CalcTextSize("Play as").x;
  ImGui::SetCursorPos({(display_size.x - label_w) / 2, y - min_dim * 0.04f});
  ImGui::Text("Play as");

  auto highlight = [](bool active) {
    if (active) {
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
  };
  auto pop = [](bool active) {
    if (active) {
      ImGui::PopStyleColor();
    }
  };

  bool px = player_side_ == PlayerSide::X;
  bool po = player_side_ == PlayerSide::O;
  bool pr = player_side_ == PlayerSide::RANDOM;

  // X button
  ImGui::SetCursorPos({start_x, y});
  highlight(px);
  if (sound_button(audio_, "X", btn_size)) {
    player_side_ = PlayerSide::X;
  }
  pop(px);

  // O button
  ImGui::SetCursorPos({start_x + btn_size.x + gap, y});
  highlight(po);
  if (sound_button(audio_, "O", btn_size)) {
    player_side_ = PlayerSide::O;
  }
  pop(po);

  // Random button
  ImGui::SetCursorPos({start_x + (btn_size.x + gap) * 2, y});
  highlight(pr);
  if (sound_button(audio_, "Random", btn_size)) {
    player_side_ = PlayerSide::RANDOM;
  }
  pop(pr);
}

//////////////////////////////////////////////////////////////
// GAME SCENE (base)
//////////////////////////////////////////////////////////////

GameScene::GameScene(unsigned board_dimension, unsigned in_a_row,
                     unsigned int x_tex, unsigned int o_tex, ma_engine *audio,
                     ma_sound *bgm, unsigned int music_tex)
    : Scene(audio, bgm, music_tex), game_(board_dimension, in_a_row),
      x_tex_(x_tex), o_tex_(o_tex) {}

SceneType GameScene::draw() {
  // Create window
  ImGui::SetNextWindowPos({0, 0});
  ImVec2 display_size = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowSize(display_size);
  ImGui::Begin("Game", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

  // Subclass hook (e.g. bot move logic)
  pre_draw();

  float min_dim = std::min(display_size.x, display_size.y);

  make_volume_control(min_dim);
  make_header_labels(display_size, min_dim);
  make_board_buttons(display_size, min_dim);

  SceneType next = make_action_buttons(display_size, min_dim);
  if (next != SceneType::NONE) {
    ImGui::End();
    return next;
  }

  ImGui::End();
  return SceneType::NONE;
}

// Renders a label at (x, y), coloring X red and O green
static void render_label_colored(const std::string &label, float x, float y) {
  constexpr ImVec4 red = {1.0f, 0.3f, 0.3f, 1.0f};
  constexpr ImVec4 green = {0.3f, 1.0f, 0.3f, 1.0f};

  bool first = true;
  size_t start = 0;

  // Position first segment at (x, y), chain the rest with SameLine
  auto emit = [&](const char *text, const ImVec4 *color) {
    if (first) {
      ImGui::SetCursorPos({x, y});
      first = false;
    } else {
      ImGui::SameLine(0, 0);
    }
    if (color) {
      ImGui::TextColored(*color, "%s", text);
    } else {
      ImGui::Text("%s", text);
    }
  };

  // Walk the string, flushing plain segments before each colored character
  for (size_t i = 0; i <= label.size(); i++) {
    char c = i < label.size() ? label[i] : '\0';
    if (c == PLAYER_X || c == PLAYER_O || i == label.size()) {
      if (i > start) {
        emit(label.substr(start, i - start).c_str(), nullptr);
      }
      if (c == PLAYER_X) {
        emit("X", &red);
        start = i + 1;
      } else if (c == PLAYER_O) {
        emit("O", &green);
        start = i + 1;
      }
    }
  }
}

void GameScene::make_header_labels(const ImVec2 &display_size, float min_dim) {
  float label_y = display_size.y * 0.1f;
  constexpr float score_scale = 2.0f;

  // Centered, above the game label, "Target - <in_a_row> in a row"
  std::string target_label =
      "Target - " + std::to_string(game_.get_in_a_row()) + " in a row";
  float target_label_w = ImGui::CalcTextSize(target_label.c_str()).x;
  ImGui::SetWindowFontScale(1.0f);
  render_label_colored(target_label, (display_size.x - target_label_w) / 2,
                       label_y - display_size.y * 0.05f);

  // X score on the left, scaled up
  ImGui::SetWindowFontScale(score_scale);
  std::string x_score_str = "X: " + std::to_string(game_.get_x_score());
  render_label_colored(x_score_str, display_size.x * 0.15f,
                       label_y + display_size.y * 0.05f);

  // Game label in the center (normal scale), X red and O green
  ImGui::SetWindowFontScale(1.0f);
  std::string game_label = get_game_label();
  std::string game_label_base = game_label;
  while (!game_label_base.empty() && game_label_base.back() == '.') {
    game_label_base.pop_back();
  } // Ignore "..." when positioning label
  float game_label_w = ImGui::CalcTextSize(game_label_base.c_str()).x;
  render_label_colored(game_label, (display_size.x - game_label_w) / 2,
                       label_y);

  // O score on the right, scaled up
  ImGui::SetWindowFontScale(score_scale);
  std::string o_score_str = "O: " + std::to_string(game_.get_o_score());
  float o_score_w = ImGui::CalcTextSize(o_score_str.c_str()).x;
  render_label_colored(o_score_str, display_size.x * 0.85f - o_score_w,
                       label_y + display_size.y * 0.05f);

  ImGui::SetWindowFontScale(1.0f);
}

SceneType GameScene::make_action_buttons(const ImVec2 &display_size,
                                         float min_dim) {
  // Play again and return to menu buttons
  ImVec2 btn_size = {min_dim * 0.3f, min_dim * 0.08f};
  float btn_gap = min_dim * 0.02f;
  float btn_total_w = btn_size.x * 2 + btn_gap;
  float btn_x = (display_size.x - btn_total_w) / 2;
  float btn_y = display_size.y * 0.85f;

  ImGui::SetCursorPos({btn_x, btn_y});
  if (sound_button(audio_, "Play Again", btn_size)) {
    on_reset();
    game_.reset();
  }

  ImGui::SetCursorPos({btn_x + btn_size.x + btn_gap, btn_y});
  if (sound_button(audio_, "Return to Menu", btn_size)) {
    return SceneType::MENU;
  }

  return SceneType::NONE;
}

void GameScene::make_board_buttons(const ImVec2 &display_size, float min_dim) {
  unsigned dim = game_.get_board_dimension();

  float board_size = min_dim * 0.6f;
  float gap_to_cell_ratio = 0.1f;
  float cell_size = board_size / (dim + (dim - 1) * gap_to_cell_ratio);
  float board_x = (display_size.x - board_size) / 2;
  float board_y = (display_size.y - board_size) / 2;

  float img_padding = cell_size * 0.1f;
  float img_size = cell_size - (2 * img_padding);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {img_padding, img_padding});

  // Create buttons
  float step = cell_size * (1 + gap_to_cell_ratio);
  for (unsigned r = 0; r < dim; r++) {
    for (unsigned c = 0; c < dim; c++) {
      ImGui::SetCursorPos({board_x + c * step, board_y + r * step});
      ImGui::PushID(r * dim + c);
      char cell = game_.get_cell(r, c);
      if (cell == EMPTY) {
        if (ImGui::Button("", {cell_size, cell_size}) && can_move()) {
          game_.make_move(r, c);
        }
      } else {
        ImTextureID tex =
            (ImTextureID)(intptr_t)(cell == PLAYER_X ? x_tex_ : o_tex_);
        ImGui::ImageButton("img", tex, {img_size, img_size});
      }
      ImGui::PopID();
    }
  }

  ImGui::PopStyleVar();
}

//////////////////////////////////////////////////////////////
// PVP SCENE
//////////////////////////////////////////////////////////////

PVPScene::PVPScene(unsigned board_dimension, unsigned in_a_row,
                   unsigned int x_tex, unsigned int o_tex, ma_engine *audio,
                   ma_sound *bgm, unsigned int music_tex)
    : GameScene(board_dimension, in_a_row, x_tex, o_tex, audio, bgm,
                music_tex) {}

std::string PVPScene::get_game_label() const {
  Result result = game_.get_result();
  if (result == Result::X_WIN) {
    return "X wins!";
  } else if (result == Result::O_WIN) {
    return "O wins!";
  } else if (result == Result::TIE) {
    return "Tie...";
  } else {
    return game_.get_x_turn() ? "Player X's turn" : "Player O's turn";
  }
}

//////////////////////////////////////////////////////////////
// PVB SCENE
//////////////////////////////////////////////////////////////

PVBScene::PVBScene(unsigned board_dimension, unsigned in_a_row, unsigned depth,
                   bool player_x, bool iddfs, unsigned int x_tex,
                   unsigned int o_tex, ma_engine *audio, ma_sound *bgm,
                   unsigned int music_tex)
    : GameScene(board_dimension, in_a_row, x_tex, o_tex, audio, bgm, music_tex),
      depth_(depth), player_x_(player_x) {
  bot_ = std::make_unique<Bot>(board_dimension, in_a_row, depth, iddfs);
}

void PVBScene::pre_draw() {
  if (game_.get_result() != Result::NOT_OVER) {
    return;
  }

  if (game_.get_x_turn() != player_x_) {
    if (!bot_thinking_) {
      // Launch bot computation on a background thread
      char bot_player = player_x_ ? PLAYER_O : PLAYER_X;
      auto board = game_.get_board();
      bot_future_ = std::async(std::launch::async, [this, bot_player, board] {
        return bot_->get_move(board, bot_player, false);
      });
      bot_thinking_ = true;
    } else if (bot_future_.wait_for(std::chrono::seconds(0)) ==
               std::future_status::ready) {
      // Bot finished, apply move
      game_.make_move(bot_future_.get());
      bot_thinking_ = false;
    }
  }
}

std::string PVBScene::get_game_label() const {
  Result result = game_.get_result();
  if (result == Result::X_WIN) {
    std::string label = player_x_ ? "Player" : "Bot";
    return label + "/X wins!";
  } else if (result == Result::O_WIN) {
    std::string label = player_x_ ? "Bot" : "Player";
    return label + "/O wins!";
  } else if (result == Result::TIE) {
    return "Tie...";
  } else if (bot_thinking_) {
    int dots = (int)(ImGui::GetTime() * 4) % 4;
    return "Bot is thinking" + std::string(dots, '.');
  } else {
    char char_turn = game_.get_x_turn() ? PLAYER_X : PLAYER_O;
    return "Player/" + std::string(1, char_turn) + "'s turn";
  }
}
