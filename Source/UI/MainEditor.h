#pragma once

#include "Core/ParameterRegistry.h"
#include "Midi/IonMidiService.h"
#include "UI/GlobalControlBar.h"
#include "UI/Panels/EditorPage.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

namespace aim
{
class MainEditor final : public juce::Component
{
public:
    MainEditor (const ParameterRegistry& registry, IonMidiService& midiService);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showPage (int pageIndex);
    void updateViewedPageSize();

    juce::Viewport viewport;
    GlobalControlBar controlBar;
    std::array<std::unique_ptr<EditorPage>, 5> pages;
    int currentPage = 0;
};
}
