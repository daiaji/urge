# Components - filesystem
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# Source of filesystem
#--------------------------------------------------------------------------------

set(COMPONENTS_FILESYSTEM_SRC
  io_service.cc
  io_service.h
)

add_library(components_filesystem STATIC ${COMPONENTS_FILESYSTEM_SRC})
target_link_libraries(components_filesystem
 PUBLIC
  engine_base
  physfs-static
)

if (ENABLE_ADMENRI_BUILD_VERSION)
  target_link_libraries(components_filesystem PRIVATE admenri_arb)
  target_compile_definitions(components_filesystem PRIVATE HAVE_ARB_ENCRYPTO_SUPPORT=1)
endif ()