#ifndef COMPONENTS_AUDIOSERVICE_MIDI_PLAYER_H_
#define COMPONENTS_AUDIOSERVICE_MIDI_PLAYER_H_

#include <string>
#include <vector>

struct SDL_SharedObject;

namespace audioservice {

// Runtime FluidSynth loader + MIDI → PCM renderer.
// Inspired by mkxp-z's fluid-fun + midisource design.
class MidiPlayer {
 public:
  MidiPlayer();
  ~MidiPlayer();

  MidiPlayer(const MidiPlayer&) = delete;
  MidiPlayer& operator=(const MidiPlayer&) = delete;

  // Set SoundFont path. Must be set before rendering.
  void SetSoundFont(const std::string& sf2_path);

  // Returns true if FluidSynth was loaded successfully.
  bool IsAvailable() const { return have_fluid_; }

  // Render a MIDI file to interleaved float32 PCM.
  struct PCMResult {
    std::vector<float> samples;
    int sample_rate;
    int channels;
  };
  bool Render(const std::string& midi_path, PCMResult* out);

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
