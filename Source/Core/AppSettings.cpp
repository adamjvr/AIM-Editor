#include "AppSettings.h"

namespace aim
{
namespace
{
constexpr auto pageKey = "session.page";
constexpr auto midiChannelKey = "session.midi_channel";
constexpr auto bankKey = "session.bank";
constexpr auto programKey = "session.program";
constexpr auto midiInputKey = "session.midi_input_identifier";
constexpr auto midiOutputKey = "session.midi_output_identifier";
}

AppSettings::AppSettings()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "AIM Editor";
    options.filenameSuffix = "settings";
    options.folderName = "AIM Editor";
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    properties.setStorageParameters (options);
}

AppSettings::~AppSettings()
{
    flush();
}

SessionSnapshot AppSettings::loadSession()
{
    SessionSnapshot snapshot;
    if (auto* settings = properties.getUserSettings())
    {
        snapshot.pageIndex = juce::jlimit (0, 4, settings->getIntValue (pageKey, 0));
        snapshot.midiChannel = juce::jlimit (1, 16, settings->getIntValue (midiChannelKey, 1));
        snapshot.bankIndex = juce::jlimit (0, 4, settings->getIntValue (bankKey, 0));
        snapshot.programIndex = juce::jmax (0, settings->getIntValue (programKey, 0));
        snapshot.midiInputIdentifier = settings->getValue (midiInputKey);
        snapshot.midiOutputIdentifier = settings->getValue (midiOutputKey);
    }
    return snapshot;
}

void AppSettings::saveSession (const SessionSnapshot& snapshot)
{
    if (auto* settings = properties.getUserSettings())
    {
        settings->setValue (pageKey, juce::jlimit (0, 4, snapshot.pageIndex));
        settings->setValue (midiChannelKey, juce::jlimit (1, 16, snapshot.midiChannel));
        settings->setValue (bankKey, juce::jlimit (0, 4, snapshot.bankIndex));
        settings->setValue (programKey, juce::jmax (0, snapshot.programIndex));
        settings->setValue (midiInputKey, snapshot.midiInputIdentifier);
        settings->setValue (midiOutputKey, snapshot.midiOutputIdentifier);
    }
}

void AppSettings::flush()
{
    properties.saveIfNeeded();
}
}
