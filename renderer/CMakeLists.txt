# renderer components
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# components source
#--------------------------------------------------------------------------------

set(ENGINE_RENDERER_SRC
  context/device_context.cc
  context/device_context.h
  device/render_device.cc
  device/render_device.h
  pipeline/binding_layout.cc
  pipeline/binding_layout.h
  pipeline/builtin_wgsl.cc
  pipeline/builtin_wgsl.h
  pipeline/render_pipeline.cc
  pipeline/render_pipeline.h
  render/render_pass.cc
  render/render_pass.h
  resource/render_buffer.cc
  resource/render_buffer.h
  utils/buffer_utils.cc
  utils/buffer_utils.h
  utils/texture_utils.cc
  utils/texture_utils.h
  vertex/vertex_layout.cc
  vertex/vertex_layout.h
)

add_library(engine_renderer STATIC ${ENGINE_RENDERER_SRC})
target_compile_definitions(engine_renderer PRIVATE WGPU_DAWN_NATIVE)
target_link_libraries(engine_renderer PUBLIC
  engine_base
  engine_ui
  
  webgpu_cpp
  dawn::dawn_common
  dawn::dawn_system_utils
  dawn::dawn_wgpu_utils
  dawn::dawn_proc
  dawn::dawn_glfw
  dawn::dawn_native
)
