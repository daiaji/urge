# Main imgui
cmake_minimum_required(VERSION 3.21.0)

#--------------------------------------------------------------------------------
# Source of IMGUI
#--------------------------------------------------------------------------------

set(IMGUI_SOURCE
  imgui.cpp
  imgui_demo.cpp
  imgui_draw.cpp
  imgui_freetype.cpp
  imgui_tables.cpp
  imgui_widgets.cpp

  imgui_impl_sdl3.cpp
  imgui_impl_sdl3.h
  imgui_impl_wgpu.cpp
  imgui_impl_wgpu.h
)

add_library(engine_imgui ${IMGUI_SOURCE})
target_link_libraries(engine_imgui PUBLIC engine_renderer)
