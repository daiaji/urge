cmake_minimum_required(VERSION 3.20.1)

set(BINDING_MRI_SRC
  mri_file.cc
  mri_file.h
  mri_main.cc
  mri_main.h
  mri_util.cc
  mri_util.h
  urge_binding.cc
  urge_binding.h

  zlib_binding.c
)

add_library(binding_mri STATIC ${BINDING_MRI_SRC})
target_link_directories(binding_mri PUBLIC
  engine_base
  engine_content
  mri_ruby
)
