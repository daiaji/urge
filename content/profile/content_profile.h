// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PROFILE_CONTENT_PROFILE_H_
#define CONTENT_PROFILE_CONTENT_PROFILE_H_

#include <string>

#include "SDL3/SDL_iostream.h"

#include "base/math/vector.h"

namespace content {

class ContentProfile {
 public:
  enum class APIVersion {
    UNKNOWN = 0,
    RGSS1,
    RGSS2,
    RGSS3,
  };

  ContentProfile(const std::string& app, SDL_IOStream* stream);
  ~ContentProfile();

  ContentProfile(const ContentProfile&) = delete;
  ContentProfile& operator=(const ContentProfile&) = delete;

  void LoadCommandLine(int32_t argc, char** argv);
  bool LoadConfigure(const std::string& app);
  void SaveConfigure();
  void MarkDirty() { dirty_ = true; }
  void SaveIfDirty() {
    if (dirty_) {
      SaveConfigure();
      dirty_ = false;
    }
  }
  void ResetAudioDefaults();
  void ResetRendererDefaults();

  // App
  std::vector<std::string> args;
  std::string program_name;

  // Game
  std::string window_title = "URGE Widget";
  std::string script_path = "Data/Scripts.rxdata";

  // Commandline
  bool game_debug = false;
  bool game_battle_test = false;

  // Engine
  APIVersion api_version = APIVersion::UNKNOWN;
  std::string default_font_path = "Fonts/Default.ttf";
  std::string i18n_xml_path;
  base::Vec2i window_size;
  base::Vec2i resolution;

  // GUI
  bool disable_settings = false;
  bool disable_fps_monitor = false;
  bool disable_reset = false;

  // Audio
  float audio_volume = 1.0f;
  std::string midi_soundfont;

  // Renderer
  std::string driver_backend;
  int32_t pipeline_default_sampler = 0;
  bool render_validation =
#if DILIGENT_DEVELOPMENT
      true;
#else
      false;
#endif
  bool u32_draw_index = true;
  int32_t frame_rate = 60;
  uint32_t vsync = 1;
  bool keep_ratio = true;
  bool fullscreen = false;
  bool allow_skip_frame = true;
  bool background_running = true;
  bool smooth_scale_present = false;
  int32_t smooth_scaling = 0;        // 0=Nearest, 1=Bilinear (up-scale)
  int32_t smooth_scaling_down = 0;   // 0=Nearest, 1=Bilinear (down-scale)
  bool integer_scaling = false;      // Integer multiple scaling
  int32_t scaling_mode = 0;          // Scaling algorithm for post-process
                                       // 0=Bilinear, 1=Nearest, 2=Lanczos3, 3=Bicubic
                                       // 4=Anime4K, 5=Anime4K+Sobel, 6=Anime4K Denoise L
                                       // 7=CuNNy-4x16-NVL, 8=CuNNy-4x24-NVL
  float scaling_ar_strength = 0.5f;  // Anti-ringing strength (Lanczos3)
  float scaling_bicubic_b = 0.33f;   // Bicubic B (Mitchell-Netravali)
  float scaling_bicubic_c = 0.33f;   // Bicubic C (Mitchell-Netravali)
  bool cas_enabled = false;          // CAS contrast adaptive sharpening
  float cas_sharpness = 0.4f;        // CAS sharpness (0-1)
  float scaling_sobel_strength = 1.0f; // Sobel adaptive strength
  float scaling_warp_strength = 0.0f;  // Thin warp strength (0=off)
  float scaling_darken_strength = 0.0f; // Line darken strength (0=off)
  bool sync_to_refresh_rate = false;
  bool win_resizable = true;
  bool fixed_aspect_ratio = true;
  bool udl_auto_fit = false;     // UDL: auto-fit window to 2x scale

  // Log
  bool save_log = true;

  // Log
  bool save_log = true;

  // Font
  float font_scale = 0.9f;
  bool font_kerning = true;
  int32_t font_hinting = 3;
  bool font_outline_crop = true;
  std::vector<std::string> font_subs;

  // Platform
  bool debugging_console = false;
  bool disable_ime = false;
  std::string orientation = "LandscapeLeft LandscapeRight";

 private:
  SDL_IOStream* ini_stream_;
  std::string ini_path_;
  bool dirty_ = false;
};

}  // namespace content

#endif  //! CONTENT_PROFILE_CONTENT_PROFILE_H_
