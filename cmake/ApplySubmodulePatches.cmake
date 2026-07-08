#--------------------------------------------------------------------------------------------------
# ApplySubmodulePatches.cmake
#
# Applies local patches from ${PATCH_DIR}/*.patch to a submodule working tree at
# ${SOURCE_DIR}, idempotently, at CMake configure time.
#
# Usage:
#   include(${CMAKE_SOURCE_DIR}/cmake/ApplySubmodulePatches.cmake)
#   apply_submodule_patches(
#       SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/DiligentCore
#       PATCH_DIR  ${CMAKE_SOURCE_DIR}/patches/DiligentCore
#   )
#
# Behaviour:
#   - For each *.patch under PATCH_DIR (sorted):
#       1. Probe with `git apply --check -v` (does not modify files).
#          If the probe succeeds, the patch has not been applied yet -> apply it.
#          If the probe fails, assume the patch is already applied -> skip with a STATUS line.
#       2. On apply failure that is NOT "already applied", emit FATAL_ERROR with diagnostics.
#   - Requires git on PATH (found via find_package(Git)).
#   - Cross-platform: only depends on the `git` executable, which is present on
#     Windows (Git for Windows / Visual Studio) and Unix.
#--------------------------------------------------------------------------------------------------

find_package(Git QUIET)
if(NOT GIT_FOUND)
    message(FATAL_ERROR "git not found on PATH; required to apply submodule patches.")
endif()

function(apply_submodule_patches)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;PATCH_DIR" "" ${ARGN})

    if(NOT ARG_SOURCE_DIR)
        message(FATAL_ERROR "apply_submodule_patches: SOURCE_DIR is required.")
    endif()
    if(NOT ARG_PATCH_DIR)
        message(FATAL_ERROR "apply_submodule_patches: PATCH_DIR is required.")
    endif()
    if(NOT EXISTS "${ARG_SOURCE_DIR}/.git")
        message(STATUS "apply_submodule_patches: ${ARG_SOURCE_DIR} is not a git worktree, skipping.")
        return()
    endif()

    file(GLOB PATCH_FILES "${ARG_PATCH_DIR}/*.patch")
    list(SORT PATCH_FILES)

    foreach(PATCH_FILE IN LISTS PATCH_FILES)
        get_filename_component(PATCH_NAME "${PATCH_FILE}" NAME)

        # Probe: would `git apply --check` succeed? If yes, the patch is not yet applied.
        execute_process(
            COMMAND ${GIT_EXECUTABLE} apply --check -v "${PATCH_FILE}"
            WORKING_DIRECTORY "${ARG_SOURCE_DIR}"
            RESULT_VARIABLE PROBE_RESULT
            OUTPUT_VARIABLE PROBE_OUTPUT
            ERROR_VARIABLE PROBE_ERROR
        )

        if(PROBE_RESULT EQUAL 0)
            # Not yet applied -> apply for real.
            execute_process(
                COMMAND ${GIT_EXECUTABLE} apply -v "${PATCH_FILE}"
                WORKING_DIRECTORY "${ARG_SOURCE_DIR}"
                RESULT_VARIABLE APPLY_RESULT
                OUTPUT_VARIABLE APPLY_OUTPUT
                ERROR_VARIABLE APPLY_ERROR
            )
            if(APPLY_RESULT EQUAL 0)
                message(STATUS "Submodule patch applied: ${PATCH_NAME}")
            else()
                message(FATAL_ERROR
                    "Failed to apply submodule patch '${PATCH_NAME}':\n"
                    "stdout: ${APPLY_OUTPUT}\n"
                    "stderr: ${APPLY_ERROR}")
            endif()
        else()
            # Probe failed: most likely already applied (context mismatch / reversed).
            # Distinguish "already applied" from a genuinely broken patch by checking
            # if `git apply --check -R` (reverse) succeeds.
            execute_process(
                COMMAND ${GIT_EXECUTABLE} apply --check -R "${PATCH_FILE}"
                WORKING_DIRECTORY "${ARG_SOURCE_DIR}"
                RESULT_VARIABLE REV_PROBE_RESULT
                OUTPUT_QUIET ERROR_QUIET
            )
            if(REV_PROBE_RESULT EQUAL 0)
                message(STATUS "Submodule patch already applied: ${PATCH_NAME}")
            else()
                message(FATAL_ERROR
                    "Submodule patch '${PATCH_NAME}' does not apply cleanly and is not "
                    "detected as already applied. Manual resolution required.\n"
                    "Probe stdout: ${PROBE_OUTPUT}\n"
                    "Probe stderr: ${PROBE_ERROR}")
            endif()
        endif()
    endforeach()
endfunction()
