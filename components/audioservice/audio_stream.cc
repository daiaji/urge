#include "components/audioservice/audio_stream.h"

#include "base/debug/logging.h"
#include "components/audioservice/midi_player.h"
#include "components/audioservice/midi_stream_source.h"

namespace audioservice {

namespace {

bool HasPathExtension(const std::string& filename) {
  const size_t dot = filename.find_last_of('.');
  if (dot == std::string::npos)
    return false;
  const size_t slash = filename.find_last_of("/\\");
  return slash == std::string::npos || dot > slash;
}

bool ShouldTryMidiFallback(ma_result result) {
  return result == MA_INVALID_FILE || result == MA_DOES_NOT_EXIST;
}

}  // namespace

AudioStream::AudioStream(ma_engine* engine, MidiPlayer* midi_player)
    : engine_(engine),
      midi_player_(midi_player),
      handle_({}),
      cursor_(0),
      looping_(false),
      current_volume_(0),
      external_volume_(1.0f) {}

AudioStream::~AudioStream() {
  UninitSound();
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

  const bool need_new_sound = !initialized_ || filename_ != filename ||
                              midi_stream_ || pos == 0;
  if (need_new_sound) {
    UninitSound();
    filename_ = filename;

    ma_uint32 sound_flags = MA_SOUND_FLAG_ASYNC;
    if (looping_)
      sound_flags |= MA_SOUND_FLAG_LOOPING;
    auto result = ma_sound_init_from_file(
        engine_, filename.c_str(), sound_flags, nullptr, nullptr, &handle_);

    LOG(INFO) << "[Audio] ma_sound_init_from_file result=" << result
              << " for: " << filename;

    // If miniaudio can't open/decode it, try MIDI (RGSS paths often lack extension).
    if (ShouldTryMidiFallback(result)) {
      if (midi_player_ && midi_player_->IsAvailable())
        return PlayMIDI(filename, volume, pitch, pos);
      return result;
    }

    if (result != MA_SUCCESS)
      return result;

    initialized_ = true;
    midi_stream_.reset();
  }

  current_volume_ = volume;
  ApplyVolume();
  ma_sound_set_pitch(&handle_, pitch / 100.0f);
  if (pos && ma_sound_seek_to_pcm_frame(&handle_, pos) != MA_SUCCESS)
    return MA_BAD_SEEK;
  if (!paused_for_me_) {
    no_resume_after_me_ = false;
    auto result = ma_sound_start(&handle_);
    if (result != MA_SUCCESS)
      return result;
  } else {
    cursor_ = pos;
    no_resume_after_me_ = false;
  }
  return MA_SUCCESS;
}

void AudioStream::SetVolume(int32_t volume) {
  current_volume_ = volume;
  ApplyVolume();
}

void AudioStream::SetExternalVolume(float volume) {
  external_volume_ = std::clamp(volume, 0.0f, 1.0f);
  ApplyVolume();
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
    if (!HasPathExtension(filename)) {
      std::string with_ext = filename + ".mid";
      LOG(INFO) << "[Audio] PlayMIDI retry with: " << with_ext;
      stream = std::unique_ptr<MidiStreamSource>(
          midi_player_->CreateStream(with_ext));
    }
    if (!stream && !HasPathExtension(filename)) {
      std::string with_ext = filename + ".midi";
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
  UninitSound();

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
  initialized_ = true;

  // Keep stream alive for duration of playback
  midi_stream_ = std::move(stream);

  current_volume_ = volume;
  ApplyVolume();
  ma_sound_set_pitch(&handle_, pitch / 100.0f);
  if (pos && ma_sound_seek_to_pcm_frame(&handle_, pos) != MA_SUCCESS)
    return MA_BAD_SEEK;
  if (!paused_for_me_) {
    no_resume_after_me_ = false;
    auto result = ma_sound_start(&handle_);
    if (result != MA_SUCCESS)
      return result;
  } else {
    cursor_ = pos;
    no_resume_after_me_ = false;
  }
  return MA_SUCCESS;
}

void AudioStream::Stop() {
  no_resume_after_me_ = true;
  paused_for_me_ = false;
  external_volume_ = 1.0f;
  if (initialized_) {
    ma_sound_stop(&handle_);
    cursor_ = 0;
    ApplyVolume();
  }
}

void AudioStream::Fade(int32_t time) {
  no_resume_after_me_ = true;
  if (!initialized_)
    return;
  if (time <= 0) {
    Stop();
    return;
  }
  ma_sound_stop_with_fade_in_milliseconds(&handle_, time);
}

uint64_t AudioStream::Pos() {
  ma_uint64 cursor = 0;
  if (!initialized_)
    return 0;
  if (ma_sound_get_cursor_in_pcm_frames(&handle_, &cursor) != MA_SUCCESS)
    return 0;
  return cursor;
}

bool AudioStream::IsPlaying() {
  return initialized_ && ma_sound_is_playing(&handle_);
}

bool AudioStream::IsPausing() {
  return cursor_ > 0;
}

void AudioStream::Pause() {
  if (!initialized_)
    return;
  if (ma_sound_get_cursor_in_pcm_frames(&handle_, &cursor_) != MA_SUCCESS)
    cursor_ = 0;
  ma_sound_stop(&handle_);
}

void AudioStream::Resume() {
  if (!initialized_)
    return;
  if (paused_for_me_) {
    ResumeFromME();
    return;
  }
  if (ma_sound_seek_to_pcm_frame(&handle_, cursor_) != MA_SUCCESS)
    return;
  if (ma_sound_start(&handle_) != MA_SUCCESS)
    return;
  cursor_ = 0;
}

void AudioStream::BeginMEPause() {
  if (paused_for_me_)
    return;
  const bool should_resume = initialized_ && ma_sound_is_playing(&handle_);
  paused_for_me_ = true;
  no_resume_after_me_ = !should_resume;
}

void AudioStream::PauseForME() {
  if (!initialized_ || !paused_for_me_)
    return;
  if (ma_sound_get_cursor_in_pcm_frames(&handle_, &cursor_) != MA_SUCCESS)
    cursor_ = 0;
  ma_sound_stop(&handle_);
}

bool AudioStream::ResumeFromME() {
  if (!initialized_)
    return false;
  if (no_resume_after_me_) {
    paused_for_me_ = false;
    no_resume_after_me_ = false;
    cursor_ = 0;
    return false;
  }
  if (ma_sound_seek_to_pcm_frame(&handle_, cursor_) != MA_SUCCESS)
    return false;
  if (ma_sound_start(&handle_) != MA_SUCCESS)
    return false;
  cursor_ = 0;
  paused_for_me_ = false;
  return true;
}

bool AudioStream::IsLooping() {
  return looping_;
}

void AudioStream::SetLooping(bool looping) {
  looping_ = looping;
}

void AudioStream::ApplyVolume() {
  if (initialized_)
    ma_sound_set_volume(&handle_, LogVolumeCurve(current_volume_) *
                                      external_volume_);
}

void AudioStream::UninitSound() {
  if (initialized_) {
    ma_sound_uninit(&handle_);
    initialized_ = false;
  }
  midi_stream_.reset();
  cursor_ = 0;
}

}  // namespace audioservice
