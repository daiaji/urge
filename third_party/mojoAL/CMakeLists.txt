cmake_minimum_required(VERSION 3.12.0)

add_library(mojoal STATIC mojoal.c)
target_include_directories(mojoal PRIVATE AL)
target_link_libraries(mojoal PUBLIC engine_base)
