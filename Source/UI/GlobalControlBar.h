#pragma once

#include "Midi/IonMidiService.h"
#include "Midi/IonSysExCodec.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace aim
{
class GlobalControlBar final : public juce::Component
{
public:
    explicit GlobalControlBar (IonMidiService& midiService);

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void (int)> onPageChanged;
    std::function<void()> onSysExToolsRequested;
    std::function<void (bool)> onLiveEditingChanged;
    std::function<void (int)> onMidiChannelChanged;

private:
    void refreshMidiDevices();
    void selectMidiInput();
    void selectMidiOutput();
    void sendAllNotesOff();
    void requestCurrentPatch();
    void refreshProgramSelector();

    IonMidiService& midi;

    juce::ComboBox midiInput;
    juce::ComboBox midiBank;
    juce::ComboBox programSelector;
    juce::ComboBox midiOutput;
    juce::ComboBox midiChannel;
    juce::ComboBox pageSelector;

    juce::TextButton requestPatch { "request patch" };
    juce::TextButton sysexTools { "sysex tools" };
    juce::TextButton updateEditBuffer { "update edit buffer" };
    juce::TextButton allNotesOff { "all notes off" };
    juce::ToggleButton liveEdit { "live NRPN" };
    juce::TextButton settings { "settings" };

    juce::Label status;

    juce::StringArray inputIdentifiers;
    juce::StringArray outputIdentifiers;
};
}
