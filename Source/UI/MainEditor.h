#pragma once

#include "Core/AppSettings.h"
#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "Core/ProgramHistory.h"
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
    MainEditor (const ParameterRegistry& registry, IonMidiService& midiService, AppSettings& appSettings);
    ~MainEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

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
    void persistSession();

    const ParameterRegistry& registry;
    IonMidiService& midi;
    AppSettings& appSettings;
    ProgramState programState;
    ProgramHistory programHistory;
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
