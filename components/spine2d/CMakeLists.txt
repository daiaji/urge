# Components - spine2d renderer
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# Source of limiter
#--------------------------------------------------------------------------------

set(COMPONENTS_SPINE2D_SRC
  skeleton_renderer.cc
  skeleton_renderer.h
)

add_library(components_spine2drenderer ${COMPONENTS_SPINE2D_SRC})
target_link_libraries(components_spine2drenderer engine_base engine_renderer)
