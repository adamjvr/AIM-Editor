include(FetchContent)

if(AIM_EDITOR_JUCE_PATH)
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
