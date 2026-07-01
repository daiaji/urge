#ifndef COMPONENTS_AUDIOSERVICE_MIDI_STREAM_SOURCE_H_
#define COMPONENTS_AUDIOSERVICE_MIDI_STREAM_SOURCE_H_

#include <string>
#include <vector>

#include "miniaudio.h"

namespace audioservice {

class MidiPlayer;

struct MidiStreamSource {
  ma_data_source_base base;

  MidiPlayer* owner;
  void* synth;
  void* player;
  void* settings;

  int sample_rate;
  int channels;
  ma_uint64 cursor;
  bool has_ended;

  MidiStreamSource(MidiPlayer* owner, void* synth, void* player,
                   void* settings, int sample_rate, int channels);
  ~MidiStreamSource();

  MidiStreamSource(const MidiStreamSource&) = delete;
  MidiStreamSource& operator=(const MidiStreamSource&) = delete;

  static ma_data_source_vtable k_VTable;

 private:
  static ma_result OnRead(ma_data_source* ds, void* frames_out,
                          ma_uint64 frame_count, ma_uint64* frames_read);
  static ma_result OnSeek(ma_data_source* ds, ma_uint64 frame_index);
  static ma_result OnGetDataFormat(ma_data_source* ds, ma_format* fmt,
                                   ma_uint32* ch, ma_uint32* rate,
                                   ma_channel* chmap, size_t chmap_cap);
  static ma_result OnGetCursor(ma_data_source* ds, ma_uint64* cursor);
  static ma_result OnSetLooping(ma_data_source* ds, ma_bool32 looping);
};

}  // namespace audioservice

#endif  // !COMPONENTS_AUDIOSERVICE_MIDI_STREAM_SOURCE_H_
