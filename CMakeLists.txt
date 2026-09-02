cmake_minimum_required(VERSION 3.24)

project(AIMEditor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AIM_EDITOR_BUILD_TESTS "Build AIM Editor tests" ON)
set(AIM_EDITOR_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout. If empty, JUCE is fetched.")
set(AIM_EDITOR_JUCE_TAG "9.0.1" CACHE STRING "JUCE git tag used when fetching JUCE")

include(cmake/JUCE.cmake)

juce_add_binary_data(AIMEditorData
    HEADER_NAME BinaryData.h
    NAMESPACE AIMBinaryData
    SOURCES
        data/parameters.json
        data/protocol/ion-sysex.json
        data/protocol/ion-nrpn.json
        data/enums/modulation-sources.json
        data/enums/modulation-destinations.json
        data/enums/filter-types.json
        schemas/parameter-database.schema.json
        schemas/program.schema.json
        schemas/bank.schema.json
        schemas/midi-capture.schema.json
        schemas/ion-sysex.schema.json
        schemas/ion-nrpn.schema.json
)

add_library(AIMEditorCore STATIC
    Source/Core/ParameterDefinition.cpp
    Source/Core/ParameterDefinition.h
    Source/Core/ParameterFormatter.cpp
    Source/Core/ParameterFormatter.h
    Source/Core/ParameterRegistry.cpp
    Source/Core/ParameterRegistry.h
    Source/Core/IonProgram.cpp
    Source/Core/IonProgram.h
    Source/Core/ProgramJson.cpp
    Source/Core/ProgramJson.h
    Source/Core/ProgramState.cpp
    Source/Core/ProgramState.h
    Source/Core/RandomizerEngine.cpp
    Source/Core/RandomizerEngine.h
    Source/Core/ProgramBank.cpp
    Source/Core/ProgramBank.h
    Source/Core/BankJson.cpp
    Source/Core/BankJson.h
)

target_include_directories(AIMEditorCore PUBLIC Source)
target_link_libraries(AIMEditorCore
    PUBLIC
        juce::juce_core
        juce::juce_data_structures
)

target_compile_definitions(AIMEditorCore PUBLIC JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)

add_library(AIMEditorMidi STATIC
    Source/Midi/MidiCaptureEvent.cpp
    Source/Midi/MidiCaptureEvent.h
    Source/Midi/IonMidiService.cpp
    Source/Midi/IonMidiService.h
    Source/Midi/IonProtocol.cpp
    Source/Midi/IonProtocol.h
    Source/Midi/IonParameterTransmitter.cpp
    Source/Midi/IonParameterTransmitter.h
    Source/Midi/IonNrpnDecoder.cpp
    Source/Midi/IonNrpnDecoder.h
    Source/Midi/IonSysExCodec.cpp
    Source/Midi/IonSysExCodec.h
    Source/Midi/IonProgramDecoder.cpp
    Source/Midi/IonProgramDecoder.h
    Source/Midi/IonProgramEncoder.cpp
    Source/Midi/IonProgramEncoder.h
    Source/Midi/IonSyxFileCodec.cpp
    Source/Midi/IonSyxFileCodec.h
)

target_include_directories(AIMEditorMidi PUBLIC Source)
target_link_libraries(AIMEditorMidi
    PUBLIC
        AIMEditorCore
        juce::juce_audio_basics
        juce::juce_audio_devices
)

juce_add_gui_app(AIMEditor
    PRODUCT_NAME "AIM Editor"
    COMPANY_NAME "AIM Editor"
    BUNDLE_ID "org.aim-editor.app"
)

target_sources(AIMEditor PRIVATE
    Source/App/Main.cpp
    Source/App/MainWindow.cpp
    Source/App/MainWindow.h
    Source/UI/MainEditor.cpp
    Source/UI/MainEditor.h
    Source/UI/IonLookAndFeel.cpp
    Source/UI/IonLookAndFeel.h
    Source/UI/GlobalControlBar.cpp
    Source/UI/GlobalControlBar.h
    Source/UI/SysExInspector.cpp
    Source/UI/SysExInspector.h
    Source/UI/ProgramLibrarian.cpp
    Source/UI/ProgramLibrarian.h
    Source/UI/HardwareTransferPanel.cpp
    Source/UI/HardwareTransferPanel.h
    Source/UI/Components/SectionPanel.cpp
    Source/UI/Components/SectionPanel.h
    Source/UI/Components/OscillatorPanel.cpp
    Source/UI/Components/OscillatorPanel.h
    Source/UI/Components/FilterPanel.cpp
    Source/UI/Components/FilterPanel.h
    Source/UI/Components/EnvelopePanel.cpp
    Source/UI/Components/EnvelopePanel.h
    Source/UI/Components/ModulatorPanel.cpp
    Source/UI/Components/ModulatorPanel.h
    Source/UI/Components/VoicePanel.cpp
    Source/UI/Components/VoicePanel.h
    Source/UI/Components/EffectsPanel.cpp
    Source/UI/Components/EffectsPanel.h
    Source/UI/Components/MixerPanel.cpp
    Source/UI/Components/MixerPanel.h
    Source/UI/Components/OutputPanel.cpp
    Source/UI/Components/OutputPanel.h
    Source/UI/Components/ModMatrixPanel.cpp
    Source/UI/Components/ModMatrixPanel.h
    Source/UI/Components/RandomizerPanel.cpp
    Source/UI/Components/RandomizerPanel.h
    Source/UI/Components/TrackingGeneratorPanel.cpp
    Source/UI/Components/TrackingGeneratorPanel.h
    Source/UI/Panels/EditorPage.cpp
    Source/UI/Panels/EditorPage.h
)

target_include_directories(AIMEditor PRIVATE Source)
target_link_libraries(AIMEditor
    PRIVATE
        AIMEditorCore
        AIMEditorMidi
        AIMEditorData
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(AIMEditor PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:AIMEditor,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

if(APPLE)
    set_target_properties(AIMEditor PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(AIMEditor PROPERTIES
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "2"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "17.0"
        XCODE_ATTRIBUTE_SUPPORTS_MACCATALYST NO
    )
endif()

if(AIM_EDITOR_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(AIMEditorTests PRODUCT_NAME "AIM Editor Tests")
    target_sources(AIMEditorTests PRIVATE Tests/CoreTests.cpp)
    target_include_directories(AIMEditorTests PRIVATE Source)
    target_link_libraries(AIMEditorTests PRIVATE AIMEditorCore AIMEditorMidi AIMEditorData juce::juce_core juce::juce_data_structures)
    add_test(NAME AIMEditorCoreTests COMMAND AIMEditorTests)
endif()
