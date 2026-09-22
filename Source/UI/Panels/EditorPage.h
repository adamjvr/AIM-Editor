#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "UI/Components/EnvelopePanel.h"
#include "UI/Components/EffectsPanel.h"
#include "UI/Components/FilterPanel.h"
#include "UI/Components/MixerPanel.h"
#include "UI/Components/MicronExtensionsPanel.h"
#include "UI/Components/ModMatrixPanel.h"
#include "UI/Components/ModulatorPanel.h"
#include "UI/Components/OscillatorPanel.h"
#include "UI/Components/OutputPanel.h"
#include "UI/Components/RandomizerPanel.h"
#include "UI/Components/SectionPanel.h"
#include "UI/Components/TrackingGeneratorPanel.h"
#include "UI/Components/VoicePanel.h"
#include "Midi/IonFamilyDevice.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

namespace aim
{
class EditorPage final : public juce::Component
{
public:
    EditorPage (juce::String pageKey,
                juce::String pageTitle,
                const ParameterRegistry& registry,
                ProgramState& state);

    void paint (juce::Graphics&) override;
    void resized() override;
    void setDeviceProfile (IonFamilyDevice device);

    [[nodiscard]] int preferredHeightForWidth (int width) const;
    [[nodiscard]] const juce::String& getPageKey() const noexcept { return key; }

private:
    struct SectionEntry
    {
        juce::String group;
        std::unique_ptr<EditorSection> section;
    };

    static juce::String groupKeyForSection (const juce::String& section);
    static juce::String titleForGroup (const juce::String& group);
    [[nodiscard]] int columnCountForWidth (int width) const;
    [[nodiscard]] EditorSection* sectionFor (const juce::String& group) const;
    [[nodiscard]] int rowPreferredHeight (int width,
                                          std::initializer_list<std::pair<const char*, float>> groups) const;
    int layoutRow (juce::Rectangle<int> area,
                   int y,
                   std::initializer_list<std::pair<const char*, float>> groups);
    void layoutMasonry (juce::Rectangle<int> area);
    [[nodiscard]] int masonryPreferredHeight (int width) const;

    juce::String key;
    juce::String title;
    IonFamilyDevice deviceProfile = IonFamilyDevice::ion;
    std::vector<SectionEntry> sections;
};
}
