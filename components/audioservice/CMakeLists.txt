# content components
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# components source
#--------------------------------------------------------------------------------

set(COMPONENTS_AUDIOSERVICE_SRC
  audio_service.cc
  audio_service.h
)

add_library(components_audioservice STATIC ${COMPONENTS_AUDIOSERVICE_SRC})
urge_setup_target_compile_feature(components_audioservice)
target_link_libraries(components_audioservice PUBLIC
  engine_base
)
