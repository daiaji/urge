# Spine2d renderer
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# Spine2D renderer
#--------------------------------------------------------------------------------

set(COMPONENTS_SPINE2D_SRC
  spine_renderer.cc
  spine_renderer.h
)

add_library(components_spine2drenderer ${COMPONENTS_SPINE2D_SRC})
target_link_libraries(components_spine2drenderer
  PUBLIC
    engine_base
    engine_renderer
    components_filesystem
)
