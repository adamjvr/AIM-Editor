#pragma once

#include "Midi/IonMidiService.h"

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

private:
    void refreshMidiDevices();
    void selectMidiInput();
    void selectMidiOutput();
    void sendAllNotesOff();

    IonMidiService& midi;

    juce::ComboBox midiInput;
    juce::ComboBox programSelector;
    juce::ComboBox midiOutput;
    juce::ComboBox pageSelector;

    juce::TextButton requestPatch { "request patch" };
    juce::TextButton sysexTools { "sysex tools" };
    juce::TextButton updateEditBuffer { "update edit buffer" };
    juce::TextButton allNotesOff { "all notes off" };
    juce::TextButton settings { "settings" };

    juce::Label status;

    juce::StringArray inputIdentifiers;
    juce::StringArray outputIdentifiers;
};
}
