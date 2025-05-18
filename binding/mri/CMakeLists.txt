# Binding - MRI
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# Autogen MRI binding source collection
#--------------------------------------------------------------------------------

set(PYTHON_SCRIPT "${PROJECT_SOURCE_DIR}/binding/mri/tools/autogen.py")
set(INPUT_BINDING_DIR "${PROJECT_SOURCE_DIR}/content/public/")
set(GENERATE_BINDING_DIR "${RGU_AUTOGEN_DIR}/binding/mri/")

execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -B "${PYTHON_SCRIPT}" "${INPUT_BINDING_DIR}" "${GENERATE_BINDING_DIR}"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  RESULT_VARIABLE SCRIPT_RESULT
)

if(NOT SCRIPT_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to create MRI binding source: ${SCRIPT_RESULT}")
endif()

file(GLOB GENERATED_SOURCES CONFIGURE_DEPENDS "${GENERATE_BINDING_DIR}/*.cc")
file(GLOB GENERATED_HEADERS CONFIGURE_DEPENDS "${GENERATE_BINDING_DIR}/*.h")

set(BINDING_MRI_SRC
  mri_file.cc
  mri_file.h
  mri_main.cc
  mri_main.h
  mri_util.cc
  mri_util.h

  ${GENERATED_SOURCES}
  ${GENERATED_HEADERS}

  binding_patch.cc
  binding_patch.h

  zlib_binding.c
)

add_library(binding_mri STATIC ${BINDING_MRI_SRC})
target_link_libraries(binding_mri
 PUBLIC
  engine_base
  engine_content
  engine_renderer
  mri_ruby
  zlibstatic
)

if (ENABLE_ADMENRI_BUILD_VERSION)
  target_link_libraries(binding_mri PRIVATE admenri_arb)
  target_compile_definitions(binding_mri PRIVATE HAVE_GET_MACHINE_HASH=1)
endif ()
