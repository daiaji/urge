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
  components/disposable.cc
  components/disposable.h
  components/font_context.cc
  components/font_context.h
  components/sprite_batch.cc
  components/sprite_batch.h
  composite/tilequad.cc
  composite/tilequad.h
  common/value_observer.h
  context/exception_state.cc
  context/exception_state.h
  context/execution_context.cc
  context/execution_context.h
  input/keyboard_controller.cc
  input/keyboard_controller.h
  input/mouse_controller.cc
  input/mouse_controller.h
  media/audio_impl.cc
  media/audio_impl.h
  profile/command_ids.h
  profile/content_profile.cc
  profile/content_profile.h
  profile/i18n_profile.cc
  profile/i18n_profile.h
  public/engine_audio.h
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
  public/engine_tilemapautotile.h
  public/engine_tilemapbitmap.h
  public/engine_tilemap2.h
  public/engine_tone.h
  public/engine_viewport.h
  public/engine_window.h
  public/engine_window2.h
  render/drawable_controller.cc
  render/drawable_controller.h
  render/plane_impl.cc
  render/plane_impl.h
  render/sprite_impl.cc
  render/sprite_impl.h
  render/tilemap_impl.cc
  render/tilemap_impl.h
  render/tilemap2_impl.cc
  render/tilemap2_impl.h
  render/window_impl.cc
  render/window_impl.h
  render/window2_impl.cc
  render/window2_impl.h
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

#--------------------------------------------------------------------------------
# Embeded program resource
#--------------------------------------------------------------------------------

execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/buildtools/build_file_binary.py" "${PROJECT_SOURCE_DIR}/content/resource/embed.ttf" -o "${RGU_AUTOGEN_DIR}/content/resource/embed.ttf.bin"
  OUTPUT_QUIET
)

add_library(engine_content STATIC ${ENGINE_CONTENT_SRC})
target_link_libraries(engine_content PUBLIC
  engine_base
  engine_ui
  engine_renderer
  components_filesystem
  sdl_image_loader
  sdl_ttf_render
  ocornut_imgui
  soloud
)
