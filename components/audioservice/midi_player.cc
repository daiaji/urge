#include "components/audioservice/midi_player.h"

#include "SDL3/SDL_loadso.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

#include "base/debug/logging.h"
#include "components/audioservice/midi_stream_source.h"
#include "physfs.h"

namespace audioservice {

// Platform-specific FluidSynth shared library name
#if defined(OS_WIN)
#define FLUID_LIB "fluidsynth.dll"
#elif defined(OS_APPLE)
#define FLUID_LIB "libfluidsynth.3.dylib"
#else
#define FLUID_LIB "libfluidsynth.so.3"
#endif

MidiPlayer::MidiPlayer() { have_fluid_ = LoadFluidSynth(); }

MidiPlayer::~MidiPlayer() { UnloadFluidSynth(); }

bool MidiPlayer::LoadFluidSynth() {
  lib_handle_ = SDL_LoadObject(FLUID_LIB);
  if (!lib_handle_) {
    LOG(WARNING) << "[MIDI] " << FLUID_LIB
                 << " not found. MIDI playback disabled.";
    return false;
  }

#define LOAD(member, name)                                              \
  do {                                                                  \
    auto _f = SDL_LoadFunction(lib_handle_, name);                     \
    if (!_f) {                                                          \
      LOG(WARNING) << "[MIDI] Failed to load: " << name;                \
      SDL_UnloadObject(lib_handle_);                                    \
      lib_handle_ = nullptr;                                            \
      return false;                                                     \
    }                                                                   \
    std::memcpy(&api_.member, &_f, sizeof(api_.member));               \
  } while (0)

  LOAD(settings_setnum, "fluid_settings_setnum");
  LOAD(settings_setint, "fluid_settings_setint");
  LOAD(settings_setstr, "fluid_settings_setstr");
  LOAD(synth_sfload, "fluid_synth_sfload");
  LOAD(synth_system_reset, "fluid_synth_system_reset");
  LOAD(synth_write_float, "fluid_synth_write_float");
  LOAD(synth_noteon, "fluid_synth_noteon");
  LOAD(synth_noteoff, "fluid_synth_noteoff");
  LOAD(synth_cc, "fluid_synth_cc");
  LOAD(synth_program_change, "fluid_synth_program_change");
  LOAD(synth_pitch_bend, "fluid_synth_pitch_bend");
  LOAD(synth_channel_pressure, "fluid_synth_channel_pressure");

  LOAD(new_settings, "new_fluid_settings");
  LOAD(new_synth, "new_fluid_synth");
  LOAD(delete_settings, "delete_fluid_settings");
  LOAD(delete_synth, "delete_fluid_synth");
  LOAD(new_player, "new_fluid_player");
  LOAD(player_add, "fluid_player_add");
  LOAD(player_add_mem, "fluid_player_add_mem");
  LOAD(player_play, "fluid_player_play");
  LOAD(player_get_status, "fluid_player_get_status");
  LOAD(delete_player, "delete_fluid_player");

#undef LOAD

  LOG(INFO) << "[MIDI] FluidSynth loaded successfully";
  return true;
}

void MidiPlayer::UnloadFluidSynth() {
  if (lib_handle_) {
    SDL_UnloadObject(lib_handle_);
    lib_handle_ = nullptr;
  }
}

void MidiPlayer::SetSoundFont(const std::string& sf2_path) {
  soundfont_path_ = sf2_path;
}

bool MidiPlayer::IsMIDIFile(const std::string& path) {
  auto dot = path.rfind('.');
  if (dot == std::string::npos)
    return false;
  std::string ext = path.substr(dot);
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return ext == ".mid" || ext == ".midi";
}

std::string MidiPlayer::ResolveSoundFontPath() const {
  if (soundfont_path_.empty())
    return {};
  const char* sf_dir = PHYSFS_getRealDir(soundfont_path_.c_str());
  if (sf_dir)
    return std::string(sf_dir) + "/" + soundfont_path_;
  return soundfont_path_;
}

bool MidiPlayer::ReadMIDIFile(const std::string& path,
                              std::vector<char>* data) {
  static const char* kExts[] = {".mid", ".midi"};
  auto try_open = [&](const std::string& p) -> bool {
    PHYSFS_File* f = PHYSFS_openRead(p.c_str());
    if (!f)
      return false;
    PHYSFS_sint64 sz = PHYSFS_fileLength(f);
    if (sz <= 0) {
      PHYSFS_close(f);
      return false;
    }
    data->resize(static_cast<size_t>(sz));
    PHYSFS_sint64 got = PHYSFS_readBytes(f, data->data(), sz);
    PHYSFS_close(f);
    return got == sz;
  };
  if (try_open(path))
    return true;
  // RGSS stores filenames without extension; try appending lower-case
  if (path.rfind('.') == std::string::npos) {
    for (auto* ext : kExts) {
      std::string with = path + ext;
      if (try_open(with)) {
        LOG(INFO) << "[MIDI] Resolved file: " << with;
        return true;
      }
    }
  }
  // CWD fallback via stdio
  FILE* fp = fopen(path.c_str(), "rb");
  if (fp) {
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);
    if (sz > 0) {
      data->resize(sz);
      size_t got = fread(data->data(), 1, sz, fp);
      fclose(fp);
      return got == static_cast<size_t>(sz);
    }
    fclose(fp);
  }
  return false;
}

MidiStreamSource* MidiPlayer::CreateStream(const std::string& midi_path) {
  if (!have_fluid_) {
    LOG(WARNING) << "[MIDI] FluidSynth unavailable, cannot play "
                 << midi_path;
    return nullptr;
  }

  if (soundfont_path_.empty()) {
    LOG(WARNING) << "[MIDI] No SoundFont configured, cannot play "
                 << midi_path << ". "
                 << "Set [Audio] SoundFont=<path> in Game.ini to enable MIDI.";
    return nullptr;
  }

  // Read MIDI file data
  std::vector<char> midi_data;
  if (!ReadMIDIFile(midi_path, &midi_data)) {
    LOG(ERROR) << "[MIDI] Cannot read file: " << midi_path;
    return nullptr;
  }

  // Create FluidSynth settings
  void* settings = api_.new_settings();
  if (!settings) {
    LOG(ERROR) << "[MIDI] Failed to create FluidSynth settings";
    return nullptr;
  }

  api_.settings_setnum(settings, "synth.sample-rate", 44100.0);
  api_.settings_setint(settings, "synth.chorus.active", 0);
  api_.settings_setint(settings, "synth.reverb.active", 0);

  // Create synthesizer
  void* synth = api_.new_synth(settings);
  if (!synth) {
    LOG(ERROR) << "[MIDI] Failed to create FluidSynth synthesizer";
    api_.delete_settings(settings);
    return nullptr;
  }

  // Load SoundFont from real filesystem path
  std::string real_sf = ResolveSoundFontPath();
  LOG(INFO) << "[MIDI] Loading SoundFont: " << real_sf;
  if (api_.synth_sfload(synth, real_sf.c_str(), 1) == -1) {
    LOG(ERROR) << "[MIDI] Failed to load SoundFont: " << real_sf;
    api_.delete_synth(synth);
    api_.delete_settings(settings);
    return nullptr;
  }

  // Create player and load MIDI from memory
  void* player = api_.new_player(synth);
  if (!player) {
    LOG(ERROR) << "[MIDI] Failed to create FluidSynth player";
    api_.delete_synth(synth);
    api_.delete_settings(settings);
    return nullptr;
  }

  if (api_.player_add_mem(player, midi_data.data(), midi_data.size()) != 0) {
    LOG(ERROR) << "[MIDI] Failed to load MIDI file";
    api_.delete_player(player);
    api_.delete_synth(synth);
    api_.delete_settings(settings);
    return nullptr;
  }

  api_.synth_system_reset(synth);
  api_.player_play(player);

  const int kSampleRate = 44100;
  const int kChannels = 2;
  return new MidiStreamSource(this, synth, player, settings, std::move(midi_data),
                              kSampleRate, kChannels);
}

}  // namespace audioservice
