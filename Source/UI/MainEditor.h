#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "Midi/IonMidiService.h"
#include "Midi/IonParameterTransmitter.h"
#include "Midi/IonNrpnDecoder.h"
#include "UI/GlobalControlBar.h"
#include "UI/Panels/EditorPage.h"
#include "UI/ProgramLibrarian.h"
#include "UI/HardwareTransferPanel.h"
#include "UI/SysExInspector.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

namespace aim
{
class MainEditor final : public juce::Component
{
public:
    MainEditor (const ParameterRegistry& registry, IonMidiService& midiService);
    ~MainEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showPage (int pageIndex);
    void updateViewedPageSize();
    void showSysExInspector();
    void hideSysExInspector();
    void showProgramLibrarian();
    void hideProgramLibrarian();
    void showHardwareTools();
    void hideHardwareTools();
    void applyIncomingNrpn (const DecodedNrpn& decoded);

    const ParameterRegistry& registry;
    IonMidiService& midi;
    ProgramState programState;
    IonNrpnDecoder nrpnDecoder;
    IonParameterTransmitter parameterTransmitter;
    juce::Viewport viewport;
    GlobalControlBar controlBar;
    SysExInspector sysExInspector;
    ProgramLibrarian programLibrarian;
    HardwareTransferPanel hardwareTools;
    std::array<std::unique_ptr<EditorPage>, 5> pages;
    int currentPage = 0;
};
}
