#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace aim
{
class IonLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    IonLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool isMouseOverButton, bool isButtonDown) override;
};
}
