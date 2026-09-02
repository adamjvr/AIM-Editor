#pragma once

#include "Core/IonProgram.h"
#include "Core/ParameterRegistry.h"
#include "Core/ProgramJson.h"
#include "Midi/IonMidiService.h"
#include "Midi/IonNrpnDecoder.h"
#include "Midi/IonSysExCodec.h"
#include "Midi/IonProgramDecoder.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
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
    std::function<void (const IonProgram&, const IonPatchDump&)> onLoadCandidateProgram;

private:
    void addEventOnMessageThread (MidiCaptureEvent event);
    void inspectCandidatePatch (const MidiCaptureEvent& event);
    void inspectCandidateNrpn (const MidiCaptureEvent& event);
    void rebuildLog();
    void clearCapture();
    void copyJsonToClipboard();
    void saveJson();
    void loadLatestCandidateProgram();
    void updateVerificationContextStatus();
    [[nodiscard]] juce::String makeCaptureJson() const;
    [[nodiscard]] juce::String makeLogLine (const MidiCaptureEvent& event) const;

    struct CandidateNrpnTransaction
    {
        MidiDirection direction = MidiDirection::input;
        std::int64_t utcMilliseconds = 0;
        juce::String deviceIdentifier;
        DecodedNrpn decoded;
        juce::String parameterId;
        juce::String parameterName;
        juce::String mappingStatus { "unmapped" };
        juce::String valueEncoding;
        std::optional<int> semanticValue;

        [[nodiscard]] juce::var toJson() const;
    };

    static constexpr std::size_t maxEvents = 4096;

    IonMidiService& midi;
    const ParameterRegistry& registry;
    std::vector<MidiCaptureEvent> events;
    std::vector<CandidateNrpnTransaction> nrpnTransactions;
    IonNrpnDecoder inputNrpnDecoder;
    IonNrpnDecoder outputNrpnDecoder;
    std::optional<IonProgram> latestCandidateProgram;
    std::optional<IonPatchDump> latestCandidatePatch;
    juce::String latestCandidateName;

    juce::Label title;
    juce::Label summary;
    juce::Label candidateSummary;
    juce::Label verificationTagLabel;
    juce::TextEditor verificationParameter;
    juce::ToggleButton verificationIsolation { "Only this control moved" };
    juce::Label verificationStatus;
    juce::TextEditor verificationNote;
    juce::TextEditor log;
    juce::ToggleButton sysexOnly { "SysEx only" };
    juce::TextButton clearButton { "Clear" };
    juce::TextButton copyButton { "Copy JSON" };
    juce::TextButton saveButton { "Save JSON" };
    juce::TextButton loadPatchButton { "Load Patch" };
    juce::TextButton closeButton { "Close" };
    std::unique_ptr<juce::FileChooser> fileChooser;
};
}
