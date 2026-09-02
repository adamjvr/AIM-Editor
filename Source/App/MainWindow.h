#pragma once

#include "Core/AppSettings.h"
#include "Core/ParameterRegistry.h"
#include "Midi/IonMidiService.h"
#include "UI/MainEditor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace aim
{
class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow (const ParameterRegistry& registry, IonMidiService& midiService, AppSettings& appSettings);
    void closeButtonPressed() override;
    void requestQuit();
};
}
