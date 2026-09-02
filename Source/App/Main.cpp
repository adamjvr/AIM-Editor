#include "App/MainWindow.h"
#include "Core/ParameterRegistry.h"
#include "Midi/IonMidiService.h"
#include "UI/IonLookAndFeel.h"

#include <BinaryData.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace aim
{
class AIMEditorApplication final : public juce::JUCEApplication
{
public:
    AIMEditorApplication() = default;

    const juce::String getApplicationName() override { return "AIM Editor"; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const juce::String&) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);

       #if JUCE_WINDOWS
        juce::TopLevelWindow::setUsingWindowsMultiTouch (true);
       #endif

        const auto json = juce::String::fromUTF8 (AIMBinaryData::parameters_json,
                                                  AIMBinaryData::parameters_jsonSize);
        const auto loadResult = registry.loadFromJson (json);

        if (loadResult.failed())
        {
            juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                         "AIM Editor",
                                                         "Could not load parameter database:\n" + loadResult.getErrorMessage());
        }

        const auto loadEnumTable = [this] (const char* data, int size)
        {
            const auto text = juce::String::fromUTF8 (data, size);
            if (const auto result = registry.loadEnumTableFromJson (text); result.failed())
                juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                             "AIM Editor",
                                                             "Could not load enum table:\n" + result.getErrorMessage());
        };

        loadEnumTable (AIMBinaryData::modulation_sources_json, AIMBinaryData::modulation_sources_jsonSize);
        loadEnumTable (AIMBinaryData::modulation_destinations_json, AIMBinaryData::modulation_destinations_jsonSize);
        loadEnumTable (AIMBinaryData::filter_types_json, AIMBinaryData::filter_types_jsonSize);

        mainWindow = std::make_unique<MainWindow> (registry, midiService);
    }

    void shutdown() override
    {
        mainWindow.reset();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted (const juce::String&) override {}

private:
    IonLookAndFeel lookAndFeel;
    ParameterRegistry registry;
    IonMidiService midiService;
    std::unique_ptr<MainWindow> mainWindow;
};
}

START_JUCE_APPLICATION (aim::AIMEditorApplication)
