# Components - audioservice
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# Source of audioservice component
#--------------------------------------------------------------------------------

set(COMPONENTS_AUDIOSERVICE_SRC
  audio_service.cc
  audio_service.h
  audio_stream.cc
  audio_stream.h
  midi_player.cc
  midi_player.h
  midi_stream_source.cc
  midi_stream_source.h
  sound_emit.cc
  sound_emit.h
)

add_library(components_audioservice STATIC ${COMPONENTS_AUDIOSERVICE_SRC})
target_link_libraries(components_audioservice
 PUBLIC
  engine_base
  components_filesystem
  miniaudio
)
# MIDI support via runtime FluidSynth loading (SDL_LoadObject).
# No compile-time dependency needed — gracefully degrades if library is absent.
