#include "SysExInspector.h"
#include <initializer_list>
#include "Midi/IonProtocol.h"

#include <juce_data_structures/juce_data_structures.h>

#include <set>

namespace aim
{
juce::var SysExInspector::CandidateNrpnTransaction::toJson() const
{
    auto* object = new juce::DynamicObject();
    object->setProperty ("direction", direction == MidiDirection::input ? "input" : "output");
    object->setProperty ("utc_ms", static_cast<juce::int64> (utcMilliseconds));
    object->setProperty ("device_identifier", deviceIdentifier);
    object->setProperty ("midi_channel", decoded.midiChannel);
    object->setProperty ("nrpn", decoded.parameter);
    object->setProperty ("value14", decoded.value14Bit);
    object->setProperty ("mapping_status", mappingStatus);
    object->setProperty ("value_encoding", valueEncoding);
    object->setProperty ("parameter_id", parameterId.isNotEmpty() ? juce::var (parameterId) : juce::var());
    object->setProperty ("parameter_name", parameterName.isNotEmpty() ? juce::var (parameterName) : juce::var());
    object->setProperty ("semantic_value", semanticValue.has_value() ? juce::var (*semanticValue) : juce::var());
    return juce::var (object);
}

SysExInspector::SysExInspector (IonMidiService& midiService, const ParameterRegistry& parameterRegistry)
    : midi (midiService),
      registry (parameterRegistry)
{
    title.setText ("MIDI / SysEx Inspector", juce::dontSendNotification);
    title.setFont (juce::FontOptions (18.0f));
    title.setColour (juce::Label::textColourId, juce::Colours::white);

    summary.setText ("0 captured", juce::dontSendNotification);
    summary.setJustificationType (juce::Justification::centredRight);
    summary.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.7f));

    candidateSummary.setText ("No checksum-valid candidate Ion patch captured yet", juce::dontSendNotification);
    candidateSummary.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 160, 70));
    candidateSummary.setFont (juce::FontOptions (11.0f));

    verificationTagLabel.setText ("VERIFY", juce::dontSendNotification);
    verificationTagLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.65f));
    verificationTagLabel.setFont (juce::FontOptions (10.0f));

    verificationParameter.setMultiLine (false);
    verificationParameter.setTextToShowWhenEmpty ("parameter id, e.g. filter1.frequency",
                                                   juce::Colours::white.withAlpha (0.35f));
    verificationParameter.setTooltip ("Tag this capture with the one semantic parameter intentionally moved during a controlled hardware experiment.");
    verificationParameter.onTextChange = [this] { updateVerificationContextStatus(); };

    verificationIsolation.setTooltip ("Explicitly confirm that no other Ion control was deliberately moved during this capture. Required before offline tooling can mark evidence promotable.");

    verificationStatus.setText ("optional experiment tag", juce::dontSendNotification);
    verificationStatus.setJustificationType (juce::Justification::centredRight);
    verificationStatus.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.5f));
    verificationStatus.setFont (juce::FontOptions (10.0f));

    verificationNote.setMultiLine (false);
    verificationNote.setTextToShowWhenEmpty ("verification note (value sweep, control position, test number…)",
                                              juce::Colours::white.withAlpha (0.35f));
    verificationNote.setTooltip ("Free-form human context stored with the raw capture. It never changes the captured MIDI bytes.");

    startVerificationButton.setTooltip ("Clear the current capture and begin a controlled one-parameter hardware experiment.");
    stopVerificationButton.setTooltip ("Freeze the controlled experiment so unrelated MIDI cannot contaminate the saved evidence.");
    stopVerificationButton.setEnabled (false);
    startVerificationButton.onClick = [this] { startVerificationExperiment(); };
    stopVerificationButton.onClick = [this] { stopVerificationExperiment(); };

    experimentAssessment.setText ("No controlled verification test running", juce::dontSendNotification);
    experimentAssessment.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));
    experimentAssessment.setFont (juce::FontOptions (10.5f));
    experimentAssessment.setJustificationType (juce::Justification::centredLeft);

    verificationIsolation.onClick = [this] { updateExperimentAssessment(); };

    log.setMultiLine (true);
    log.setReadOnly (true);
    log.setScrollbarsShown (true);
    log.setCaretVisible (false);
    log.setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (18, 18, 18));
    log.setColour (juce::TextEditor::textColourId, juce::Colour::fromRGB (225, 225, 225));
    log.setFont (juce::Font (juce::FontOptions (12.0f)));

    sysexOnly.setToggleState (true, juce::dontSendNotification);
    sysexOnly.setTooltip ("Show only SysEx in the text view. JSON export still preserves the complete capture.");

    for (auto* component : std::initializer_list<juce::Component*> { static_cast<juce::Component*> (&title), &summary, &candidateSummary,
                             &verificationTagLabel, &verificationParameter, &verificationIsolation,
                             &verificationStatus, &verificationNote, &startVerificationButton,
                             &stopVerificationButton, &experimentAssessment, &log, &sysexOnly,
                             &clearButton, &copyButton, &saveButton, &loadPatchButton, &closeButton })
        addAndMakeVisible (component);

    sysexOnly.onClick = [this] { rebuildLog(); };
    clearButton.onClick = [this] { clearCapture(); };
    copyButton.onClick = [this] { copyJsonToClipboard(); };
    saveButton.onClick = [this] { saveJson(); };
    loadPatchButton.setEnabled (false);
    loadPatchButton.setTooltip ("Load the latest checksum-valid candidate Ion patch into the semantic editor state");
    loadPatchButton.onClick = [this] { loadLatestCandidateProgram(); };
    closeButton.onClick = [this]
    {
        if (verificationExperimentActive)
            stopVerificationExperiment();
        if (onClose)
            onClose();
    };

    midi.setMonitorHandler ([safe = juce::Component::SafePointer<SysExInspector> (this)] (const MidiCaptureEvent& event) mutable
    {
        if (safe == nullptr)
            return;

        auto copy = event;
        juce::MessageManager::callAsync ([safe, copy = std::move (copy)]() mutable
        {
            if (safe != nullptr)
                safe->addEventOnMessageThread (std::move (copy));
        });
    });
}

SysExInspector::~SysExInspector()
{
    midi.setMonitorHandler ({});
}

void SysExInspector::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillAll();

    const auto panel = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour::fromRGB (38, 38, 38));
    g.fillRoundedRectangle (panel, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (panel, 8.0f, 1.0f);
}

void SysExInspector::resized()
{
    auto area = getLocalBounds().reduced (14);
    auto header = area.removeFromTop (30);
    title.setBounds (header.removeFromLeft (juce::jmax (180, header.getWidth() / 2)));
    summary.setBounds (header);

    area.removeFromTop (4);
    candidateSummary.setBounds (area.removeFromTop (22));
    area.removeFromTop (4);

    auto verificationRow = area.removeFromTop (28);
    verificationTagLabel.setBounds (verificationRow.removeFromLeft (52));
    verificationRow.removeFromLeft (4);
    verificationParameter.setBounds (verificationRow.removeFromLeft (juce::jlimit (180, 300, getWidth() / 4)));
    verificationRow.removeFromLeft (6);
    verificationIsolation.setBounds (verificationRow.removeFromLeft (160));
    verificationRow.removeFromLeft (6);
    verificationStatus.setBounds (verificationRow);

    area.removeFromTop (4);
    verificationNote.setBounds (area.removeFromTop (26));
    area.removeFromTop (4);
    auto experimentRow = area.removeFromTop (28);
    startVerificationButton.setBounds (experimentRow.removeFromLeft (92));
    experimentRow.removeFromLeft (6);
    stopVerificationButton.setBounds (experimentRow.removeFromLeft (92));
    experimentRow.removeFromLeft (8);
    experimentAssessment.setBounds (experimentRow);
    area.removeFromTop (5);
    auto controls = area.removeFromBottom (32);
    const int gap = 6;

    sysexOnly.setBounds (controls.removeFromLeft (110));
    controls.removeFromLeft (gap);
    clearButton.setBounds (controls.removeFromLeft (72));
    controls.removeFromLeft (gap);
    copyButton.setBounds (controls.removeFromLeft (100));
    controls.removeFromLeft (gap);
    saveButton.setBounds (controls.removeFromLeft (100));
    controls.removeFromLeft (gap);
    loadPatchButton.setBounds (controls.removeFromLeft (100));
    controls.removeFromLeft (gap);
    closeButton.setBounds (controls.removeFromRight (80));

    area.removeFromBottom (8);
    log.setBounds (area);
}

void SysExInspector::addEventOnMessageThread (MidiCaptureEvent event)
{
    jassert (juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (verificationCaptureFrozen)
        return;

    if (events.size() >= maxEvents)
        events.erase (events.begin(), events.begin() + static_cast<std::ptrdiff_t> (maxEvents / 8));

    const auto shouldShow = ! sysexOnly.getToggleState() || event.isSysEx();
    events.push_back (std::move (event));
    inspectCandidatePatch (events.back());
    inspectCandidateNrpn (events.back());

    if (shouldShow)
    {
        log.moveCaretToEnd();
        log.insertTextAtCaret (makeLogLine (events.back()) + "\n");
    }

    std::size_t sysexCount = 0;
    for (const auto& captured : events)
        if (captured.isSysEx())
            ++sysexCount;

    summary.setText (juce::String (static_cast<juce::int64> (events.size())) + " captured / "
                         + juce::String (static_cast<juce::int64> (sysexCount)) + " SysEx / "
                         + juce::String (static_cast<juce::int64> (nrpnTransactions.size())) + " NRPN",
                     juce::dontSendNotification);

    if (verificationExperimentActive)
        updateExperimentAssessment();
}

void SysExInspector::inspectCandidatePatch (const MidiCaptureEvent& event)
{
    if (! event.isSysEx()
        || event.message.getSysExDataSize() != static_cast<int> (IonSysExCodec::encodedSinglePatchPayloadSize))
        return;

    IonPatchDump patch;
    if (const auto result = IonSysExCodec::decodeSinglePatchDump (event.message, patch); result.failed())
        return;

    if (! patch.checksumValid)
        return;

    IonProgram program;
    if (const auto result = IonProgramDecoder::decode (patch, registry, program); result.failed())
        return;

    latestCandidateName = patch.name;
    latestCandidatePatch = patch;
    latestCandidateProgram = std::move (program);
    candidateSummary.setText ("Valid candidate patch: "
                                + (patch.name.isNotEmpty() ? patch.name : juce::String ("<unnamed>"))
                                + "  •  " + IonSysExCodec::bankName (static_cast<IonBank> (patch.bank))
                                + " " + juce::String (patch.slot + 1)
                                + "  •  firmware " + patch.firmwareVersion
                                + "  •  checksum OK",
                              juce::dontSendNotification);
    candidateSummary.setColour (juce::Label::textColourId, juce::Colour::fromRGB (105, 210, 120));
    loadPatchButton.setEnabled (true);
    loadPatchButton.setButtonText (latestCandidateName.isNotEmpty() ? "Load " + latestCandidateName : "Load Patch");
}

void SysExInspector::inspectCandidateNrpn (const MidiCaptureEvent& event)
{
    if (! event.message.isController())
        return;

    auto& decoder = event.direction == MidiDirection::input ? inputNrpnDecoder : outputNrpnDecoder;
    const auto decoded = decoder.push (event.message);
    if (! decoded)
        return;

    CandidateNrpnTransaction transaction;
    transaction.direction = event.direction;
    transaction.utcMilliseconds = event.utcMilliseconds;
    transaction.deviceIdentifier = event.deviceIdentifier;
    transaction.decoded = *decoded;

    if (const auto* definition = registry.findByNrpn (decoded->parameter))
    {
        transaction.parameterId = juce::String (definition->id);
        transaction.parameterName = definition->name;
        transaction.mappingStatus = mappingStatusToString (definition->nrpnMappingStatus);
        transaction.valueEncoding = definition->nrpnValueEncoding;

        if (definition->nrpnValueEncoding == "signed_14_wrap")
            transaction.semanticValue = IonProtocol::decodeIonSigned14 (decoded->value14Bit);
        else if (definition->nrpnValueEncoding == "unsigned_14")
            transaction.semanticValue = decoded->value14Bit;
    }

    if (nrpnTransactions.size() >= maxEvents)
        nrpnTransactions.erase (nrpnTransactions.begin(), nrpnTransactions.begin() + static_cast<std::ptrdiff_t> (maxEvents / 8));

    nrpnTransactions.push_back (std::move (transaction));
}

void SysExInspector::updateVerificationContextStatus()
{
    const auto parameterId = verificationParameter.getText().trim();
    if (parameterId.isEmpty())
    {
        verificationStatus.setText ("optional experiment tag", juce::dontSendNotification);
        verificationStatus.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.5f));
        return;
    }

    const auto* definition = registry.find (parameterId.toStdString());
    if (definition == nullptr)
    {
        verificationStatus.setText ("unknown parameter id", juce::dontSendNotification);
        verificationStatus.setColour (juce::Label::textColourId, juce::Colour::fromRGB (235, 95, 95));
        return;
    }

    auto text = definition->name;
    if (definition->nrpn)
        text << "  • NRPN " << *definition->nrpn << " " << mappingStatusToString (definition->nrpnMappingStatus);
    if (definition->sysex.offset)
        text << "  • SysEx @" << *definition->sysex.offset << " " << mappingStatusToString (definition->sysexMappingStatus);
    verificationStatus.setText (text, juce::dontSendNotification);
    verificationStatus.setColour (juce::Label::textColourId, juce::Colour::fromRGB (115, 205, 130));
}

int SysExInspector::recommendedDistinctValues (const ParameterDefinition& definition) const
{
    if (definition.kind == ParameterKind::boolean)
        return 2;

    if (definition.rawMin && definition.rawMax && *definition.rawMax - *definition.rawMin <= 1.0)
        return 2;

    return 3;
}

void SysExInspector::startVerificationExperiment()
{
    const auto parameterId = verificationParameter.getText().trim();
    const auto* definition = parameterId.isNotEmpty() ? registry.find (parameterId.toStdString()) : nullptr;
    if (definition == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                "AIM Editor",
                                                "Enter a known semantic parameter ID before starting a verification test.");
        return;
    }

    if (! definition->nrpn)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                "AIM Editor",
                                                "This parameter has no candidate NRPN mapping to test yet.");
        return;
    }

    clearCapture();
    verificationExperimentActive = true;
    verificationCaptureFrozen = false;
    verificationStartedUtcMs = juce::Time::currentTimeMillis();
    verificationCompletedUtcMs = 0;
    verificationParameter.setReadOnly (true);
    verificationIsolation.setToggleState (false, juce::dontSendNotification);
    startVerificationButton.setEnabled (false);
    stopVerificationButton.setEnabled (true);
    copyButton.setEnabled (false);
    saveButton.setEnabled (false);
    sysexOnly.setToggleState (false, juce::dontSendNotification);
    rebuildLog();
    updateExperimentAssessment();
}

void SysExInspector::stopVerificationExperiment()
{
    if (! verificationExperimentActive)
        return;

    verificationExperimentActive = false;
    verificationCaptureFrozen = true;
    verificationCompletedUtcMs = juce::Time::currentTimeMillis();
    // Keep the target locked with the frozen capture. Clear explicitly before
    // re-tagging evidence to a different semantic parameter.
    verificationParameter.setReadOnly (true);
    startVerificationButton.setEnabled (true);
    stopVerificationButton.setEnabled (false);
    copyButton.setEnabled (true);
    saveButton.setEnabled (true);
    updateExperimentAssessment();
}

void SysExInspector::updateExperimentAssessment()
{
    const auto parameterId = verificationParameter.getText().trim();
    const auto* definition = parameterId.isNotEmpty() ? registry.find (parameterId.toStdString()) : nullptr;
    if (definition == nullptr || ! definition->nrpn)
    {
        experimentAssessment.setText (verificationExperimentActive ? "Invalid verification target" : "No controlled verification test running",
                                      juce::dontSendNotification);
        experimentAssessment.setColour (juce::Label::textColourId,
                                        verificationExperimentActive ? juce::Colour::fromRGB (235, 95, 95)
                                                                     : juce::Colours::white.withAlpha (0.55f));
        return;
    }

    int expectedCount = 0;
    std::set<int> distinctValues;
    std::set<int> competingNumbers;
    for (const auto& transaction : nrpnTransactions)
    {
        if (transaction.direction != MidiDirection::input)
            continue;

        if (transaction.decoded.parameter == *definition->nrpn)
        {
            ++expectedCount;
            if (transaction.semanticValue)
                distinctValues.insert (*transaction.semanticValue);
        }
        else
        {
            competingNumbers.insert (transaction.decoded.parameter);
        }
    }

    const auto recommended = recommendedDistinctValues (*definition);
    juce::String text;
    if (verificationExperimentActive)
        text << "RECORDING • ";
    else if (verificationCaptureFrozen)
        text << "FROZEN • ";

    text << "NRPN " << *definition->nrpn
         << ": " << expectedCount << " tx / " << static_cast<int> (distinctValues.size())
         << "/" << recommended << " distinct";

    if (! competingNumbers.empty())
        text << " • " << static_cast<int> (competingNumbers.size()) << " competing NRPN";
    else
        text << " • no competing NRPN";

    const auto structurallyReady = static_cast<int> (distinctValues.size()) >= recommended
                                && competingNumbers.empty();
    if (verificationCaptureFrozen && structurallyReady)
        text << (verificationIsolation.getToggleState() ? " • ready for offline sealing" : " • confirm isolation");

    experimentAssessment.setText (text, juce::dontSendNotification);
    experimentAssessment.setColour (juce::Label::textColourId,
                                    structurallyReady ? juce::Colour::fromRGB (105, 210, 120)
                                                      : juce::Colour::fromRGB (220, 160, 70));
}

void SysExInspector::loadLatestCandidateProgram()
{
    if (! latestCandidateProgram || ! latestCandidatePatch || ! onLoadCandidateProgram)
        return;

    onLoadCandidateProgram (*latestCandidateProgram, *latestCandidatePatch);
}

void SysExInspector::rebuildLog()
{
    juce::String text;
    const auto onlySysEx = sysexOnly.getToggleState();

    for (const auto& event : events)
        if (! onlySysEx || event.isSysEx())
            text << makeLogLine (event) << "\n";

    log.setText (text, false);
    log.moveCaretToEnd();
}

void SysExInspector::clearCapture()
{
    verificationExperimentActive = false;
    verificationCaptureFrozen = false;
    verificationStartedUtcMs = 0;
    verificationCompletedUtcMs = 0;
    verificationParameter.setReadOnly (false);
    startVerificationButton.setEnabled (true);
    stopVerificationButton.setEnabled (false);
    copyButton.setEnabled (true);
    saveButton.setEnabled (true);
    experimentAssessment.setText ("No controlled verification test running", juce::dontSendNotification);
    experimentAssessment.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));

    events.clear();
    nrpnTransactions.clear();
    inputNrpnDecoder.reset();
    outputNrpnDecoder.reset();
    latestCandidateProgram.reset();
    latestCandidatePatch.reset();
    latestCandidateName.clear();
    candidateSummary.setText ("No checksum-valid candidate Ion patch captured yet", juce::dontSendNotification);
    candidateSummary.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 160, 70));
    loadPatchButton.setEnabled (false);
    loadPatchButton.setButtonText ("Load Patch");
    log.setText ({}, false);
    summary.setText ("0 captured", juce::dontSendNotification);
}

void SysExInspector::copyJsonToClipboard()
{
    juce::SystemClipboard::copyTextToClipboard (makeCaptureJson());
}

void SysExInspector::saveJson()
{
    auto stem = verificationParameter.getText().trim().replaceCharacters ("./\\: ", "_____");
    if (stem.isEmpty())
        stem = "aim-midi-capture";
    else
        stem = "verify-" + stem;
    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile (stem + ".json");

    fileChooser = std::make_unique<juce::FileChooser> ("Export AIM Editor MIDI capture",
                                                       suggested,
                                                       "*.json",
                                                       true);

    const auto json = makeCaptureJson();
    const auto chooserFlags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync (chooserFlags,
                              [safe = juce::Component::SafePointer<SysExInspector> (this), json] (const juce::FileChooser& chooser)
                              {
                                  const auto destination = chooser.getResult();
                                  if (destination == juce::File{})
                                      return;

                                  if (! destination.replaceWithText (json) && safe != nullptr)
                                      juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                                              "AIM Editor",
                                                                              "Could not write MIDI capture JSON.");
                              });
}

juce::String SysExInspector::makeCaptureJson() const
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("format", "aim-editor.midi-capture");
    root->setProperty ("schema_version", 1);
    root->setProperty ("exported_at_utc_ms", static_cast<juce::int64> (juce::Time::currentTimeMillis()));

    auto* verificationContext = new juce::DynamicObject();
    const auto verificationParameterId = verificationParameter.getText().trim();
    const auto parameterKnown = verificationParameterId.isNotEmpty()
                             && registry.find (verificationParameterId.toStdString()) != nullptr;
    verificationContext->setProperty ("parameter_id", verificationParameterId.isNotEmpty()
                                                        ? juce::var (verificationParameterId) : juce::var());
    verificationContext->setProperty ("parameter_known", parameterKnown);
    verificationContext->setProperty ("user_confirmed_control_isolation", verificationIsolation.getToggleState());
    verificationContext->setProperty ("note", verificationNote.getText());
    verificationContext->setProperty ("experiment_started_utc_ms", verificationStartedUtcMs > 0
                                                                   ? juce::var (static_cast<juce::int64> (verificationStartedUtcMs))
                                                                   : juce::var());
    verificationContext->setProperty ("experiment_completed_utc_ms", verificationCompletedUtcMs > 0
                                                                     ? juce::var (static_cast<juce::int64> (verificationCompletedUtcMs))
                                                                     : juce::var());
    verificationContext->setProperty ("capture_frozen", verificationCaptureFrozen);

    int expectedCount = 0;
    std::set<int> distinctValues;
    std::set<int> competingNumbers;
    if (const auto* definition = parameterKnown ? registry.find (verificationParameterId.toStdString()) : nullptr;
        definition != nullptr && definition->nrpn)
    {
        for (const auto& transaction : nrpnTransactions)
        {
            if (transaction.direction != MidiDirection::input)
                continue;
            if (transaction.decoded.parameter == *definition->nrpn)
            {
                ++expectedCount;
                if (transaction.semanticValue)
                    distinctValues.insert (*transaction.semanticValue);
            }
            else
            {
                competingNumbers.insert (transaction.decoded.parameter);
            }
        }
    }
    verificationContext->setProperty ("observed_expected_nrpn_transactions", expectedCount);
    verificationContext->setProperty ("observed_distinct_semantic_values", static_cast<int> (distinctValues.size()));
    verificationContext->setProperty ("observed_competing_nrpn_numbers", static_cast<int> (competingNumbers.size()));
    root->setProperty ("verification_context", juce::var (verificationContext));

    juce::Array<juce::var> jsonEvents;
    jsonEvents.ensureStorageAllocated (static_cast<int> (events.size()));
    for (const auto& event : events)
    {
        auto eventJson = event.toJson();

        // Keep raw capture protocol-neutral, but attach a clearly-labelled
        // candidate interpretation when a message has the exact single-patch
        // transport length and passes the structural Ion checks.
        if (event.isSysEx() && event.message.getSysExDataSize() == static_cast<int> (IonSysExCodec::encodedSinglePatchPayloadSize))
        {
            IonPatchDump patch;
            if (const auto result = IonSysExCodec::decodeSinglePatchDump (event.message, patch); result.wasOk())
            {
                if (auto* object = eventJson.getDynamicObject())
                {
                    object->setProperty ("ion_candidate_patch", patch.toJsonSummary());

                    IonProgram program;
                    if (const auto decodeResult = IonProgramDecoder::decode (patch, registry, program); decodeResult.wasOk())
                    {
                        juce::var programJson;
                        if (juce::JSON::parse (ProgramJson::encode (program), programJson).wasOk())
                            object->setProperty ("ion_candidate_program", std::move (programJson));
                    }
                }
            }
        }

        jsonEvents.add (std::move (eventJson));
    }

    juce::Array<juce::var> jsonNrpnTransactions;
    jsonNrpnTransactions.ensureStorageAllocated (static_cast<int> (nrpnTransactions.size()));
    for (const auto& transaction : nrpnTransactions)
        jsonNrpnTransactions.add (transaction.toJson());

    root->setProperty ("events", juce::var (jsonEvents));
    root->setProperty ("nrpn_transactions", juce::var (jsonNrpnTransactions));
    return juce::JSON::toString (juce::var (root), false);
}

juce::String SysExInspector::makeLogLine (const MidiCaptureEvent& event) const
{
    const auto direction = event.direction == MidiDirection::input ? "IN " : "OUT";
    const auto time = juce::Time (event.utcMilliseconds).toString (false, true, true, true);

    juce::String interpretation;
    if (event.isSysEx() && event.message.getSysExDataSize() == static_cast<int> (IonSysExCodec::encodedSinglePatchPayloadSize))
    {
        IonPatchDump patch;
        if (const auto result = IonSysExCodec::decodeSinglePatchDump (event.message, patch); result.wasOk())
            interpretation = " | Ion patch? name=\"" + patch.name + "\" bank=" + juce::String (patch.bank)
                           + " slot=" + juce::String (patch.slot)
                           + " checksum=" + (patch.checksumValid ? "OK" : "BAD");
    }

    return "[" + time + "] [" + direction + "] "
         + event.messageKind() + " "
         + juce::String (event.message.getRawDataSize()) + " B | "
         + event.hexBytes() + interpretation;
}
}
