cmake_minimum_required(VERSION 3.20.1)

project(URGE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
include_directories("${PROJECT_SOURCE_DIR}")

#--------------------------------------------------------------------------------
# Platform and compiler check
#--------------------------------------------------------------------------------

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
  link_libraries(
    "shell32;ws2_32;iphlpapi;imagehlp;shlwapi;bcrypt"
    "setupapi;advapi32;uuid;version;oleaut32;ole32;imm32;winmm;user32;rpcrt4;usp10"
  )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  link_libraries(crypt)
endif ()

#--------------------------------------------------------------------------------
# Auto-generated directory include path
#--------------------------------------------------------------------------------

find_package(Python3 COMPONENTS Interpreter)

if (Python3_FOUND)
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    message(STATUS "Found Python3: ${Python3_EXECUTABLE}")
else ()
    message(FATAL_ERROR "Python3 not found.")
endif ()

set(RGU_AUTOGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen")
include_directories("${RGU_AUTOGEN_DIR}")

#-------------------------------------------------------------------------
# * third party libraries
#-------------------------------------------------------------------------

include_directories(base/third_party)
include_directories(third_party)

### Android compile configure
set(SDL_STATIC on)
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(SDL_STATIC off)
endif ()

## SDL
add_subdirectory(third_party/SDL)
include_directories(third_party/SDL/include)

## image loader (SDL_image)
add_subdirectory(third_party/image_loader)
include_directories(third_party/image_loader/include/SDL3_image)

## text render (SDL_ttf)
add_subdirectory(third_party/freetype)
include_directories(third_party/freetype/include)
add_subdirectory(third_party/text_render)
include_directories(third_party/text_render/include/SDL3_ttf)

## WebGPU Renderer
set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory(third_party/dawn)

#-------------------------------------------------------------------------
# * source
#-------------------------------------------------------------------------

add_subdirectory(components/filesystem)
add_subdirectory(components/audioservice)
add_subdirectory(binding/unittests)
add_subdirectory(base)
add_subdirectory(ui)
add_subdirectory(renderer)
add_subdirectory(content)
add_subdirectory(app)
add_subdirectory(unittests)
