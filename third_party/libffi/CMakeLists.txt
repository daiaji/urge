cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# libffi autogen configure
#--------------------------------------------------------------------------------

enable_language(ASM_MASM)

configure_file("${CMAKE_SOURCE_DIR}/include/ffi.h.in" "${RGU_AUTOGEN_DIR}/include/ffi.h")
configure_file("${CMAKE_SOURCE_DIR}/include/fficonfig.h.in" "${RGU_AUTOGEN_DIR}/include/fficonfig.h")
