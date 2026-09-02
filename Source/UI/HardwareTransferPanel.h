#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "Midi/IonMidiService.h"
#include "Midi/IonProgramEncoder.h"
#include "Midi/IonSysExCodec.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace aim
{
/** Explicit hardware-transfer surface for candidate Ion SysEx operations.

    Full program writes are deliberately gated behind an arm toggle and a real
    378-byte source template. AIM Editor never fabricates unknown patch bytes.
*/
class HardwareTransferPanel final : public juce::Component,
                                    private ProgramState::Listener
{
public:
    HardwareTransferPanel (IonMidiService& midiService,
                           const ParameterRegistry& parameterRegistry,
                           ProgramState& programState);
    ~HardwareTransferPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void()> onClose;

private:
    void parameterValueChanged (std::string_view, const juce::var&, ProgramChangeOrigin) override {}
    void programReplaced (ProgramChangeOrigin) override { updateState(); }
    void programMetadataChanged (ProgramChangeOrigin) override { updateState(); }

    void refreshProgramSelector();
    void requestSelectedPatch();
    void requestSelectedBank();
    void sendCurrentToEditBuffer();
    void updateState();
    [[nodiscard]] bool hasPatchTemplate() const noexcept;

    IonMidiService& midi;
    const ParameterRegistry& registry;
    ProgramState& state;

    juce::Label title;
    juce::Label warning;
    juce::Label sourceStatus;
    juce::Label transferStatus;
    juce::Label bankLabel;
    juce::Label programLabel;
    juce::Label editSlotLabel;

    juce::ComboBox bank;
    juce::ComboBox program;
    juce::ComboBox editSlot;

    juce::TextButton requestPatch { "Request Patch" };
    juce::TextButton requestBank { "Request Bank" };
    juce::ToggleButton armWrites { "Arm candidate full-patch writes" };
    juce::TextButton sendEditBuffer { "Send Current to Edit Buffer" };
    juce::TextButton closeButton { "Close" };
};
}
