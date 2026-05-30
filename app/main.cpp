#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <random>

#include "assets.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "miniaudio.h"
#include "scene.hpp"
#include "stb_image.h"
#include "utils.hpp"

// UI configuration
constexpr const char* TITLE = "Ongoing Tic-Tac-Toe";
constexpr float WINDOWED_FACTOR = 0.6f;
constexpr float BASE_FONT_SIZE = 160.0f;
constexpr float FONT_SIZE_RATIO = 0.03f / BASE_FONT_SIZE;
constexpr float FRAME_ROUNDING_RATIO = 0.005f;

// Registers audio data with the resource manager and initializes a sound
static bool load_sound_mem(ma_engine* engine, const char* name,
                           const void* data, ma_uint32 len, unsigned flags,
                           ma_sound* out) {
  ma_resource_manager* rm = ma_engine_get_resource_manager(engine);
  if (ma_resource_manager_register_encoded_data(rm, name, data, len) !=
      MA_SUCCESS) {
    fprintf(stderr, "Failed to register sound: %s\n", name);
    return false;
  }
  if (ma_sound_init_from_file(engine, name, flags, nullptr, nullptr, out) !=
      MA_SUCCESS) {
    fprintf(stderr, "Failed to init sound: %s\n", name);
    return false;
  }
  return true;
}

// Initializes the audio engine and loads all sounds from embedded data
static void init_audio(ma_engine* engine, ma_sound* bgm, ma_sound* button,
                       ma_sound* slider, ma_sound* tile, ma_sound* game_end_win,
                       ma_sound* game_end_lose, ma_sound* game_end_tie,
                       AudioContext& ctx) {
  // Initialize engine
  ma_engine_config cfg = ma_engine_config_init();
  cfg.periodSizeInMilliseconds = 10;
  if (ma_engine_init(&cfg, engine) != MA_SUCCESS) {
    fprintf(stderr,
            "Failed to initialize audio engine, continuing without audio\n");
    return;
  }
  ctx.engine = engine;

  // Load background music
  if (load_sound_mem(engine, "bgm", bossa_nova_background_mp3,
                     bossa_nova_background_mp3_len, MA_SOUND_FLAG_DECODE,
                     bgm)) {
    ma_sound_set_looping(bgm, MA_TRUE);
    ma_sound_set_volume(bgm, BASE_BGM_VOLUME);
    ma_sound_start(bgm);
    ctx.bgm = bgm;
  }

  // Load sound effects
  if (load_sound_mem(engine, "button", button_mp3, button_mp3_len,
                     MA_SOUND_FLAG_DECODE, button)) {
    ctx.button = button;
  }
  if (load_sound_mem(engine, "slider", slider_mp3, slider_mp3_len,
                     MA_SOUND_FLAG_DECODE, slider)) {
    ctx.slider = slider;
  }
  if (load_sound_mem(engine, "tile", tile_mp3, tile_mp3_len,
                     MA_SOUND_FLAG_DECODE, tile)) {
    ctx.tile = tile;
  }
  if (load_sound_mem(engine, "game_end_win", game_end_win_mp3,
                     game_end_win_mp3_len, MA_SOUND_FLAG_DECODE,
                     game_end_win)) {
    ctx.game_end_win = game_end_win;
  }
  if (load_sound_mem(engine, "game_end_lose", game_end_lose_mp3,
                     game_end_lose_mp3_len, MA_SOUND_FLAG_DECODE,
                     game_end_lose)) {
    ctx.game_end_lose = game_end_lose;
  }
  if (load_sound_mem(engine, "game_end_tie", game_end_tie_mp3,
                     game_end_tie_mp3_len, MA_SOUND_FLAG_DECODE,
                     game_end_tie)) {
    ctx.game_end_tie = game_end_tie;
  }
}

// Decodes a PNG from memory and uploads it as an OpenGL texture
static GLuint load_texture(const unsigned char* buf, unsigned int len) {
  int w, h, channels;
  unsigned char* data =
      stbi_load_from_memory(buf, (int)len, &w, &h, &channels, 4);
  GLuint tex = 0;
  if (!data) {
    fprintf(stderr, "Failed to load texture\n");
    return 0;
  }
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               data);
  stbi_image_free(data);
  return tex;
}

int main() {
  // Init GLFW
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  // Create window hidden
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  if (!monitor) {
    fprintf(stderr, "Failed to get primary monitor\n");
    glfwTerminate();
    return -1;
  }
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  if (!mode) {
    fprintf(stderr, "Failed to get video mode\n");
    glfwTerminate();
    return -1;
  }
  int win_w = (int)(mode->width * WINDOWED_FACTOR);
  int win_h = (int)(mode->height * WINDOWED_FACTOR);
  GLFWwindow* window = glfwCreateWindow(win_w, win_h, TITLE, nullptr, nullptr);
  if (!window) {
    fprintf(stderr,
            "Failed to create GLFW window (OpenGL 3.3 not supported?)\n");
    glfwTerminate();
    return -1;
  }

  // Center and show window
  int mx, my;
  glfwGetMonitorPos(monitor, &mx, &my);
  glfwSetWindowPos(window, mx + (mode->width - win_w) / 2,
                   my + (mode->height - win_h) / 2);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glfwShowWindow(window);

  // Init ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;

  // Init font, make sure it is large enough to prevent stretching
  ImFontConfig font_cfg;
  font_cfg.OversampleH = 1;
  font_cfg.OversampleV = 1;
  font_cfg.PixelSnapH = true;
  font_cfg.FontDataOwnedByAtlas = false;
  if (!io.Fonts->AddFontFromMemoryTTF(pixel_emulator_ttf,
                                      pixel_emulator_ttf_len, BASE_FONT_SIZE,
                                      &font_cfg)) {
    fprintf(stderr, "Failed to load font\n");
  }
  if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    fprintf(stderr, "Failed to initialize ImGui GLFW backend\n");
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return -1;
  }
  if (!ImGui_ImplOpenGL3_Init("#version 330")) {
    fprintf(stderr, "Failed to initialize ImGui OpenGL3 backend\n");
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return -1;
  }

  // Load icon textures
  TextureContext tex_ctx;
  tex_ctx.music_tex = load_texture(music_icon_png, music_icon_png_len);
  tex_ctx.sfx_tex = load_texture(sfx_icon_png, sfx_icon_png_len);
  tex_ctx.x_tex = load_texture(x_icon_png, x_icon_png_len);
  tex_ctx.o_tex = load_texture(o_icon_png, o_icon_png_len);
  tex_ctx.speed_tex = load_texture(speed_icon_png, speed_icon_png_len);

  // Init audio engine and sounds
  ma_engine audio;
  ma_sound bgm, button_snd, slider_snd, tile_snd, game_end_win_snd,
      game_end_lose_snd, game_end_tie_snd;
  AudioContext audio_ctx;
  init_audio(&audio, &bgm, &button_snd, &slider_snd, &tile_snd,
             &game_end_win_snd, &game_end_lose_snd, &game_end_tie_snd,
             audio_ctx);

  // Persistent menu, reused between games
  auto menu = std::make_shared<MenuScene>(&audio_ctx, &tex_ctx);
  std::shared_ptr<Scene> scene = menu;

  while (!glfwWindowShouldClose(window)) {
    // Handle input events
    glfwPollEvents();

    // Scale frame rounding and font size according to window
    int fb_w, fb_h;
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    float min_dim = std::min(fb_w * mode->height / mode->width, fb_h);
    if (mode->height > mode->width) {
      min_dim = std::min(fb_w, fb_h * mode->width / mode->height);
    }
    ImGui::GetStyle().FrameRounding = FRAME_ROUNDING_RATIO * min_dim;
    ImGui::GetIO().FontGlobalScale = FONT_SIZE_RATIO * min_dim;

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Draw current scene, handle transitions
    SceneType next = scene->draw();
    if (next == SceneType::GAME) {
      if (menu->get_gamemode() == Gamemode::LOCAL_MULTIPLAYER) {
        // Player vs player
        scene = std::make_shared<PVPLocalScene>(menu->get_board_dimension(),
                                                menu->get_in_a_row(),
                                                &audio_ctx, &tex_ctx);
      } else if (menu->get_gamemode() == Gamemode::AGAINST_BOT) {
        // Player vs bot
        bool player_x;
        // Check random player condition
        if (menu->get_player_side() == PlayerSide::RANDOM) {
          std::mt19937 rng{std::random_device{}()};
          player_x = std::uniform_int_distribution<int>(0, 1)(rng);
        } else {
          player_x = menu->get_player_side() == PlayerSide::X;
        }
        scene = std::make_shared<PVBScene>(
            menu->get_board_dimension(), menu->get_in_a_row(),
            menu->get_depth1(), player_x, &audio_ctx, &tex_ctx);
      } else if (menu->get_gamemode() == Gamemode::BOT_VS_BOT) {
        // Bot vs bot
        scene = std::make_shared<BVBScene>(
            menu->get_board_dimension(), menu->get_in_a_row(),
            menu->get_depth1(), menu->get_depth2(), &audio_ctx, &tex_ctx);
      }
    } else if (next == SceneType::MENU) {
      scene = menu;
    } else if (next == SceneType::QUIT) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // Render ImGui draw data onto cleared framebuffer
    ImGui::Render();
    glViewport(0, 0, fb_w, fb_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  // Cleanup
  if (audio_ctx.tile) {
    ma_sound_uninit(&tile_snd);
  }
  if (audio_ctx.button) {
    ma_sound_uninit(&button_snd);
  }
  if (audio_ctx.slider) {
    ma_sound_uninit(&slider_snd);
  }
  if (audio_ctx.bgm) {
    ma_sound_uninit(&bgm);
  }
  if (audio_ctx.engine) {
    ma_engine_uninit(&audio);
  }
  glDeleteTextures(1, &tex_ctx.music_tex);
  glDeleteTextures(1, &tex_ctx.sfx_tex);
  glDeleteTextures(1, &tex_ctx.x_tex);
  glDeleteTextures(1, &tex_ctx.o_tex);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
