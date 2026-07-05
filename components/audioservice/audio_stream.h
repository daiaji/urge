#ifndef COMPONENTS_AUDIOSERVICE_AUDIO_STREAM_H_
#define COMPONENTS_AUDIOSERVICE_AUDIO_STREAM_H_

#include <stdint.h>
#include <algorithm>
#include <memory>
#include <string>

#include <cmath>

#include "miniaudio.h"

// DirectSound-compatible -35dB logarithmic volume curve.
// Maps RGSS linear volume (0-100) to a logarithmic scale matching
// the human ear's perception curve, used by mkxp-z and original RGSS.
inline float LogVolumeCurve(int32_t volume) {
  if (volume <= 0)
    return 0.0f;
  float normalized = std::min(volume / 100.0f, 1.0f);
  return std::pow(10.0f, -(35.0f / 20.0f) * (1.0f - normalized));
}

namespace audioservice {

class MidiPlayer;
struct MidiStreamSource;

class AudioStream {
 public:
  ~AudioStream();

  AudioStream(const AudioStream&) = delete;
  AudioStream& operator=(const AudioStream&) = delete;

  // Play audio stream from file in looping.
  // If the filename is different from the last one, it will reset the audio.
  ma_result Play(const std::string& filename,
                 int32_t volume,
                 int32_t pitch,
                 uint64_t pos = 0);
  void Stop();
  void Fade(int32_t time);
  uint64_t Pos();

  // Pause and resume audio stream.
  bool IsPlaying();
  bool IsPausing();
  void Pause();
  void Resume();
  void BeginMEPause();
  void PauseForME();
  bool ResumeFromME();
  bool IsPausedForME() const { return paused_for_me_; }

  // Playback controller
  bool IsLooping();
  void SetLooping(bool looping);

  // Volume control (0-100, RGSS units)
  int32_t GetVolume() const { return current_volume_; }
  void SetVolume(int32_t volume);
  float GetExternalVolume() const { return external_volume_; }
  void SetExternalVolume(float volume);

 private:
  friend class AudioService;
  AudioStream(ma_engine* engine, MidiPlayer* midi_player);

  ma_result PlayMIDI(const std::string& filename,
                     int32_t volume,
                     int32_t pitch,
                     uint64_t pos);
  void ApplyVolume();
  void UninitSound();

  ma_engine* engine_;
  MidiPlayer* midi_player_;
  std::string filename_;
  ma_sound handle_;
  ma_uint64 cursor_;
  ma_bool32 looping_;
  int32_t current_volume_;
  float external_volume_ = 1.0f;
  bool paused_for_me_ = false;
  bool no_resume_after_me_ = false;
  bool initialized_ = false;

  // Streaming MIDI source (alive for duration of MIDI playback)
  std::unique_ptr<MidiStreamSource> midi_stream_;
};

}  // namespace audioservice

#endif  // !COMPONENTS_AUDIOSERVICE_AUDIO_STREAM_H_
