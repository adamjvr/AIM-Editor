#include "SysExInspector.h"
#include "Midi/IonProtocol.h"

#include <juce_data_structures/juce_data_structures.h>

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

    log.setMultiLine (true);
    log.setReadOnly (true);
    log.setScrollbarsShown (true);
    log.setCaretVisible (false);
    log.setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (18, 18, 18));
    log.setColour (juce::TextEditor::textColourId, juce::Colour::fromRGB (225, 225, 225));
    log.setFont (juce::Font (juce::FontOptions (12.0f)));

    sysexOnly.setToggleState (true, juce::dontSendNotification);
    sysexOnly.setTooltip ("Show only SysEx in the text view. JSON export still preserves the complete capture.");

    for (auto* component : { static_cast<juce::Component*> (&title), &summary, &candidateSummary, &log, &sysexOnly,
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
        transaction.mappingStatus = mappingStatusToString (definition->mappingStatus);
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
    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile ("aim-midi-capture.json");

    fileChooser = std::make_unique<juce::FileChooser> ("Export AIM Editor MIDI capture",
                                                       suggested,
                                                       "*.json",
                                                       true);

    const auto json = makeCaptureJson();
    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync (flags,
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
