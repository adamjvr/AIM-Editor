include(FetchContent)

# Prefer explicit/local verified JUCE trees before allowing network FetchContent.
# This keeps release builds reproducible and makes offline development simple.
if(NOT AIM_EDITOR_JUCE_PATH AND DEFINED ENV{AIM_EDITOR_JUCE_PATH} AND NOT "$ENV{AIM_EDITOR_JUCE_PATH}" STREQUAL "")
    set(AIM_EDITOR_JUCE_PATH "$ENV{AIM_EDITOR_JUCE_PATH}")
endif()

if(NOT AIM_EDITOR_JUCE_PATH AND EXISTS "${CMAKE_SOURCE_DIR}/.deps/JUCE/CMakeLists.txt")
    set(AIM_EDITOR_JUCE_PATH "${CMAKE_SOURCE_DIR}/.deps/JUCE")
endif()

if(AIM_EDITOR_JUCE_PATH)
    get_filename_component(AIM_EDITOR_JUCE_PATH "${AIM_EDITOR_JUCE_PATH}" ABSOLUTE)
    if(NOT EXISTS "${AIM_EDITOR_JUCE_PATH}/CMakeLists.txt")
        message(FATAL_ERROR "AIM_EDITOR_JUCE_PATH does not point to a JUCE source tree: ${AIM_EDITOR_JUCE_PATH}")
    endif()

    # Refuse an accidental JUCE version drift even when the caller points us at
    # an otherwise valid source tree. Release archives have no .git directory,
    # so the declared JUCE project version is the portable contract to check.
    file(READ "${AIM_EDITOR_JUCE_PATH}/CMakeLists.txt" _aim_juce_root_cmake)
    string(REGEX MATCH "project\\([ \t\r\n]*JUCE[ \t\r\n]+VERSION[ \t\r\n]+([0-9]+\\.[0-9]+\\.[0-9]+)"
           _aim_juce_project_match "${_aim_juce_root_cmake}")
    set(_aim_juce_local_version "${CMAKE_MATCH_1}")
    if(NOT _aim_juce_local_version)
        message(FATAL_ERROR "Could not determine JUCE version from ${AIM_EDITOR_JUCE_PATH}/CMakeLists.txt")
    endif()
    if(NOT _aim_juce_local_version STREQUAL AIM_EDITOR_JUCE_TAG)
        message(FATAL_ERROR
            "AIM Editor is pinned to JUCE ${AIM_EDITOR_JUCE_TAG}, but ${AIM_EDITOR_JUCE_PATH} declares JUCE ${_aim_juce_local_version}")
    endif()

    message(STATUS "Using local JUCE ${_aim_juce_local_version}: ${AIM_EDITOR_JUCE_PATH}")
    add_subdirectory("${AIM_EDITOR_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    # Tags can theoretically move. Network fallback therefore checks out the
    # exact commit corresponding to the pinned JUCE 9.0.1 release. The preferred
    # developer path remains tools/bootstrap_juce.*, which additionally verifies
    # the official release archive SHA-256.
    message(STATUS "Fetching JUCE ${AIM_EDITOR_JUCE_TAG} at ${AIM_EDITOR_JUCE_COMMIT}")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${AIM_EDITOR_JUCE_COMMIT}
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()
