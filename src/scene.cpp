#include "scene.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <future>

#include "miniaudio.h"

//////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
//////////////////////////////////////////////////////////////

static void play_sfx(ma_sound* sound, float volume) {
  if (!sound) {
    return;
  }
  ma_sound_seek_to_pcm_frame(sound, 0);
  ma_sound_set_volume(sound, volume * BASE_SFX_VOLUME / 0.5f);
  ma_sound_start(sound);
}

static bool sound_button(ma_sound* sound, float sfx_volume, const char* label,
                         const ImVec2& size = {0, 0}) {
  bool clicked = ImGui::Button(label, size);
  if (clicked && sound) {
    play_sfx(sound, sfx_volume);
  }
  return clicked;
}

static bool sound_slider(AudioContext* ctx, const char* label, int* v,
                         int v_min, int v_max) {
  bool changed = ImGui::SliderInt(label, v, v_min, v_max);
  if (changed && ctx) {
    play_sfx(ctx->slider, ctx->sfx_volume);
  }
  return changed;
}

// Renders a label at (x, y), coloring X red and O green
static void render_label_colored(const std::string& label, float x, float y) {
  constexpr ImVec4 red = {1.0f, 0.3f, 0.3f, 1.0f};
  constexpr ImVec4 green = {0.3f, 1.0f, 0.3f, 1.0f};

  bool first = true;
  size_t start = 0;

  // Position first segment at (x, y), chain the rest with SameLine
  auto emit = [&](const char* text, const ImVec4* color) {
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

    bool is_player = (c == PLAYER_X || c == PLAYER_O);
    bool prev_alpha = i > 0 && std::isalpha((unsigned char)label[i - 1]);
    bool next_alpha =
        (i + 1 < label.size()) && std::isalpha((unsigned char)label[i + 1]);
    bool standalone = is_player && !prev_alpha && !next_alpha;

    if (standalone || i == label.size()) {
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

//////////////////////////////////////////////////////////////
// SCENE (Base)
//////////////////////////////////////////////////////////////

void Scene::make_volume_control(float min_dim) {
  // Element geometry
  float pad = min_dim * 0.02f;
  float icon_size = min_dim * 0.04f;
  float slider_w = min_dim * 0.2f;
  float row_h = icon_size + pad * 0.5f;
  float slider_x = 2 * pad + icon_size;
  float center_y_off = (icon_size - ImGui::GetFrameHeight()) / 2;

  // Style sliders
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, min_dim * 0.0125f);
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 1));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.3f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.4f, 0.4f, 1));

  // BGM - music icon & slider
  ImGui::SetCursorPos({pad, pad});
  ImGui::Image((ImTextureID)(intptr_t)tex_ctx_->music_tex,
               {icon_size, icon_size});
  ImGui::SetCursorPos({slider_x, pad + center_y_off});
  ImGui::SetNextItemWidth(slider_w);
  if (audio_ctx_ &&
      ImGui::SliderFloat("##bgm_volume", &audio_ctx_->bgm_volume, 0.0f, 1.0f,
                         "") &&
      audio_ctx_->bgm) {
    ma_sound_set_volume(audio_ctx_->bgm,
                        audio_ctx_->bgm_volume * BASE_BGM_VOLUME / 0.5f);
  }

  // SFX - icon & slider
  ImGui::SetCursorPos({pad, pad + row_h});
  ImGui::Image((ImTextureID)(intptr_t)tex_ctx_->sfx_tex,
               {icon_size, icon_size});
  ImGui::SetCursorPos({slider_x, pad + row_h + center_y_off});
  ImGui::SetNextItemWidth(slider_w);
  if (audio_ctx_) {
    ImGui::SliderFloat("##sfx_volume", &audio_ctx_->sfx_volume, 0.0f, 1.0f, "");
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
  make_gamemode_dropdown(display_size, min_dim);

  // Board dimension slider
  make_dimension_slider(display_size, min_dim);

  // In-a-row slider
  make_in_a_row_slider(display_size, min_dim);

  // Online multiplayer server input
  if (gamemode_ == Gamemode::ONLINE_MULTIPLAYER) {
    make_server_input(display_size, min_dim);
  }

  // Bot-only options
  if (gamemode_ == Gamemode::AGAINST_BOT || gamemode_ == Gamemode::BOT_VS_BOT) {
    // First slider (guaranteed bot)
    make_difficulty_slider(display_size, min_dim, true);
    if (gamemode_ == Gamemode::BOT_VS_BOT) {
      // Second slider (only for bot vs bot)
      make_difficulty_slider(display_size, min_dim, false);
    }
    if (gamemode_ == Gamemode::AGAINST_BOT) {
      make_player_side_buttons(display_size, min_dim);
    }
  }

  // Play and quit buttons centered side by side
  ImVec2 btn_size = {min_dim * 0.15f, min_dim * 0.07f};
  float btn_gap = min_dim * 0.02f;

  float btn_y = 0.57f;
  if (gamemode_ == Gamemode::ONLINE_MULTIPLAYER) {
    btn_y = 0.67f;
  } else if (gamemode_ == Gamemode::AGAINST_BOT ||
             gamemode_ == Gamemode::BOT_VS_BOT) {
    btn_y = 0.77f;
  }

  float btn_y_px = display_size.y * btn_y;
  float btn_total_w = btn_size.x * 2 + btn_gap;
  float btn_x = (display_size.x - btn_total_w) / 2;

  SceneType next_scene = SceneType::NONE;

  ImGui::SetCursorPos({btn_x + btn_size.x + btn_gap, btn_y_px});
  if (sound_button(audio_ctx_->button, audio_ctx_->sfx_volume, "Play",
                   btn_size)) {
    next_scene = SceneType::GAME;
  }
  ImGui::SetCursorPos({btn_x, btn_y_px});
  if (sound_button(audio_ctx_->button, audio_ctx_->sfx_volume, "Quit",
                   btn_size)) {
    next_scene = SceneType::QUIT;
  }

  ImGui::End();
  return next_scene;
}

void MenuScene::make_gamemode_dropdown(const ImVec2& display_size,
                                       float min_dim) {
  static const char* items[] = {"Local Multiplayer", "Online Multiplayer",
                                "Against Bot", "Bot vs Bot"};
  const char* preview = items[(int)gamemode_];

  float combo_w = min_dim * 0.5f;
  float combo_h = min_dim * 0.08f;
  float combo_y = display_size.y * 0.25f;
  ImGui::SetCursorPos({(display_size.x - combo_w) / 2, combo_y});
  ImGui::SetNextItemWidth(combo_w);

  // Fixed vertical padding for consistent box height
  float pad_y = (combo_h - ImGui::GetTextLineHeight()) / 2;
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, pad_y});

  // Pass empty preview, draw centered text manually after
  bool open = ImGui::BeginCombo("##gamemode", "");
  ImGui::PopStyleVar();

  // Draw centered preview text
  float arrow_w = ImGui::GetFrameHeight();
  float text_w = ImGui::CalcTextSize(preview).x;
  ImVec2 text_pos = {
      (display_size.x - combo_w) / 2 + (combo_w - arrow_w - text_w) / 2,
      combo_y + (combo_h - ImGui::GetTextLineHeight()) / 2};
  ImGui::GetForegroundDrawList()->AddText(
      text_pos, ImGui::GetColorU32(ImGuiCol_Text), preview);

  // Draw dropdown options (not centered)
  if (open) {
    for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
      if (ImGui::Selectable(items[i], (int)gamemode_ == i)) {
        gamemode_ = (Gamemode)i;
        // Play sound on selection
        play_sfx(audio_ctx_->button, audio_ctx_->sfx_volume);
      }
    }
    ImGui::EndCombo();
  }
}

void MenuScene::make_dimension_slider(const ImVec2& display_size,
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
  sound_slider(audio_ctx_, "##board_dimension", &board_dimension_,
               MIN_BOARD_DIMENSION, MAX_BOARD_DIMENSION);
}

void MenuScene::make_in_a_row_slider(const ImVec2& display_size,
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
  sound_slider(audio_ctx_, "##in_a_row", &in_a_row_, MIN_TARGET,
               board_dimension_);
}

void MenuScene::make_server_input(const ImVec2& display_size, float min_dim) {
  float input_w = min_dim * 0.4f;
  float input_x = (display_size.x - input_w) / 2;
  float label_w = ImGui::CalcTextSize("Server Address").x;
  float pad = min_dim * 0.008f;

  ImGui::SetCursorPos({(display_size.x - label_w) / 2, display_size.y * 0.55f});
  ImGui::Text("Server Address");
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {pad, pad});
  ImGui::SetCursorPosX(input_x);
  ImGui::SetNextItemWidth(input_w);

  if (ImGui::InputText("##server_address", server_address_, 256)) {
    play_sfx(audio_ctx_->button, audio_ctx_->sfx_volume);
  }

  ImGui::PopStyleVar();
}

void MenuScene::make_difficulty_slider(const ImVec2& display_size,
                                       float min_dim, bool is_first_slider) {
  // Calculate label and y according to gamemode and slider number
  std::string label = "Bot Difficulty";
  float slider_y = display_size.y * 0.55f;
  if (gamemode_ == Gamemode::BOT_VS_BOT) {
    label = (is_first_slider ? "X-" : "O-") + label;
    slider_y = display_size.y * (is_first_slider ? 0.55f : 0.65f);
  }

  // Render slider label
  float label_w = ImGui::CalcTextSize(label.c_str()).x;
  render_label_colored(label, (display_size.x - label_w) / 2, slider_y);

  // Make slider (1 - 5)
  float slider_w = min_dim * 0.4f;
  float slider_x = (display_size.x - slider_w) / 2;
  ImGui::SetCursorPosX(slider_x);
  ImGui::SetNextItemWidth(slider_w);

  if (gamemode_ == Gamemode::AGAINST_BOT) {
    sound_slider(audio_ctx_, "##bot_difficulty", &difficulty1_, 1, 5);
    depth1_ = std::round(3 * difficulty1_ /
                         std::log(1.0 * board_dimension_ * board_dimension_));
  } else if (gamemode_ == Gamemode::BOT_VS_BOT) {
    if (is_first_slider) {
      sound_slider(audio_ctx_, "##bot1_difficulty", &difficulty1_, 1, 5);
      depth1_ = std::round(3 * difficulty1_ /
                           std::log(1.0 * board_dimension_ * board_dimension_));
    } else {
      sound_slider(audio_ctx_, "##bot2_difficulty", &difficulty2_, 1, 5);
      depth2_ = std::round(3 * difficulty2_ /
                           std::log(1.0 * board_dimension_ * board_dimension_));
    }
  }
}

void MenuScene::make_player_side_buttons(const ImVec2& display_size,
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
  if (sound_button(audio_ctx_->button, audio_ctx_->sfx_volume, "X", btn_size)) {
    player_side_ = PlayerSide::X;
  }
  pop(px);

  // O button
  ImGui::SetCursorPos({start_x + btn_size.x + gap, y});
  highlight(po);
  if (sound_button(audio_ctx_->button, audio_ctx_->sfx_volume, "O", btn_size)) {
    player_side_ = PlayerSide::O;
  }
  pop(po);

  // Random button
  ImGui::SetCursorPos({start_x + (btn_size.x + gap) * 2, y});
  highlight(pr);
  if (sound_button(audio_ctx_->button, audio_ctx_->sfx_volume, "Random",
                   btn_size)) {
    player_side_ = PlayerSide::RANDOM;
  }
  pop(pr);
}

//////////////////////////////////////////////////////////////
// GAME SCENE (base)
//////////////////////////////////////////////////////////////

GameScene::GameScene(unsigned board_dimension, unsigned in_a_row,
                     AudioContext* audio_ctx, TextureContext* tex_ctx)
    : Scene(audio_ctx, tex_ctx),
      game_(board_dimension, in_a_row),
      tint_start_times_(board_dimension,
                        std::vector<std::vector<double>>(board_dimension)) {}

void GameScene::play_game_end_sfx() {
  if (game_.get_result() == Result::X_WIN ||
      game_.get_result() == Result::O_WIN) {
    play_sfx(audio_ctx_->game_end_win, audio_ctx_->sfx_volume);
  } else if (game_.get_result() == Result::TIE) {
    play_sfx(audio_ctx_->game_end_tie, audio_ctx_->sfx_volume);
  }
}

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
  draw_bot_speed_slider(display_size, min_dim);  // Only for bot game modes
  make_board_buttons(display_size, min_dim);

  SceneType next = make_action_buttons(display_size, min_dim);
  if (next != SceneType::NONE) {
    ImGui::End();
    return next;
  }

  ImGui::End();
  return SceneType::NONE;
}

void GameScene::make_header_labels(const ImVec2& display_size, float min_dim) {
  float label_y = display_size.y * 0.1f;
  constexpr float score_scale = 2.0f;

  // Centered, above the game label, "Target - <in_a_row> in a row"
  std::string target_label =
      "Target - " + std::to_string(game_.get_in_a_row()) + " in a row";
  float target_label_w = ImGui::CalcTextSize(target_label.c_str()).x;
  ImGui::SetWindowFontScale(1.0f);
  render_label_colored(target_label, (display_size.x - target_label_w) / 2,
                       label_y - display_size.y * 0.05f);

  // Centered, game label (normal scale), X red and O green
  ImGui::SetWindowFontScale(1.0f);
  std::string game_label = get_game_label();
  std::string game_label_base = game_label;
  while (!game_label_base.empty() && game_label_base.back() == '.') {
    game_label_base.pop_back();
  }  // Ignore "..." when positioning label
  float game_label_w = ImGui::CalcTextSize(game_label_base.c_str()).x;
  render_label_colored(game_label, (display_size.x - game_label_w) / 2,
                       label_y);

  // X score on the left, scaled up
  ImGui::SetWindowFontScale(score_scale);
  std::string x_score_str = "X: " + std::to_string(game_.get_x_score());
  render_label_colored(x_score_str, display_size.x * 0.15f,
                       label_y + display_size.y * 0.05f);

  // O score on the right, scaled up
  std::string o_score_str = "O: " + std::to_string(game_.get_o_score());
  float o_score_w = ImGui::CalcTextSize(o_score_str.c_str()).x;
  render_label_colored(o_score_str, display_size.x * 0.85f - o_score_w,
                       label_y + display_size.y * 0.05f);
  ImGui::SetWindowFontScale(1.0f);
}

SceneType GameScene::make_action_buttons(const ImVec2& display_size,
                                         float min_dim) {
  // Play again and return to menu buttons
  ImVec2 btn_size = {min_dim * 0.3f, min_dim * 0.08f};
  float btn_gap = min_dim * 0.02f;
  float btn_total_w = btn_size.x * 2 + btn_gap;
  float btn_x = (display_size.x - btn_total_w) / 2;
  float btn_y = display_size.y * 0.85f;

  ImGui::SetCursorPos({btn_x, btn_y});
  if (sound_button(audio_ctx_->button, audio_ctx_->sfx_volume, "Play Again",
                   btn_size)) {
    on_reset();
    game_.reset();

    // Clear tint time entries
    for (unsigned r = 0; r < game_.get_board_dimension(); r++) {
      for (unsigned c = 0; c < game_.get_board_dimension(); c++) {
        tint_start_times_[r][c].clear();
      }
    }
  }

  ImGui::SetCursorPos({btn_x + btn_size.x + btn_gap, btn_y});
  if (sound_button(audio_ctx_->button, audio_ctx_->sfx_volume, "Return to Menu",
                   btn_size)) {
    return SceneType::MENU;
  }

  return SceneType::NONE;
}

void GameScene::make_board_buttons(const ImVec2& display_size, float min_dim) {
  unsigned dim = game_.get_board_dimension();

  float board_size = min_dim * 0.6f;
  float gap_to_cell_ratio = 0.1f;
  float cell_size = board_size / (dim + (dim - 1) * gap_to_cell_ratio);
  float board_x = (display_size.x - board_size) / 2;
  float board_y = (display_size.y - board_size) / 2;

  // Create buttons
  float step = cell_size * (1 + gap_to_cell_ratio);
  for (unsigned r = 0; r < dim; r++) {
    for (unsigned c = 0; c < dim; c++) {
      ImGui::SetCursorPos({board_x + c * step, board_y + r * step});
      ImGui::PushID(r * dim + c);
      char cell = game_.get_cell(r, c);
      if (cell == EMPTY) {
        if (ImGui::Button("", {cell_size, cell_size}) && can_move()) {
          play_sfx(audio_ctx_->tile, audio_ctx_->sfx_volume);
          make_move_wrapper(r, c);
        }
      } else {
        GameScene::CellEmphasis emphasis = get_cell_emphasis(r, c);
        float img_size = cell_size * emphasis.size_factor;
        float img_padding = cell_size * (1 - emphasis.size_factor) / 2;

        ImTextureID tex =
            (ImTextureID)(intptr_t)(cell == PLAYER_X ? tex_ctx_->x_tex
                                                     : tex_ctx_->o_tex);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            {img_padding, img_padding});
        ImGui::ImageButton("img", tex, {img_size, img_size}, {0, 0}, {1, 1},
                           {0, 0, 0, 0}, emphasis.tint);
        ImGui::PopStyleVar();
      }
      ImGui::PopID();
    }
  }
}

static constexpr double TINT_DELTA_TIME = 0.25;  // seconds between tile tints
static constexpr double TINT_DURATION_PER_TILE =
    TINT_DELTA_TIME * 2;  // no tint -> full tint (halfway) -> no tint
static constexpr float BASE_ICON_IMG_SIZE =
    0.8f;  // Icon images are scaled to this fraction of cell size;

void GameScene::make_move_wrapper(unsigned r, unsigned c) {
  bool is_scoring = game_.make_move(r, c);

  // Get in a row locations of scoring move, and update tint times
  if (is_scoring) {
    auto locs = game_.get_in_a_row_locs(r, c);
    double now = ImGui::GetTime();
    for (unsigned i = 0; i < locs.size(); i++) {
      std::pair<unsigned, unsigned> loc = locs[i];
      tint_start_times_[loc.first][loc.second].push_back(now +
                                                         i * TINT_DELTA_TIME);
    }
  }

  // Check if game ended, play sound
  if (game_.get_result() != Result::NOT_OVER) {
    play_game_end_sfx();
  }
}

GameScene::CellEmphasis GameScene::get_cell_emphasis(unsigned r, unsigned c) {
  double now = ImGui::GetTime();

  // Iterate through start times for cell
  for (double tint_start : tint_start_times_[r][c]) {
    // Check if tile's tint is in progress
    if (tint_start <= now && now <= tint_start + TINT_DURATION_PER_TILE) {
      double tile_delta_time = now - tint_start;
      double tint_strength =
          std::sin(M_PI * tile_delta_time / TINT_DURATION_PER_TILE);  // [0,1]
      double size_factor =
          BASE_ICON_IMG_SIZE + 0.8 * tint_strength * (1 - BASE_ICON_IMG_SIZE);

      // Accentuate X with red tint, O with green tint
      double tint_val = 1 - tint_strength;
      if (game_.get_cell(r, c) == PLAYER_X) {
        return {ImVec4(1, tint_val, tint_val, 1), size_factor};
      } else if (game_.get_cell(r, c) == PLAYER_O) {
        return {ImVec4(tint_val, 1, tint_val, 1), size_factor};
      }
    }
  }

  return {ImVec4(1, 1, 1, 1), BASE_ICON_IMG_SIZE};
}

//////////////////////////////////////////////////////////////
// PVP LOCAL SCENE
//////////////////////////////////////////////////////////////

PVPLocalScene::PVPLocalScene(unsigned board_dimension, unsigned in_a_row,
                             AudioContext* audio_ctx, TextureContext* tex_ctx)
    : GameScene(board_dimension, in_a_row, audio_ctx, tex_ctx) {}

std::string PVPLocalScene::get_game_label() const {
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
// PVP ONLINE SCENE
//////////////////////////////////////////////////////////////

PVPOnlineScene::PVPOnlineScene(const char* server_address,
                               unsigned board_dimension, unsigned in_a_row,
                               AudioContext* audio_ctx, TextureContext* tex_ctx)
    : GameScene(board_dimension, in_a_row, audio_ctx, tex_ctx),
      server_address_(server_address) {
  // Parse host and port from "host:port"
  std::string addr_str(server_address);
  std::string host = addr_str;
  int port = 12345;
  size_t colon = addr_str.rfind(':');
  if (colon != std::string::npos) {
    host = addr_str.substr(0, colon);
    port = std::stoi(addr_str.substr(colon + 1));
  }

  // Resolve hostname and connect
  addrinfo hints{}, *res = nullptr;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) !=
      0) {
    fprintf(stderr, "Failed to resolve server address: %s\n", server_address);
    return;
  }
  sock_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sock_ < 0 || connect(sock_, res->ai_addr, res->ai_addrlen) < 0) {
    perror("connect");
    if (sock_ >= 0) {
      close(sock_);
      sock_ = -1;
    }
    freeaddrinfo(res);
    return;
  }
  freeaddrinfo(res);
  printf("Connected to %s:%d\n", host.c_str(), port);

  // Send game settings to server for matchmaking
  uint8_t settings[2] = {(uint8_t)board_dimension, (uint8_t)in_a_row};
  send(sock_, settings, sizeof(settings), 0);
}

PVPOnlineScene::~PVPOnlineScene() {
  // Disconnect from server
  if (sock_ >= 0) {
    close(sock_);
  }
}

void PVPOnlineScene::pre_draw() {
  if (sock_ < 0) {
    return;
  }

  if (!role_assigned_) {
    // Wait for server to send role byte ('X' or 'O')
    char role;
    ssize_t n = recv(sock_, &role, 1, MSG_DONTWAIT);
    if (n == 1) {
      player_x_ = (role == PLAYER_X);
      role_assigned_ = true;
      printf("Assigned role: %c\n", role);
    }
    return;
  }
}

std::string PVPOnlineScene::get_game_label() const {
  if (!role_assigned_) {
    int dots = (int)(ImGui::GetTime() * 4) % 4;
    return sock_ < 0 ? "Failed to connect"
                     : "Waiting for opponent" + std::string(dots, '.');
  }
  Result result = game_.get_result();
  if (result == Result::X_WIN) {
    return "X: " + std::string(player_x_ ? "You win!" : "Opponent wins...");
  } else if (result == Result::O_WIN) {
    return "O: " + std::string(player_x_ ? "Opponent wins..." : "You win!");
  } else if (result == Result::TIE) {
    return "Tie...";
  } else {
    char char_turn = game_.get_x_turn() ? PLAYER_X : PLAYER_O;
    if (game_.get_x_turn() == player_x_) {
      return std::string(1, char_turn) + ": Your turn";
    } else {
      return std::string(1, char_turn) + ": Opponent's turn";
    }
  }
}

//////////////////////////////////////////////////////////////
// PVB SCENE
//////////////////////////////////////////////////////////////

PVBScene::PVBScene(unsigned board_dimension, unsigned in_a_row, unsigned depth,
                   bool player_x, AudioContext* audio_ctx,
                   TextureContext* tex_ctx)
    : GameScene(board_dimension, in_a_row, audio_ctx, tex_ctx),
      depth_(depth),
      player_x_(player_x) {
  bot_ = std::make_shared<Bot>(board_dimension, in_a_row, depth);
}

PVBScene::~PVBScene() {
  if (bot_thinking_ && bot_future_.valid()) {
    // Detach so the destructor doesn't block the main thread
    std::thread([f = std::move(bot_future_)]() mutable { f.get(); }).detach();
  }
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
      bot_future_ =
          std::async(std::launch::async, [bot = bot_, bot_player, board] {
            return bot->get_move(board, bot_player, false);
          });
      bot_thinking_ = true;
      bot_move_start_time_ = ImGui::GetTime();
    } else if (bot_future_.wait_for(std::chrono::seconds(0)) ==
                   std::future_status::ready &&
               ImGui::GetTime() - bot_move_start_time_ > get_min_bot_time()) {
      // Bot finished, make move
      make_move_wrapper(bot_future_.get());
      bot_thinking_ = false;

      // Play tile sound
      play_sfx(audio_ctx_->tile, audio_ctx_->sfx_volume);
    }
  }
}

void PVBScene::play_game_end_sfx() {
  if (player_x_ && game_.get_result() == Result::X_WIN ||
      !player_x_ && game_.get_result() == Result::O_WIN) {
    play_sfx(audio_ctx_->game_end_win, audio_ctx_->sfx_volume);
  } else if (game_.get_result() == Result::TIE) {
    play_sfx(audio_ctx_->game_end_tie, audio_ctx_->sfx_volume);
  } else if (game_.get_result() != Result::NOT_OVER) {
    play_sfx(audio_ctx_->game_end_lose, audio_ctx_->sfx_volume);
  }
}

std::string PVBScene::get_game_label() const {
  Result result = game_.get_result();
  if (result == Result::X_WIN) {
    return "X: " + std::string(player_x_ ? "You win!" : "Bot wins...");
  } else if (result == Result::O_WIN) {
    return "O: " + std::string(player_x_ ? "Bot wins..." : "You win!");
  } else if (result == Result::TIE) {
    return "Tie...";
  } else {
    char char_turn = game_.get_x_turn() ? PLAYER_X : PLAYER_O;
    if (bot_thinking_) {
      int dots = (int)(ImGui::GetTime() * 4) % 4;
      return std::string(1, char_turn) + ": Bot is thinking" +
             std::string(dots, '.');
    } else {
      return std::string(1, char_turn) + ": Your turn";
    }
  }
}

//////////////////////////////////////////////////////////////
// BVB SCENE
//////////////////////////////////////////////////////////////

BVBScene::BVBScene(unsigned board_dimension, unsigned in_a_row,
                   unsigned x_depth, unsigned o_depth, AudioContext* audio_ctx,
                   TextureContext* tex_ctx)
    : GameScene(board_dimension, in_a_row, audio_ctx, tex_ctx) {
  x_bot_ = std::make_shared<Bot>(board_dimension, in_a_row, x_depth);
  o_bot_ = std::make_shared<Bot>(board_dimension, in_a_row, o_depth);
}

BVBScene::~BVBScene() {
  if (bot_thinking_ && bot_future_.valid()) {
    // Detach so the destructor doesn't block the main thread
    std::thread([f = std::move(bot_future_)]() mutable { f.get(); }).detach();
  }
}

void BVBScene::pre_draw() {
  if (game_.get_result() != Result::NOT_OVER) {
    return;
  }

  if (!bot_thinking_) {
    // Launch bot computation on a background thread
    auto board = game_.get_board();
    if (game_.get_x_turn()) {
      // x bot
      bot_future_ = std::async(std::launch::async, [bot = x_bot_, board] {
        return bot->get_move(board, PLAYER_X, false);
      });
    } else {
      // o bot
      bot_future_ = std::async(std::launch::async, [bot = o_bot_, board] {
        return bot->get_move(board, PLAYER_O, false);
      });
    }
    bot_thinking_ = true;
    bot_move_start_time_ = ImGui::GetTime();
  } else if (bot_future_.wait_for(std::chrono::seconds(0)) ==
                 std::future_status::ready &&
             ImGui::GetTime() - bot_move_start_time_ > get_min_bot_time()) {
    // Bot finished, make move
    make_move_wrapper(bot_future_.get());
    bot_thinking_ = false;

    // Play tile sound
    play_sfx(audio_ctx_->tile, audio_ctx_->sfx_volume);
  }
}

std::string BVBScene::get_game_label() const {
  Result result = game_.get_result();
  if (result == Result::X_WIN) {
    return "X wins!";
  } else if (result == Result::O_WIN) {
    return "O wins!";
  } else if (result == Result::TIE) {
    return "Tie...";
  } else {
    char char_turn = game_.get_x_turn() ? PLAYER_X : PLAYER_O;
    int dots = (int)(ImGui::GetTime() * 4) % 4;
    return std::string(1, char_turn) + ": Bot is thinking" +
           std::string(dots, '.');
  }
}

void BVBScene::draw_bot_speed_slider(const ImVec2& display_size,
                                     float min_dim) {
  // Draw speed icon
  float icon_size = min_dim * 0.025f;
  float slider_w = min_dim * 0.3f;
  float start_x = (display_size.x - slider_w) / 2 - icon_size;
  float y = display_size.y * 0.15f;
  ImGui::SetCursorPos({start_x, y});
  ImGui::Image((ImTextureID)(intptr_t)tex_ctx_->speed_tex,
               {icon_size, icon_size});

  // Style slider
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, min_dim * 0.0125f);
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 1));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.3f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.4f, 0.4f, 1));

  // Draw slider
  float center_y_off = (icon_size - ImGui::GetFrameHeight()) / 2;
  ImGui::SetCursorPos(
      {start_x + icon_size + min_dim * 0.01f, y + center_y_off});
  ImGui::SetNextItemWidth(slider_w);
  ImGui::SliderFloat("##bot_speed", &bot_speed_, 0.0f, 1.0f, "");
  ImGui::PopStyleVar(1);
  ImGui::PopStyleColor(4);
}
