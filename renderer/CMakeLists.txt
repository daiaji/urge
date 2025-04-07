# renderer components
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# components source
#--------------------------------------------------------------------------------

set(ENGINE_RENDERER_SRC
  device/render_device.cc
  device/render_device.h
  pipeline/builtin_hlsl.cc
  pipeline/builtin_hlsl.h
  pipeline/render_binding.cc
  pipeline/render_binding.h
  pipeline/render_pipeline.cc
  pipeline/render_pipeline.h
  resource/render_buffer.cc
  resource/render_buffer.h
  vertex/vertex_layout.cc
  vertex/vertex_layout.h
)

add_library(engine_renderer STATIC ${ENGINE_RENDERER_SRC})
target_link_libraries(engine_renderer PUBLIC
  engine_base
  engine_ui
  
  Diligent-GraphicsEngine
  Diligent-GraphicsEngineOpenGL-static
  Diligent-TextureLoader
)
