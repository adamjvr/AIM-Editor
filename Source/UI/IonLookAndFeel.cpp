#include "IonLookAndFeel.h"

namespace aim
{
IonLookAndFeel::IonLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour::fromRGB (154, 154, 151));
    setColour (juce::Label::textColourId, juce::Colours::black);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (45, 45, 45));
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::outlineColourId, juce::Colour::fromRGB (20, 20, 20));
    setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (52, 52, 52));
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::black);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void IonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                       juce::Slider& slider)
{
    const auto radius = juce::jmin (width, height) * 0.5f - 4.0f;
    const auto centre = juce::Point<float> (x + width * 0.5f, y + height * 0.5f);
    const auto bounds = juce::Rectangle<float> (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (juce::Colour::fromRGB (55, 55, 55));
    g.fillEllipse (bounds);
    g.setColour (juce::Colour::fromRGB (15, 15, 15));
    g.drawEllipse (bounds, 1.5f);

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path indicator;
    indicator.addRoundedRectangle (-1.6f, -radius + 4.0f, 3.2f, radius * 0.45f, 1.2f);
    g.setColour (slider.isEnabled() ? juce::Colour::fromRGB (225, 35, 35)
                                    : juce::Colour::fromRGB (120, 75, 75));
    g.fillPath (indicator, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

void IonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto colour = backgroundColour;
    if (isButtonDown) colour = colour.darker (0.25f);
    else if (isMouseOverButton) colour = colour.brighter (0.12f);

    g.setColour (colour);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}
}
