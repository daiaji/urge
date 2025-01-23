# content components
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# components source
#--------------------------------------------------------------------------------

set(ENGINE_CONTENT_SRC
  canvas/canvas_impl.cc
  canvas/canvas_impl.h
  canvas/canvas_scheduler.cc
  canvas/canvas_scheduler.h
  canvas/font_impl.cc
  canvas/font_impl.h
  common/color_impl.cc
  common/color_impl.h
  common/rect_impl.cc
  common/rect_impl.h
  common/table_impl.cc
  common/table_impl.h
  common/tone_impl.cc
  common/tone_impl.h
  common/value_observer.h
  context/exception_state.cc
  context/exception_state.h
  context/execution_context.cc
  context/execution_context.h
  profile/content_profile.cc
  profile/content_profile.h
  public/engine_bitmap.h
  public/engine_color.h
  public/engine_font.h
  public/engine_graphics.h
  public/engine_input.h
  public/engine_plane.h
  public/engine_rect.h
  public/engine_sprite.h
  public/engine_table.h
  public/engine_tilemap.h
  public/engine_tone.h
  public/engine_viewport.h
  public/engine_window.h
  render/drawable_controller.cc
  render/drawable_controller.h
  render/sprite_impl.cc
  render/sprite_impl.h
  screen/renderscreen_impl.cc
  screen/renderscreen_impl.h
  screen/viewport_impl.cc
  screen/viewport_impl.h
  worker/content_runner.cc
  worker/content_runner.h
  worker/coroutine_context.h
  worker/engine_binding.cc
  worker/engine_binding.h
  content_config.h
)

add_library(engine_content STATIC ${ENGINE_CONTENT_SRC})
urge_setup_target_compile_feature(engine_content)
target_link_libraries(engine_content PUBLIC
  engine_base
  engine_ui
  engine_renderer
  components_filesystem
  sdl_image_loader
  sdl_ttf_render
)
