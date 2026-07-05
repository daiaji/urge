#ifndef COMPONENTS_AUDIOSERVICE_MIDI_PLAYER_H_
#define COMPONENTS_AUDIOSERVICE_MIDI_PLAYER_H_

#include <string>
#include <vector>

struct SDL_SharedObject;

namespace audioservice {

class MidiStreamSource;

// Runtime FluidSynth loader + MIDI streaming.
// Inspired by mkxp-z's fluid-fun + midisource design.
class MidiPlayer {
 public:
  MidiPlayer();
  ~MidiPlayer();

  MidiPlayer(const MidiPlayer&) = delete;
  MidiPlayer& operator=(const MidiPlayer&) = delete;

  // Set SoundFont path. Must be set before rendering.
  void SetSoundFont(const std::string& sf2_path);

  const std::string& GetSoundFont() const { return soundfont_path_; }

  // Returns true if FluidSynth was loaded successfully.
  bool IsAvailable() const { return have_fluid_; }

  // Create a streaming MIDI data source (fluid_synth + fluid_player).
  // Returns null on failure. Caller owns the returned object.
  MidiStreamSource* CreateStream(const std::string& midi_path);

  // FluidSynth API accessors (used by MidiStreamSource)
  void FluidSynthWriteFloat(void* synth, int n, float* l, int lo, int li,
                            float* r, int ro, int ri) {
    api_.synth_write_float(synth, n, l, lo, li, r, ro, ri);
  }
  int FluidPlayerGetStatus(void* player) {
    return api_.player_get_status(player);
  }
  void FluidSynthSystemReset(void* synth) {
    api_.synth_system_reset(synth);
  }
  void* FluidNewPlayer(void* synth) { return api_.new_player(synth); }
  int FluidPlayerAddMem(void* player, const void* data, size_t len) {
    return api_.player_add_mem(player, data, len);
  }
  int FluidPlayerPlay(void* player) { return api_.player_play(player); }
  void FluidDeletePlayer(void* player) { api_.delete_player(player); }
  void FluidDeleteSynth(void* synth) { api_.delete_synth(synth); }
  void FluidDeleteSettings(void* settings) { api_.delete_settings(settings); }

  // Resolve SoundFont path via PHYSFS, return real filesystem path.
  std::string ResolveSoundFontPath() const;

  // Read MIDI file data through PHYSFS. Returns true on success.
  static bool ReadMIDIFile(const std::string& path, std::vector<char>* data);

  // Check if path is a MIDI file by extension (case-insensitive).
  static bool IsMIDIFile(const std::string& path);

 private:
  bool LoadFluidSynth();
  void UnloadFluidSynth();

  std::string soundfont_path_;

  // Dynamic library handle
  SDL_SharedObject* lib_handle_ = nullptr;

  // FluidSynth function pointers (populated by LoadFluidSynth)
  struct FluidAPI {
    void* (*new_settings)();
    void* (*new_synth)(void*);
    void (*delete_settings)(void*);
    void (*delete_synth)(void*);
    int (*settings_setnum)(void*, const char*, double);
    int (*settings_setint)(void*, const char*, int);
    int (*settings_setstr)(void*, const char*, const char*);
    int (*synth_sfload)(void*, const char*, int);
    void (*synth_system_reset)(void*);
    int (*synth_write_float)(void*, int, float*, int, int, float*, int, int);
    int (*synth_noteon)(void*, int, int, int);
    int (*synth_noteoff)(void*, int, int);
    int (*synth_cc)(void*, int, int, int);
    int (*synth_program_change)(void*, int, int);
    int (*synth_pitch_bend)(void*, int, int);
    int (*synth_channel_pressure)(void*, int, int);
    void* (*new_player)(void*);
    int (*player_add)(void*, const char*);
    int (*player_add_mem)(void*, const void*, size_t);
    int (*player_play)(void*);
    int (*player_get_status)(void*);
    void (*delete_player)(void*);
  } api_ = {};

  bool have_fluid_ = false;
};

}  // namespace audioservice

#endif  // !COMPONENTS_AUDIOSERVICE_MIDI_PLAYER_H_
