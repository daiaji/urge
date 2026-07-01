#include "components/audioservice/audio_stream.h"

#include "base/debug/logging.h"
#include "components/audioservice/midi_player.h"
#include "components/audioservice/midi_stream_source.h"

namespace audioservice {

AudioStream::AudioStream(ma_engine* engine, MidiPlayer* midi_player)
    : engine_(engine),
      midi_player_(midi_player),
      handle_({}),
      cursor_(0),
      looping_(false) {}

AudioStream::~AudioStream() {
  ma_sound_uninit(&handle_);
}

ma_result AudioStream::Play(const std::string& filename,
                            int32_t volume,
                            int32_t pitch,
                            uint64_t pos) {
  if (filename.empty())
    return MA_INVALID_ARGS;

  LOG(INFO) << "[Audio] Play: " << filename;

  // Fast path: explicit MIDI extension (case-insensitive)
  if (MidiPlayer::IsMIDIFile(filename))
    return PlayMIDI(filename, volume, pitch, pos);

  if (!ma_sound_is_playing(&handle_) || filename_ != filename) {
    filename_ = filename;
    ma_sound_uninit(&handle_);

    ma_uint32 sound_flags = MA_SOUND_FLAG_ASYNC;
    if (looping_)
      sound_flags |= MA_SOUND_FLAG_LOOPING;
    auto result = ma_sound_init_from_file(
        engine_, filename.c_str(), sound_flags, nullptr, nullptr, &handle_);

    LOG(INFO) << "[Audio] ma_sound_init_from_file result=" << result
              << " for: " << filename;

    // If miniaudio can't decode it, try MIDI (RGSS paths often lack extension)
    if (result == MA_INVALID_FILE) {
      if (midi_player_ && midi_player_->IsAvailable())
        return PlayMIDI(filename, volume, pitch, pos);
      return result;
    }

    if (result != MA_SUCCESS)
      return result;
  }

  ma_sound_set_volume(&handle_, volume / 100.0f);
  ma_sound_set_pitch(&handle_, pitch / 100.0f);
  if (pos)
    ma_sound_seek_to_pcm_frame(&handle_, pos);
  ma_sound_start(&handle_);
  return MA_SUCCESS;
}

ma_result AudioStream::PlayMIDI(const std::string& filename,
                                int32_t volume,
                                int32_t pitch,
                                uint64_t pos) {
  LOG(INFO) << "[Audio] PlayMIDI: " << filename;

  if (!midi_player_ || !midi_player_->IsAvailable()) {
    LOG(WARNING) << "[MIDI] FluidSynth unavailable for: " << filename;
    return MA_DOES_NOT_EXIST;
  }

  // Try creating a streaming MIDI source
  auto stream = std::unique_ptr<MidiStreamSource>(
      midi_player_->CreateStream(filename));
  if (!stream) {
    // Retry with .mid extension for extensionless RGSS paths
    if (filename.rfind('.') == std::string::npos) {
      std::string with_ext = filename + ".mid";
      LOG(INFO) << "[Audio] PlayMIDI retry with: " << with_ext;
      stream = std::unique_ptr<MidiStreamSource>(
          midi_player_->CreateStream(with_ext));
    }
    if (!stream) {
      LOG(WARNING) << "[MIDI] Failed to create stream for: " << filename;
      return MA_DOES_NOT_EXIST;
    }
  }

  LOG(INFO) << "[Audio] PlayMIDI stream created OK";

  // Uninit previous sound
  ma_sound_uninit(&handle_);

  // Init sound from streaming data source
  ma_uint32 sound_flags = MA_SOUND_FLAG_ASYNC;
  if (looping_)
    sound_flags |= MA_SOUND_FLAG_LOOPING;

  auto result = ma_sound_init_from_data_source(
      engine_, &stream->base, sound_flags, nullptr, &handle_);
  if (result != MA_SUCCESS) {
    LOG(ERROR) << "[MIDI] ma_sound_init_from_data_source failed: " << result;
    return result;
  }

  // Keep stream alive for duration of playback
  midi_stream_ = std::move(stream);

  ma_sound_set_volume(&handle_, volume / 100.0f);
  ma_sound_set_pitch(&handle_, pitch / 100.0f);
  if (pos)
    ma_sound_seek_to_pcm_frame(&handle_, pos);
  ma_sound_start(&handle_);
  return MA_SUCCESS;
}

void AudioStream::Stop() {
  ma_sound_stop(&handle_);
}

void AudioStream::Fade(int32_t time) {
  ma_sound_stop_with_fade_in_milliseconds(&handle_, time);
}

uint64_t AudioStream::Pos() {
  ma_uint64 cursor;
  ma_sound_get_cursor_in_pcm_frames(&handle_, &cursor);
  return cursor;
}

bool AudioStream::IsPlaying() {
  return ma_sound_is_playing(&handle_);
}

bool AudioStream::IsPausing() {
  return cursor_ > 0;
}

void AudioStream::Pause() {
  ma_sound_get_cursor_in_pcm_frames(&handle_, &cursor_);
  ma_sound_stop(&handle_);
}

void AudioStream::Resume() {
  ma_sound_seek_to_pcm_frame(&handle_, cursor_);
  ma_sound_start(&handle_);
  cursor_ = 0;
}

bool AudioStream::IsLooping() {
  return looping_;
}

void AudioStream::SetLooping(bool looping) {
  looping_ = looping;
}

}  // namespace audioservice
