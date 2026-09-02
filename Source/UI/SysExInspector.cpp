#include "SysExInspector.h"

#include <juce_data_structures/juce_data_structures.h>

namespace aim
{
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

    log.setMultiLine (true);
    log.setReadOnly (true);
    log.setScrollbarsShown (true);
    log.setCaretVisible (false);
    log.setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (18, 18, 18));
    log.setColour (juce::TextEditor::textColourId, juce::Colour::fromRGB (225, 225, 225));
    log.setFont (juce::Font (juce::FontOptions (12.0f)));

    sysexOnly.setToggleState (true, juce::dontSendNotification);
    sysexOnly.setTooltip ("Show only SysEx in the text view. JSON export still preserves the complete capture.");

    for (auto* component : { static_cast<juce::Component*> (&title), &summary, &log, &sysexOnly,
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

    area.removeFromTop (8);
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

    if (shouldShow)
    {
        log.moveCaretToEnd();
        log.insertTextAtCaret (makeLogLine (events.back()) + "\n");
    }

    std::size_t sysexCount = 0;
    for (const auto& captured : events)
        if (captured.isSysEx())
            ++sysexCount;

    summary.setText (juce::String (static_cast<juce::int64> (events.size())) + " captured / " + juce::String (static_cast<juce::int64> (sysexCount)) + " SysEx",
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
    loadPatchButton.setEnabled (true);
    loadPatchButton.setButtonText (latestCandidateName.isNotEmpty() ? "Load " + latestCandidateName : "Load Patch");
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
    latestCandidateProgram.reset();
    latestCandidatePatch.reset();
    latestCandidateName.clear();
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

    root->setProperty ("events", juce::var (jsonEvents));
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
