// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/profile/content_profile.h"

#include "base/buildflags/build.h"

#include <cstdio>

#include "inih/INIReader.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace content {

namespace {

// https://stackoverflow.com/questions/28270310/how-to-easily-detect-utf8-encoding-in-the-string
bool CheckValidUTF8(const char* string) {
  if (!string)
    return true;

  const unsigned char* bytes = (const unsigned char*)string;
  unsigned int cp;
  int num;

  while (*bytes != 0x00) {
    if ((*bytes & 0x80) == 0x00) {
      // U+0000 to U+007F
      cp = (*bytes & 0x7F);
      num = 1;
    } else if ((*bytes & 0xE0) == 0xC0) {
      // U+0080 to U+07FF
      cp = (*bytes & 0x1F);
      num = 2;
    } else if ((*bytes & 0xF0) == 0xE0) {
      // U+0800 to U+FFFF
      cp = (*bytes & 0x0F);
      num = 3;
    } else if ((*bytes & 0xF8) == 0xF0) {
      // U+10000 to U+10FFFF
      cp = (*bytes & 0x07);
      num = 4;
    } else
      return false;

    bytes += 1;
    for (int i = 1; i < num; ++i) {
      if ((*bytes & 0xC0) != 0x80)
        return false;
      cp = (cp << 6) | (*bytes & 0x3F);
      bytes += 1;
    }

    if ((cp > 0x10FFFF) || ((cp >= 0xD800) && (cp <= 0xDFFF)) ||
        ((cp <= 0x007F) && (num != 1)) ||
        ((cp >= 0x0080) && (cp <= 0x07FF) && (num != 2)) ||
        ((cp >= 0x0800) && (cp <= 0xFFFF) && (num != 3)) ||
        ((cp >= 0x10000) && (cp <= 0x1FFFFF) && (num != 4)))
      return false;
  }

  return true;
}

void ReplaceStringWidth(std::string& str, char before, char after) {
  for (size_t i = 0; i < str.size(); ++i)
    if (str[i] == before)
      str[i] = after;
}

base::Vec2i GetVec2iFromString(std::string in,
                               const base::Vec2i& default_value) {
  size_t sep = in.find("|");
  if (sep != std::string::npos) {
    int32_t num1 = std::stoi(in.substr(0, sep).c_str());
    int32_t num2 = std::stoi(in.substr(sep + 1).c_str());
    return base::Vec2i(num1, num2);
  }

  return default_value;
}

char* IniStreamReader(char* str, int32_t num, void* stream) {
  SDL_IOStream* io = static_cast<SDL_IOStream*>(stream);

  memset(str, 0, num);
  char c;
  int32_t i = 0;

  while (i < num - 1 && SDL_ReadIO(io, &c, 1)) {
    str[i++] = c;
    if (c == '\n')
      break;
  }

  str[i] = '\0';
  return i ? str : nullptr;
}

#if defined(OS_WIN)
std::string ANSIFromUtf8(const std::string& asciiStr) {
  if (asciiStr.empty()) {
    return "";
  }

  // ASCII -> UTF-16
  int wcharsCount =
      MultiByteToWideChar(CP_ACP, 0, asciiStr.c_str(), -1, NULL, 0);
  if (wcharsCount == 0) {
    throw std::runtime_error("MultiByteToWideChar failed: " +
                             std::to_string(GetLastError()));
  }

  std::vector<wchar_t> wstr(wcharsCount);
  if (MultiByteToWideChar(CP_ACP, 0, asciiStr.c_str(), -1, &wstr[0],
                          wcharsCount) == 0) {
    throw std::runtime_error("MultiByteToWideChar failed: " +
                             std::to_string(GetLastError()));
  }

  // UTF-16 -> UTF-8
  int utf8Count =
      WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
  if (utf8Count == 0) {
    throw std::runtime_error("WideCharToMultiByte failed: " +
                             std::to_string(GetLastError()));
  }

  std::vector<char> utf8str(utf8Count);
  if (WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, &utf8str[0], utf8Count,
                          NULL, NULL) == 0) {
    throw std::runtime_error("WideCharToMultiByte failed: " +
                             std::to_string(GetLastError()));
  }

  return std::string(&utf8str[0]);
}
#endif

}  // namespace

ContentProfile::ContentProfile(const std::string& app, SDL_IOStream* stream)
    : program_name(app), ini_stream_(stream), ini_path_(app + ".ini") {}

ContentProfile::~ContentProfile() {
  SaveIfDirty();
}

void ContentProfile::LoadCommandLine(int32_t argc, char** argv) {
  for (int32_t i = 0; i < argc; ++i)
    args.push_back(argv[i]);

  for (int32_t i = 0; i < argc; i++) {
    if (std::string(argv[i]) == "test" || std::string(argv[i]) == "debug")
      game_debug = true;
    if (std::string(argv[i]) == "btest")
      game_battle_test = true;
  }
}

bool ContentProfile::LoadConfigure(const std::string& app) {
  std::unique_ptr<INIReader> reader = std::make_unique<INIReader>();
  if (ini_stream_) {
    reader = std::make_unique<INIReader>(ini_stream_, IniStreamReader);
    if (reader->ParseError())
      return false;
  } else {
    return false;
  }

  // Default
  script_path = reader->Get("Game", "Scripts", script_path);
  ReplaceStringWidth(script_path, '\\', '/');
  window_title = reader->Get("Game", "Title", window_title);
  if (!CheckValidUTF8(window_title.c_str())) {
#if defined(OS_WIN)
    window_title = ANSIFromUtf8(window_title);
#else
    window_title = "URGE Widget";
#endif
  }

  // Engine
  api_version = static_cast<APIVersion>(reader->GetInteger(
      "Engine", "APIVersion", static_cast<int32_t>(api_version)));
  if (api_version == APIVersion::UNKNOWN) {
    if (!script_path.empty()) {
      api_version = APIVersion::RGSS1;

      const char* p = &script_path[script_path.size()];
      const char* head = &script_path[0];

      while (--p != head)
        if (*p == '.')
          break;

      if (!strcmp(p, ".rvdata"))
        api_version = APIVersion::RGSS2;
      else if (!strcmp(p, ".rvdata2"))
        api_version = APIVersion::RGSS3;
    }
  }
  default_font_path =
      reader->Get("Engine", "DefaultFontPath", default_font_path);
  ReplaceStringWidth(default_font_path, '\\', '/');
  i18n_xml_path =
      reader->Get("Engine", "I18nXMLPath", std::string(app + ".xml"));

  if (api_version == APIVersion::RGSS1)
    resolution = base::Vec2i(640, 480);
  if (api_version >= APIVersion::RGSS2)
    resolution = base::Vec2i(544, 416);
  resolution =
      GetVec2iFromString(reader->Get("Engine", "Resolution", ""), resolution);
  window_size =
      GetVec2iFromString(reader->Get("Engine", "WindowSize", ""), resolution);

  // GUI
  disable_settings =
      reader->GetBoolean("GUI", "DisableSettings", disable_settings);
  disable_fps_monitor =
      reader->GetBoolean("GUI", "DisableFPSMonitor", disable_fps_monitor);
  disable_reset = reader->GetBoolean("GUI", "DisableReset", disable_reset);

  // Audio
  audio_volume = reader->GetFloat("Audio", "Volume", audio_volume);
  midi_soundfont = reader->Get("Audio", "SoundFont", midi_soundfont);

  // Renderer
  driver_backend = reader->Get("Renderer", "Backend", driver_backend);
  pipeline_default_sampler = reader->GetInteger(
      "Renderer", "PipelineDefaultSampler", pipeline_default_sampler);
  render_validation =
      reader->GetBoolean("Renderer", "RenderValidation", render_validation);
  u32_draw_index =
      reader->GetBoolean("Renderer", "LargeDrawIndex", u32_draw_index);
  frame_rate = reader->GetInteger("Renderer", "FrameRate",
                                  (api_version == APIVersion::RGSS1) ? 40 : 60);
  vsync = reader->GetInteger("Renderer", "VSync", vsync);
  keep_ratio = reader->GetBoolean("Renderer", "KeepRatio", keep_ratio);
  allow_skip_frame =
      reader->GetBoolean("Renderer", "AllowSkipFrame", allow_skip_frame);
  fullscreen = reader->GetBoolean("Renderer", "Fullscreen", fullscreen);
  background_running =
      reader->GetBoolean("Renderer", "BackgroundRunning", background_running);
  smooth_scale_present = reader->GetBoolean("Renderer", "SmoothScalePresent",
                                            smooth_scale_present);
  smooth_scaling =
      reader->GetInteger("Renderer", "SmoothScaling", smooth_scaling);
  smooth_scaling_down =
      reader->GetInteger("Renderer", "SmoothScalingDown", smooth_scaling_down);
  integer_scaling =
      reader->GetBoolean("Renderer", "IntegerScaling", integer_scaling);
  scaling_mode =
      reader->GetInteger("Renderer", "ScalingMode", scaling_mode);
  scaling_ar_strength =
      reader->GetFloat("Renderer", "ScalingARStrength", scaling_ar_strength);
  scaling_bicubic_b =
      reader->GetFloat("Renderer", "ScalingBicubicB", scaling_bicubic_b);
  scaling_bicubic_c =
      reader->GetFloat("Renderer", "ScalingBicubicC", scaling_bicubic_c);
  cas_enabled =
      reader->GetBoolean("Renderer", "CASEnabled", cas_enabled);
  cas_sharpness =
      reader->GetFloat("Renderer", "CASSharpness", cas_sharpness);
  scaling_sobel_strength =
      reader->GetFloat("Renderer", "ScalingSobelStrength", scaling_sobel_strength);
  scaling_warp_strength =
      reader->GetFloat("Renderer", "ScalingWarpStrength", scaling_warp_strength);
  scaling_darken_strength =
      reader->GetFloat("Renderer", "ScalingDarkenStrength", scaling_darken_strength);
  sync_to_refresh_rate =
      reader->GetBoolean("Renderer", "SyncToRefreshRate", sync_to_refresh_rate);
  win_resizable =
      reader->GetBoolean("Renderer", "WinResizable", win_resizable);
  fixed_aspect_ratio =
      reader->GetBoolean("Renderer", "FixedAspectRatio", fixed_aspect_ratio);

  // Font
  font_scale = reader->GetFloat("Engine", "FontScale", font_scale);
  font_kerning = reader->GetBoolean("Engine", "FontKerning", font_kerning);
  font_hinting =
      reader->GetInteger("Engine", "FontHinting", font_hinting);
  font_outline_crop =
      reader->GetBoolean("Engine", "FontOutlineCrop", font_outline_crop);
  save_log = reader->GetBoolean("Engine", "SaveLog", save_log);

  {
    std::string subs_line = reader->Get("Engine", "FontSubs", "");
    if (!subs_line.empty()) {
      size_t pos = 0;
      while (pos < subs_line.size()) {
        size_t comma = subs_line.find(',', pos);
        std::string entry = subs_line.substr(pos, comma - pos);
        if (!entry.empty()) {
          entry.erase(0, entry.find_first_not_of(" \t"));
          entry.erase(entry.find_last_not_of(" \t") + 1);
          if (!entry.empty())
            font_subs.push_back(entry);
        }
        if (comma == std::string::npos)
          break;
        pos = comma + 1;
      }
    }
  }

  // Platform
  debugging_console =
      reader->GetBoolean("Platform", "DebuggingConsole", debugging_console);
  disable_ime = reader->GetBoolean("Platform", "DisableIME", disable_ime);
  orientation = reader->Get("Platform", "Orientations", orientation);

  // End of parsing
  if (ini_stream_)
    SDL_CloseIO(ini_stream_);

  return true;
}

void ContentProfile::SaveConfigure() {
  FILE* fp = fopen(ini_path_.c_str(), "w");
  if (!fp)
    return;

  fprintf(fp, "[Game]\n");
  fprintf(fp, "Scripts=%s\n", script_path.c_str());
  fprintf(fp, "Title=%s\n", window_title.c_str());
  fprintf(fp, "\n[Engine]\n");
  fprintf(fp, "APIVersion=%d\n", static_cast<int>(api_version));
  fprintf(fp, "DefaultFontPath=%s\n", default_font_path.c_str());
  fprintf(fp, "Resolution=%d|%d\n", resolution.x, resolution.y);
  fprintf(fp, "WindowSize=%d|%d\n", window_size.x, window_size.y);
  fprintf(fp, "I18nXMLPath=%s\n", i18n_xml_path.c_str());
  if (font_scale != 0.9f)
    fprintf(fp, "FontScale=%.1f\n", font_scale);
  if (font_kerning != true)
    fprintf(fp, "FontKerning=%s\n", font_kerning ? "true" : "false");
  if (font_hinting != 3)
    fprintf(fp, "FontHinting=%d\n", font_hinting);
  if (font_outline_crop != true)
    fprintf(fp, "FontOutlineCrop=%s\n", font_outline_crop ? "true" : "false");
  if (save_log != true)
    fprintf(fp, "SaveLog=%s\n", save_log ? "true" : "false");
  fprintf(fp, "\n[Audio]\n");
  if (audio_volume != 1.0f)
    fprintf(fp, "Volume=%.2f\n", audio_volume);
  if (!midi_soundfont.empty())
    fprintf(fp, "SoundFont=%s\n", midi_soundfont.c_str());
  fprintf(fp, "\n[Renderer]\n");
  if (!driver_backend.empty())
    fprintf(fp, "Backend=%s\n", driver_backend.c_str());
  if (render_validation !=
#if DILIGENT_DEVELOPMENT
      true
#else
      false
#endif
  )
    fprintf(fp, "RenderValidation=%s\n", render_validation ? "true" : "false");
  if (u32_draw_index != true)
    fprintf(fp, "LargeDrawIndex=%s\n", u32_draw_index ? "true" : "false");
  {
    int default_fr = (api_version == APIVersion::RGSS1) ? 40 : 60;
    if (frame_rate != default_fr)
      fprintf(fp, "FrameRate=%d\n", frame_rate);
  }
  if (vsync != 1)
    fprintf(fp, "VSync=%u\n", vsync);
  if (keep_ratio != true)
    fprintf(fp, "KeepRatio=%s\n", keep_ratio ? "true" : "false");
  if (allow_skip_frame != true)
    fprintf(fp, "AllowSkipFrame=%s\n", allow_skip_frame ? "true" : "false");
  if (fullscreen != false)
    fprintf(fp, "Fullscreen=%s\n", fullscreen ? "true" : "false");
  if (background_running != true)
    fprintf(fp, "BackgroundRunning=%s\n", background_running ? "true" : "false");
  if (smooth_scale_present != false)
    fprintf(fp, "SmoothScalePresent=%s\n", smooth_scale_present ? "true" : "false");
  if (smooth_scaling != 0)
    fprintf(fp, "SmoothScaling=%d\n", smooth_scaling);
  if (smooth_scaling_down != 0)
    fprintf(fp, "SmoothScalingDown=%d\n", smooth_scaling_down);
  if (integer_scaling != false)
    fprintf(fp, "IntegerScaling=%s\n", integer_scaling ? "true" : "false");
  if (scaling_mode != 0)
    fprintf(fp, "ScalingMode=%d\n", scaling_mode);
  if (scaling_ar_strength != 0.5f)
    fprintf(fp, "ScalingARStrength=%.2f\n", scaling_ar_strength);
  if (scaling_bicubic_b != 0.33f)
    fprintf(fp, "ScalingBicubicB=%.2f\n", scaling_bicubic_b);
  if (scaling_bicubic_c != 0.33f)
    fprintf(fp, "ScalingBicubicC=%.2f\n", scaling_bicubic_c);
  if (cas_enabled != false)
    fprintf(fp, "CASEnabled=%s\n", cas_enabled ? "true" : "false");
  if (cas_sharpness != 0.4f)
    fprintf(fp, "CASSharpness=%.2f\n", cas_sharpness);
  if (scaling_sobel_strength != 1.0f)
    fprintf(fp, "ScalingSobelStrength=%.2f\n", scaling_sobel_strength);
  if (scaling_warp_strength != 0.0f)
    fprintf(fp, "ScalingWarpStrength=%.2f\n", scaling_warp_strength);
  if (scaling_darken_strength != 0.0f)
    fprintf(fp, "ScalingDarkenStrength=%.2f\n", scaling_darken_strength);
  if (sync_to_refresh_rate != false)
    fprintf(fp, "SyncToRefreshRate=%s\n", sync_to_refresh_rate ? "true" : "false");
  if (win_resizable != true)
    fprintf(fp, "WinResizable=%s\n", win_resizable ? "true" : "false");
  if (fixed_aspect_ratio != true)
    fprintf(fp, "FixedAspectRatio=%s\n", fixed_aspect_ratio ? "true" : "false");
  fprintf(fp, "\n[GUI]\n");
  if (disable_settings != false)
    fprintf(fp, "DisableSettings=%s\n", disable_settings ? "true" : "false");
  if (disable_fps_monitor != false)
    fprintf(fp, "DisableFPSMonitor=%s\n", disable_fps_monitor ? "true" : "false");
  if (disable_reset != false)
    fprintf(fp, "DisableReset=%s\n", disable_reset ? "true" : "false");
  fprintf(fp, "\n[Platform]\n");
  if (debugging_console != false)
    fprintf(fp, "DebuggingConsole=%s\n", debugging_console ? "true" : "false");
  if (disable_ime != false)
    fprintf(fp, "DisableIME=%s\n", disable_ime ? "true" : "false");

  fclose(fp);
}

void ContentProfile::ResetAudioDefaults() {
  audio_volume = 1.0f;
}

void ContentProfile::ResetRendererDefaults() {
  driver_backend.clear();
  pipeline_default_sampler = 0;
  render_validation =
#if DILIGENT_DEVELOPMENT
      true;
#else
      false;
#endif
  u32_draw_index = true;
  frame_rate = (api_version == APIVersion::RGSS1) ? 40 : 60;
  vsync = 1;
  keep_ratio = true;
  fullscreen = false;
  allow_skip_frame = true;
  background_running = true;
  smooth_scale_present = false;
  smooth_scaling = 0;
  smooth_scaling_down = 0;
  integer_scaling = false;
  scaling_mode = 0;
  scaling_ar_strength = 0.5f;
  scaling_bicubic_b = 0.33f;
  scaling_bicubic_c = 0.33f;
  cas_enabled = false;
  cas_sharpness = 0.4f;
  scaling_sobel_strength = 1.0f;
  scaling_warp_strength = 0.0f;
  scaling_darken_strength = 0.0f;
  sync_to_refresh_rate = false;
  win_resizable = true;
  fixed_aspect_ratio = true;
}

}  // namespace content
