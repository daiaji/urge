#include "components/audioservice/audio_stream.h"

#include <cstring>

#include "base/debug/logging.h"
#include "components/audioservice/midi_player.h"

namespace audioservice {

static bool IsMIDIFile(const std::string& path) {
  auto dot = path.rfind('.');
  if (dot == std::string::npos) return false;
  std::string ext = path.substr(dot);
  return ext == ".mid" || ext == ".MID" || ext == ".midi" || ext == ".MIDI";
}

// Write a 16-bit mono/stereo WAV header into buf. Returns bytes written.
static size_t WriteWAVHeader(uint8_t* buf, size_t data_bytes, int sample_rate,
                             int channels) {
  uint32_t data_sz = static_cast<uint32_t>(data_bytes);
  uint32_t file_sz = data_sz + 36;
  uint16_t fmt_tag = 3;  // IEEE float
  uint16_t num_ch = static_cast<uint16_t>(channels);
  uint32_t samp_rate = static_cast<uint32_t>(sample_rate);
  uint16_t bits = 32;  // float32
  uint16_t block_align = num_ch * (bits / 8);
  uint32_t byte_rate = samp_rate * block_align;

  std::memcpy(buf, "RIFF", 4);
  std::memcpy(buf + 4, &file_sz, 4);
  std::memcpy(buf + 8, "WAVE", 4);
  std::memcpy(buf + 12, "fmt ", 4);
  uint32_t subchunk1 = 16;
  std::memcpy(buf + 16, &subchunk1, 4);
  std::memcpy(buf + 20, &fmt_tag, 2);
  std::memcpy(buf + 22, &num_ch, 2);
  std::memcpy(buf + 24, &samp_rate, 4);
  std::memcpy(buf + 28, &byte_rate, 4);
  std::memcpy(buf + 32, &block_align, 2);
  std::memcpy(buf + 34, &bits, 2);
  std::memcpy(buf + 36, "data", 4);
  std::memcpy(buf + 40, &data_sz, 4);
  return 44;
}

AudioStream::AudioStream(ma_engine* engine, MidiPlayer* midi_player)
    : engine_(engine),
      midi_player_(midi_player),
      handle_({}),
      cursor_(0),
      looping_(false) {
  std::memset(&midi_decoder_, 0, sizeof(midi_decoder_));
}

AudioStream::~AudioStream() {
  ma_sound_uninit(&handle_);
  if (midi_decoder_.onRead) ma_decoder_uninit(&midi_decoder_);
  delete[] midi_wav_buf_;
}

ma_result AudioStream::Play(const std::string& filename,
                            int32_t volume,
                            int32_t pitch,
                            uint64_t pos) {
  if (filename.empty()) return MA_INVALID_ARGS;

  LOG(INFO) << "[Audio] Play: " << filename;

  // Fast path: known MIDI extension
  if (IsMIDIFile(filename)) return PlayMIDI(filename, volume, pitch, pos);

  if (!ma_sound_is_playing(&handle_) || filename_ != filename) {
    filename_ = filename;
    ma_sound_uninit(&handle_);

    ma_uint32 sound_flags = MA_SOUND_FLAG_ASYNC;
    if (looping_) sound_flags |= MA_SOUND_FLAG_LOOPING;
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

    if (result != MA_SUCCESS) return result;
  }

  ma_sound_set_volume(&handle_, volume / 100.0f);
  ma_sound_set_pitch(&handle_, pitch / 100.0f);
  if (pos) ma_sound_seek_to_pcm_frame(&handle_, pos);
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

  // Render MIDI to PCM buffer
  MidiPlayer::PCMResult pcm;
  if (!midi_player_->Render(filename, &pcm) || pcm.samples.empty()) {
    LOG(INFO) << "[Audio] PlayMIDI initial Render failed for: " << filename;
    // RGSS stores filenames without extension; try appending .mid
    if (filename.rfind('.') == std::string::npos) {
      std::string with_ext = filename + ".mid";
      LOG(INFO) << "[Audio] PlayMIDI retry with: " << with_ext;
      if (midi_player_->Render(with_ext, &pcm) && !pcm.samples.empty()) {
        LOG(INFO) << "[Audio] PlayMIDI retry succeeded";
        goto render_ok;
      } else {
        LOG(INFO) << "[Audio] PlayMIDI retry also failed";
      }
    }
    return MA_DOES_NOT_EXIST;
  }
render_ok:
  LOG(INFO) << "[Audio] PlayMIDI Render OK, samples=" << pcm.samples.size();

  // Build in-memory WAV
  size_t data_bytes = pcm.samples.size() * sizeof(float);
  size_t wav_bytes = data_bytes + 44;
  auto* wav_buf = new uint8_t[wav_bytes];
  WriteWAVHeader(wav_buf, data_bytes, pcm.sample_rate, pcm.channels);
  std::memcpy(wav_buf + 44, pcm.samples.data(), data_bytes);

  // Uninit previous MIDI decoder/sound if any
  if (midi_decoder_.onRead) ma_decoder_uninit(&midi_decoder_);
  ma_sound_uninit(&handle_);

  // Release previous WAV buffer
  delete[] midi_wav_buf_;
  midi_wav_buf_ = nullptr;

  // Store PCM buffer (keeps memory alive until next PlayMIDI or destructor)
  midi_pcm_ = std::move(pcm.samples);

  // Init decoder from memory WAV
  ma_decoder_config dec_cfg = ma_decoder_config_init(ma_format_f32, 0, 0);
  auto result =
      ma_decoder_init_memory(wav_buf, wav_bytes, &dec_cfg, &midi_decoder_);
  if (result != MA_SUCCESS) {
    delete[] wav_buf;
    return result;
  }

  // Transfer ownership — miniaudio decoder holds a raw pointer into wav_buf
  midi_wav_buf_ = wav_buf;

  // Init sound from decoder
  ma_uint32 sound_flags = MA_SOUND_FLAG_ASYNC;
  if (looping_) sound_flags |= MA_SOUND_FLAG_LOOPING;
  result = ma_sound_init_from_data_source(engine_, &midi_decoder_,
                                          sound_flags, nullptr, &handle_);
  if (result != MA_SUCCESS) {
    ma_decoder_uninit(&midi_decoder_);
    std::memset(&midi_decoder_, 0, sizeof(midi_decoder_));
    delete[] midi_wav_buf_;
    midi_wav_buf_ = nullptr;
    return result;
  }

  ma_sound_set_volume(&handle_, volume / 100.0f);
  ma_sound_set_pitch(&handle_, pitch / 100.0f);
  if (pos) ma_sound_seek_to_pcm_frame(&handle_, pos);
  ma_sound_start(&handle_);
  return MA_SUCCESS;
}

void AudioStream::Stop() { ma_sound_stop(&handle_); }

void AudioStream::Fade(int32_t time) {
  ma_sound_stop_with_fade_in_milliseconds(&handle_, time);
}

uint64_t AudioStream::Pos() {
  ma_uint64 cursor;
  ma_sound_get_cursor_in_pcm_frames(&handle_, &cursor);
  return cursor;
}

bool AudioStream::IsPlaying() { return ma_sound_is_playing(&handle_); }

bool AudioStream::IsPausing() { return cursor_ > 0; }

void AudioStream::Pause() {
  ma_sound_get_cursor_in_pcm_frames(&handle_, &cursor_);
  ma_sound_stop(&handle_);
}

void AudioStream::Resume() {
  ma_sound_seek_to_pcm_frame(&handle_, cursor_);
  ma_sound_start(&handle_);
  cursor_ = 0;
}

bool AudioStream::IsLooping() { return looping_; }

void AudioStream::SetLooping(bool looping) { looping_ = looping; }

}  // namespace audioservice
