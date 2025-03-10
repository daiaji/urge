# Main entry
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# Player entry source
#--------------------------------------------------------------------------------

set(URGE_DEP_LIBS
  engine_base
  engine_content
  engine_renderer
  engine_ui

  binding_mri
)

add_executable(console_engine urge_main.cc resources/app.rc)
target_link_libraries(console_engine PUBLIC ${URGE_DEP_LIBS})
