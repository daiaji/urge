// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/profile/content_profile.h"

#include "base/buildflags/build.h"
#include "base/debug/logging.h"

#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <utility>

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

std::string TrimASCIIWhitespace(std::string value) {
  size_t start = value.find_first_not_of(" \t");
  if (start == std::string::npos)
    return {};
  size_t end = value.find_last_not_of(" \t");
  return value.substr(start, end - start + 1);
}

std::string JoinStrings(const std::vector<std::string>& values,
                        const char* separator) {
  std::string out;
  for (const auto& value : values) {
    if (value.empty())
      continue;
    if (!out.empty())
      out += separator;
    out += value;
  }
  return out;
}

base::Vec2i GetVec2iFromString(std::string in,
                               const base::Vec2i& default_value) {
  size_t sep = in.find("|");
  if (sep != std::string::npos) {
    try {
      int32_t num1 = std::stoi(in.substr(0, sep).c_str());
      int32_t num2 = std::stoi(in.substr(sep + 1).c_str());
      if (num1 > 0 && num2 > 0)
        return base::Vec2i(num1, num2);
    } catch (...) {
      LOG(WARNING) << "[Profile] Invalid size value: " << in;
    }
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

std::string TrimCopy(const std::string& value) {
  const size_t begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos)
    return "";
  const size_t end = value.find_last_not_of(" \t\r\n");
  return value.substr(begin, end - begin + 1);
}

bool ParseIniKeyLine(const std::string& line, std::string* key) {
  std::string trimmed = TrimCopy(line);
  if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#')
    return false;
  size_t equal = trimmed.find('=');
  if (equal == std::string::npos)
    return false;
  *key = TrimCopy(trimmed.substr(0, equal));
  return !key->empty();
}

bool ParseIniSectionLine(const std::string& line, std::string* section) {
  std::string trimmed = TrimCopy(line);
  if (trimmed.size() < 3 || trimmed.front() != '[' || trimmed.back() != ']')
    return false;
  *section = TrimCopy(trimmed.substr(1, trimmed.size() - 2));
  return !section->empty();
}

std::string FormatBool(bool value) {
  return value ? "true" : "false";
}

std::string FormatFloat(float value, int precision) {
  std::ostringstream stream;
  stream.setf(std::ios::fixed);
  stream.precision(precision);
  stream << value;
  return stream.str();
}

using IniValues = std::map<std::string, std::map<std::string, std::string>>;
using IniKeySet = std::map<std::string, std::set<std::string>>;

void SetIniValue(IniValues* values,
                 const char* section,
                 const char* key,
                 std::string value) {
  (*values)[section][key] = std::move(value);
}

void AddKnownIniKey(IniKeySet* keys, const char* section, const char* key) {
  (*keys)[section].insert(key);
}

bool IsKnownIniKey(const IniKeySet& keys,
                   const std::string& section,
                   const std::string& key) {
  auto section_it = keys.find(section);
  return section_it != keys.end() && section_it->second.contains(key);
}

void WriteKnownSection(std::string* output,
                       const std::string& section,
                       const std::map<std::string, std::string>& values,
                       std::set<std::string>* written_keys) {
  if (values.empty())
    return;
  if (!output->empty() && output->back() != '\n')
    *output += '\n';
  if (!output->empty())
    *output += '\n';
  *output += "[" + section + "]\n";
  for (const auto& [key, value] : values) {
    *output += key + "=" + value + "\n";
    written_keys->insert(section + "." + key);
  }
}

void WriteMissingKeys(std::string* output,
                      const std::string& section,
                      const IniValues& values,
                      std::set<std::string>* written_keys) {
  auto section_it = values.find(section);
  if (section_it == values.end())
    return;
  for (const auto& [key, value] : section_it->second) {
    const std::string full_key = section + "." + key;
    if (written_keys->contains(full_key))
      continue;
    *output += key + "=" + value + "\n";
    written_keys->insert(full_key);
  }
}

void WriteMergedIniFile(const std::string& path,
                        const IniValues& values,
                        const IniKeySet& known_keys,
                        const std::vector<std::string>& section_order) {
  std::ifstream in(path);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(in, line))
    lines.push_back(line);

  std::string output;
  std::string current_section;
  std::set<std::string> seen_sections;
  std::set<std::string> written_keys;

  for (const auto& original_line : lines) {
    std::string section;
    if (ParseIniSectionLine(original_line, &section)) {
      WriteMissingKeys(&output, current_section, values, &written_keys);
      current_section = section;
      seen_sections.insert(section);
      output += original_line + "\n";
      continue;
    }

    std::string key;
    if (ParseIniKeyLine(original_line, &key)) {
      if (IsKnownIniKey(known_keys, current_section, key)) {
        auto section_it = values.find(current_section);
        if (section_it != values.end()) {
          auto key_it = section_it->second.find(key);
          if (key_it != section_it->second.end()) {
            output += key + "=" + key_it->second + "\n";
            written_keys.insert(current_section + "." + key);
          }
        }
        continue;
      }
    }

    output += original_line + "\n";
  }
  WriteMissingKeys(&output, current_section, values, &written_keys);

  for (const auto& section : section_order) {
    if (seen_sections.contains(section))
      continue;
    auto section_it = values.find(section);
    if (section_it == values.end())
      continue;
    std::map<std::string, std::string> missing;
    for (const auto& [key, value] : section_it->second) {
      if (!written_keys.contains(section + "." + key))
        missing[key] = value;
    }
    WriteKnownSection(&output, section, missing, &written_keys);
  }

  std::ofstream out(path, std::ios::trunc);
  out << output;
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
  api_version_configured = !reader->Get("Engine", "APIVersion", "").empty();
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
  udl_auto_fit =
      reader->GetBoolean("Renderer", "UDLAutoFit", udl_auto_fit);

  // Font
  font_scale = reader->GetFloat("Engine", "FontScale", font_scale);
  font_kerning = reader->GetBoolean("Engine", "FontKerning", font_kerning);
  font_hinting =
      reader->GetInteger("Engine", "FontHinting", font_hinting);
  font_outline_crop =
      reader->GetBoolean("Engine", "FontOutlineCrop", font_outline_crop);
  save_log = reader->GetBoolean("Engine", "SaveLog", save_log);

  {
    font_subs.clear();
    std::string subs_line = reader->Get("Engine", "FontSubs", "");
    if (!subs_line.empty()) {
      size_t pos = 0;
      while (pos < subs_line.size()) {
        size_t comma = subs_line.find(',', pos);
        std::string entry = subs_line.substr(pos, comma - pos);
        if (!entry.empty()) {
          entry = TrimASCIIWhitespace(entry);
          size_t sep = entry.find('>');
          if (sep != std::string::npos) {
            std::string from = TrimASCIIWhitespace(entry.substr(0, sep));
            std::string to = TrimASCIIWhitespace(entry.substr(sep + 1));
            entry = from.empty() || to.empty() ? std::string() : from + ">" + to;
          }
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
  IniValues values;
  IniKeySet known_keys;
  auto known = [&known_keys](const char* section, const char* key) {
    AddKnownIniKey(&known_keys, section, key);
  };
  known("Game", "Scripts");
  known("Game", "Title");
  known("Engine", "APIVersion");
  known("Engine", "DefaultFontPath");
  known("Engine", "Resolution");
  known("Engine", "WindowSize");
  known("Engine", "I18nXMLPath");
  known("Engine", "FontScale");
  known("Engine", "FontKerning");
  known("Engine", "FontHinting");
  known("Engine", "FontOutlineCrop");
  known("Engine", "SaveLog");
  known("Engine", "FontSubs");
  known("Audio", "Volume");
  known("Audio", "SoundFont");
  known("Renderer", "Backend");
  known("Renderer", "PipelineDefaultSampler");
  known("Renderer", "RenderValidation");
  known("Renderer", "LargeDrawIndex");
  known("Renderer", "FrameRate");
  known("Renderer", "VSync");
  known("Renderer", "KeepRatio");
  known("Renderer", "AllowSkipFrame");
  known("Renderer", "Fullscreen");
  known("Renderer", "BackgroundRunning");
  known("Renderer", "SmoothScalePresent");
  known("Renderer", "SmoothScaling");
  known("Renderer", "SmoothScalingDown");
  known("Renderer", "IntegerScaling");
  known("Renderer", "ScalingMode");
  known("Renderer", "ScalingARStrength");
  known("Renderer", "ScalingBicubicB");
  known("Renderer", "ScalingBicubicC");
  known("Renderer", "CASEnabled");
  known("Renderer", "CASSharpness");
  known("Renderer", "ScalingSobelStrength");
  known("Renderer", "ScalingWarpStrength");
  known("Renderer", "ScalingDarkenStrength");
  known("Renderer", "SyncToRefreshRate");
  known("Renderer", "WinResizable");
  known("Renderer", "FixedAspectRatio");
  known("Renderer", "UDLAutoFit");
  known("GUI", "DisableSettings");
  known("GUI", "DisableFPSMonitor");
  known("GUI", "DisableReset");
  known("Platform", "DebuggingConsole");
  known("Platform", "DisableIME");
  known("Platform", "Orientations");

  if (script_path != "Data/Scripts.rxdata")
    SetIniValue(&values, "Game", "Scripts", script_path);
  if (window_title != "URGE Widget")
    SetIniValue(&values, "Game", "Title", window_title);
  if (api_version_configured)
    SetIniValue(&values, "Engine", "APIVersion",
                std::to_string(static_cast<int>(api_version)));
  if (default_font_path != "Fonts/Default.ttf")
    SetIniValue(&values, "Engine", "DefaultFontPath", default_font_path);
  const base::Vec2i default_resolution =
      (api_version == APIVersion::RGSS1) ? base::Vec2i(640, 480)
                                        : base::Vec2i(544, 416);
  if (resolution.x != default_resolution.x ||
      resolution.y != default_resolution.y)
    SetIniValue(&values, "Engine", "Resolution",
                std::to_string(resolution.x) + "|" + std::to_string(resolution.y));
  if (window_size.x != resolution.x || window_size.y != resolution.y)
    SetIniValue(&values, "Engine", "WindowSize",
                std::to_string(window_size.x) + "|" + std::to_string(window_size.y));
  if (i18n_xml_path != program_name + ".xml")
    SetIniValue(&values, "Engine", "I18nXMLPath", i18n_xml_path);
  if (font_scale != 0.9f)
    SetIniValue(&values, "Engine", "FontScale", FormatFloat(font_scale, 1));
  if (font_kerning != true)
    SetIniValue(&values, "Engine", "FontKerning", FormatBool(font_kerning));
  if (font_hinting != 3)
    SetIniValue(&values, "Engine", "FontHinting", std::to_string(font_hinting));
  if (font_outline_crop != true)
    SetIniValue(&values, "Engine", "FontOutlineCrop", FormatBool(font_outline_crop));
  if (save_log != true)
    SetIniValue(&values, "Engine", "SaveLog", FormatBool(save_log));
  if (!font_subs.empty())
    SetIniValue(&values, "Engine", "FontSubs", JoinStrings(font_subs, ","));

  if (audio_volume != 1.0f)
    SetIniValue(&values, "Audio", "Volume", FormatFloat(audio_volume, 2));
  if (!midi_soundfont.empty())
    SetIniValue(&values, "Audio", "SoundFont", midi_soundfont);

  if (!driver_backend.empty())
    SetIniValue(&values, "Renderer", "Backend", driver_backend);
  if (pipeline_default_sampler != 0)
    SetIniValue(&values, "Renderer", "PipelineDefaultSampler",
                std::to_string(pipeline_default_sampler));
  if (render_validation !=
#if DILIGENT_DEVELOPMENT
      true
#else
      false
#endif
  )
    SetIniValue(&values, "Renderer", "RenderValidation", FormatBool(render_validation));
  if (u32_draw_index != true)
    SetIniValue(&values, "Renderer", "LargeDrawIndex", FormatBool(u32_draw_index));
  {
    int default_fr = (api_version == APIVersion::RGSS1) ? 40 : 60;
    if (frame_rate != default_fr)
      SetIniValue(&values, "Renderer", "FrameRate", std::to_string(frame_rate));
  }
  if (vsync != 1)
    SetIniValue(&values, "Renderer", "VSync", std::to_string(vsync));
  if (keep_ratio != true)
    SetIniValue(&values, "Renderer", "KeepRatio", FormatBool(keep_ratio));
  if (allow_skip_frame != true)
    SetIniValue(&values, "Renderer", "AllowSkipFrame", FormatBool(allow_skip_frame));
  if (fullscreen != false)
    SetIniValue(&values, "Renderer", "Fullscreen", FormatBool(fullscreen));
  if (background_running != true)
    SetIniValue(&values, "Renderer", "BackgroundRunning", FormatBool(background_running));
  if (smooth_scale_present != false)
    SetIniValue(&values, "Renderer", "SmoothScalePresent", FormatBool(smooth_scale_present));
  if (smooth_scaling != 0)
    SetIniValue(&values, "Renderer", "SmoothScaling", std::to_string(smooth_scaling));
  if (smooth_scaling_down != 0)
    SetIniValue(&values, "Renderer", "SmoothScalingDown", std::to_string(smooth_scaling_down));
  if (integer_scaling != false)
    SetIniValue(&values, "Renderer", "IntegerScaling", FormatBool(integer_scaling));
  if (scaling_mode != 0)
    SetIniValue(&values, "Renderer", "ScalingMode", std::to_string(scaling_mode));
  if (scaling_ar_strength != 0.5f)
    SetIniValue(&values, "Renderer", "ScalingARStrength", FormatFloat(scaling_ar_strength, 2));
  if (scaling_bicubic_b != 0.33f)
    SetIniValue(&values, "Renderer", "ScalingBicubicB", FormatFloat(scaling_bicubic_b, 2));
  if (scaling_bicubic_c != 0.33f)
    SetIniValue(&values, "Renderer", "ScalingBicubicC", FormatFloat(scaling_bicubic_c, 2));
  if (cas_enabled != false)
    SetIniValue(&values, "Renderer", "CASEnabled", FormatBool(cas_enabled));
  if (cas_sharpness != 0.4f)
    SetIniValue(&values, "Renderer", "CASSharpness", FormatFloat(cas_sharpness, 2));
  if (scaling_sobel_strength != 1.0f)
    SetIniValue(&values, "Renderer", "ScalingSobelStrength", FormatFloat(scaling_sobel_strength, 2));
  if (scaling_warp_strength != 0.0f)
    SetIniValue(&values, "Renderer", "ScalingWarpStrength", FormatFloat(scaling_warp_strength, 2));
  if (scaling_darken_strength != 0.0f)
    SetIniValue(&values, "Renderer", "ScalingDarkenStrength", FormatFloat(scaling_darken_strength, 2));
  if (sync_to_refresh_rate != false)
    SetIniValue(&values, "Renderer", "SyncToRefreshRate", FormatBool(sync_to_refresh_rate));
  if (win_resizable != true)
    SetIniValue(&values, "Renderer", "WinResizable", FormatBool(win_resizable));
  if (fixed_aspect_ratio != true)
    SetIniValue(&values, "Renderer", "FixedAspectRatio", FormatBool(fixed_aspect_ratio));
  if (udl_auto_fit != false)
    SetIniValue(&values, "Renderer", "UDLAutoFit", FormatBool(udl_auto_fit));

  if (disable_settings != false)
    SetIniValue(&values, "GUI", "DisableSettings", FormatBool(disable_settings));
  if (disable_fps_monitor != false)
    SetIniValue(&values, "GUI", "DisableFPSMonitor", FormatBool(disable_fps_monitor));
  if (disable_reset != false)
    SetIniValue(&values, "GUI", "DisableReset", FormatBool(disable_reset));

  if (debugging_console != false)
    SetIniValue(&values, "Platform", "DebuggingConsole", FormatBool(debugging_console));
  if (disable_ime != false)
    SetIniValue(&values, "Platform", "DisableIME", FormatBool(disable_ime));
  if (orientation != "LandscapeLeft LandscapeRight")
    SetIniValue(&values, "Platform", "Orientations", orientation);

  WriteMergedIniFile(
      ini_path_, values, known_keys,
      {"Game", "Engine", "Audio", "Renderer", "GUI", "Platform"});
}

void ContentProfile::ResetAudioDefaults() {
  audio_volume = 1.0f;
  midi_soundfont.clear();
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
  udl_auto_fit = false;
}

}  // namespace content
