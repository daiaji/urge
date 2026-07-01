#ifndef COMPONENTS_AUDIOSERVICE_AUDIO_STREAM_H_
#define COMPONENTS_AUDIOSERVICE_AUDIO_STREAM_H_

#include <stdint.h>
#include <memory>
#include <string>

#include "miniaudio.h"

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

  // Playback controller
  bool IsLooping();
  void SetLooping(bool looping);

 private:
  friend class AudioService;
  AudioStream(ma_engine* engine, MidiPlayer* midi_player);

  ma_result PlayMIDI(const std::string& filename,
                     int32_t volume,
                     int32_t pitch,
                     uint64_t pos);

  ma_engine* engine_;
  MidiPlayer* midi_player_;
  std::string filename_;
  ma_sound handle_;
  ma_uint64 cursor_;
  ma_bool32 looping_;

  // Streaming MIDI source (alive for duration of MIDI playback)
  std::unique_ptr<MidiStreamSource> midi_stream_;
};

}  // namespace audioservice

#endif  // !COMPONENTS_AUDIOSERVICE_AUDIO_STREAM_H_
