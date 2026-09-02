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
    message(STATUS "Using local JUCE: ${AIM_EDITOR_JUCE_PATH}")
    add_subdirectory("${AIM_EDITOR_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    message(STATUS "Fetching JUCE ${AIM_EDITOR_JUCE_TAG}")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${AIM_EDITOR_JUCE_TAG}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()
