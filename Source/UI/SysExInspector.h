#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramJson.h"
#include "Midi/IonMidiService.h"
#include "Midi/IonSysExCodec.h"
#include "Midi/IonProgramDecoder.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

namespace aim
{
class SysExInspector final : public juce::Component
{
public:
    SysExInspector (IonMidiService& midiService, const ParameterRegistry& parameterRegistry);
    ~SysExInspector() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void()> onClose;

private:
    void addEventOnMessageThread (MidiCaptureEvent event);
    void rebuildLog();
    void clearCapture();
    void copyJsonToClipboard();
    void saveJson();
    [[nodiscard]] juce::String makeCaptureJson() const;
    [[nodiscard]] juce::String makeLogLine (const MidiCaptureEvent& event) const;

    static constexpr std::size_t maxEvents = 4096;

    IonMidiService& midi;
    const ParameterRegistry& registry;
    std::vector<MidiCaptureEvent> events;

    juce::Label title;
    juce::Label summary;
    juce::TextEditor log;
    juce::ToggleButton sysexOnly { "SysEx only" };
    juce::TextButton clearButton { "Clear" };
    juce::TextButton copyButton { "Copy JSON" };
    juce::TextButton saveButton { "Save JSON" };
    juce::TextButton closeButton { "Close" };
    std::unique_ptr<juce::FileChooser> fileChooser;
};
}
