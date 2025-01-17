# Main entry
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# Player entry source
#--------------------------------------------------------------------------------

set(BINDING_UNITTESTS_SRC
  engine_binding_unittests.cc
  engine_binding_unittests.h
)

add_library(binding_unittests ${BINDING_UNITTESTS_SRC})
target_link_libraries(binding_unittests PUBLIC engine_base engine_content engine_renderer)
