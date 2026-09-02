#pragma once

#include "Core/AppSettings.h"
#include "Midi/IonMidiService.h"
#include "Midi/IonSysExCodec.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <memory>

namespace aim
{
class GlobalControlBar final : public juce::Component
{
public:
    explicit GlobalControlBar (IonMidiService& midiService);

    void paint (juce::Graphics&) override;
    void resized() override;

    void setHistoryAvailability (bool canUndo, bool canRedo);
    void setSelectedPage (int pageIndex);
    void restoreSession (const SessionSnapshot& snapshot);
    void captureSession (SessionSnapshot& snapshot) const;

    std::function<void (int)> onPageChanged;
    std::function<void()> onSysExToolsRequested;
    std::function<void()> onLibrarianRequested;
    std::function<void()> onHardwareToolsRequested;
    std::function<void (bool)> onLiveEditingChanged;
    std::function<void (int)> onMidiChannelChanged;
    std::function<void()> onUndoRequested;
    std::function<void()> onRedoRequested;
    std::function<void()> onPersistentContextChanged;

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
    std::array<std::unique_ptr<juce::TextButton>, 5> pageButtons;
    juce::Rectangle<int> pageTabsBounds;

    juce::TextButton requestPatch { "request patch" };
    juce::TextButton sysexTools { "sysex tools" };
    juce::TextButton updateEditBuffer { "update edit buffer" };
    juce::TextButton allNotesOff { "all notes off" };
    juce::TextButton librarian { "librarian" };
    juce::TextButton undo { "undo" };
    juce::TextButton redo { "redo" };
    juce::ToggleButton liveEdit { "live NRPN" };
    juce::TextButton settings { "refresh MIDI" };

    juce::Label status;

    juce::StringArray inputIdentifiers;
    juce::StringArray outputIdentifiers;
};
}
