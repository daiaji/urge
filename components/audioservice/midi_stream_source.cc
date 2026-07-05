#include "components/audioservice/midi_stream_source.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "base/debug/logging.h"
#include "components/audioservice/midi_player.h"

namespace audioservice {

namespace {

constexpr int kFluidPlayerDone = 2;
constexpr int kSeekDiscardFrames = 512;

}

ma_data_source_vtable MidiStreamSource::k_VTable = {
    .onRead = MidiStreamSource::OnRead,
    .onSeek = MidiStreamSource::OnSeek,
    .onGetDataFormat = MidiStreamSource::OnGetDataFormat,
    .onGetCursor = MidiStreamSource::OnGetCursor,
    .onSetLooping = MidiStreamSource::OnSetLooping,
    .flags = MA_DATA_SOURCE_SELF_MANAGED_RANGE_AND_LOOP_POINT,
};

MidiStreamSource::MidiStreamSource(MidiPlayer* owner, void* synth,
                                   void* player, void* settings,
                                   std::vector<char> midi_data,
                                   int sample_rate, int channels)
    : owner(owner),
      synth(synth),
      player(player),
      settings(settings),
      sample_rate(sample_rate),
      channels(channels),
      cursor(0),
      has_ended(false),
      midi_data(std::move(midi_data)) {
  ma_data_source_config config = ma_data_source_config_init();
  config.vtable = &k_VTable;
  ma_data_source_init(&config, &base);
}

MidiStreamSource::~MidiStreamSource() {
  ma_data_source_uninit(&base);
  if (player)
    owner->FluidDeletePlayer(player);
  if (synth)
    owner->FluidDeleteSynth(synth);
  if (settings)
    owner->FluidDeleteSettings(settings);
}

ma_result MidiStreamSource::OnRead(ma_data_source* ds, void* frames_out,
                                   ma_uint64 frame_count,
                                   ma_uint64* frames_read) {
  auto* self = reinterpret_cast<MidiStreamSource*>(
      reinterpret_cast<char*>(ds) - offsetof(MidiStreamSource, base));

  if (self->has_ended) {
    *frames_read = 0;
    return MA_AT_END;
  }

  // Render from FluidSynth directly into miniaudio's buffer
  float* out = static_cast<float*>(frames_out);
  int status = self->owner->FluidPlayerGetStatus(self->player);
  if (status == kFluidPlayerDone) {
    // Final flush: render any remaining samples
    const int kBlockFrames = 512;
    float block[2 * kBlockFrames];
    ma_uint64 rendered = 0;
    while (rendered < frame_count) {
      ma_uint64 todo = frame_count - rendered;
      if (todo > kBlockFrames)
        todo = kBlockFrames;
      int n = static_cast<int>(todo);
      self->owner->FluidSynthWriteFloat(self->synth, n, block, 0, 2, block, 1,
                                         2);
      std::memcpy(out + rendered * 2, block, n * 2 * sizeof(float));
      self->cursor += n;
      rendered += n;
    }
    self->has_ended = true;
    *frames_read = rendered;
    return MA_AT_END;
  }

  int n = static_cast<int>(frame_count);
  self->owner->FluidSynthWriteFloat(self->synth, n, out, 0, 2, out, 1, 2);
  self->cursor += n;
  *frames_read = frame_count;
  return MA_SUCCESS;
}

ma_result MidiStreamSource::OnSeek(ma_data_source* ds, ma_uint64 frame_index) {
  // MIDI cannot seek to arbitrary positions; reset to start
  auto* self = reinterpret_cast<MidiStreamSource*>(
      reinterpret_cast<char*>(ds) - offsetof(MidiStreamSource, base));
  self->owner->FluidSynthSystemReset(self->synth);
  if (self->player)
    self->owner->FluidDeletePlayer(self->player);
  self->player = self->owner->FluidNewPlayer(self->synth);
  if (!self->player ||
      self->owner->FluidPlayerAddMem(self->player, self->midi_data.data(),
                                     self->midi_data.size()) != 0 ||
      self->owner->FluidPlayerPlay(self->player) != 0) {
    LOG(ERROR) << "[MIDI] Failed to restart FluidSynth player";
    return MA_ERROR;
  }
  self->cursor = 0;
  self->has_ended = false;
  float discard[2 * kSeekDiscardFrames];
  ma_uint64 remaining = frame_index;
  while (remaining > 0) {
    int frames = static_cast<int>(std::min<ma_uint64>(remaining,
                                                      kSeekDiscardFrames));
    self->owner->FluidSynthWriteFloat(self->synth, frames, discard, 0, 2,
                                      discard, 1, 2);
    self->cursor += frames;
    remaining -= frames;
    if (self->owner->FluidPlayerGetStatus(self->player) == kFluidPlayerDone) {
      self->has_ended = true;
      break;
    }
  }
  return MA_SUCCESS;
}

ma_result MidiStreamSource::OnGetDataFormat(ma_data_source* ds, ma_format* fmt,
                                            ma_uint32* ch, ma_uint32* rate,
                                            ma_channel* chmap,
                                            size_t chmap_cap) {
  auto* self = reinterpret_cast<MidiStreamSource*>(
      reinterpret_cast<char*>(ds) - offsetof(MidiStreamSource, base));
  *fmt = ma_format_f32;
  *ch = static_cast<ma_uint32>(self->channels);
  *rate = static_cast<ma_uint32>(self->sample_rate);
  if (chmap_cap > 0)
    chmap[0] = MA_CHANNEL_FRONT_LEFT;
  if (chmap_cap > 1)
    chmap[1] = MA_CHANNEL_FRONT_RIGHT;
  return MA_SUCCESS;
}

ma_result MidiStreamSource::OnGetCursor(ma_data_source* ds,
                                        ma_uint64* cursor_out) {
  auto* self = reinterpret_cast<MidiStreamSource*>(
      reinterpret_cast<char*>(ds) - offsetof(MidiStreamSource, base));
  *cursor_out = self->cursor;
  return MA_SUCCESS;
}

ma_result MidiStreamSource::OnSetLooping(ma_data_source* ds,
                                         ma_bool32 looping) {
  // Looping is handled by ma_data_source_base via the
  // SELF_MANAGED_RANGE_AND_LOOP_POINT flag: miniaudio will call OnRead again
  // after OnRead returns MA_AT_END, seeking back via OnSeek internally.
  return MA_SUCCESS;
}

}  // namespace audioservice
