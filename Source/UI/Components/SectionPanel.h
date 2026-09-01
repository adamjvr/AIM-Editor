#pragma once

#include "Core/ParameterDefinition.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace aim
{
class ParameterKnob final : public juce::Component
{
public:
    explicit ParameterKnob (const ParameterDefinition& definition);
    void resized() override;

private:
    juce::Slider slider;
    juce::Label label;
};

class SectionPanel final : public juce::Component
{
public:
    SectionPanel (juce::String title, const std::vector<const ParameterDefinition*>& definitions);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String title;
    int totalParameterCount = 0;
    std::vector<std::unique_ptr<ParameterKnob>> knobs;
};
}
