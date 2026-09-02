#include "IonLookAndFeel.h"

#include <cmath>

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
    setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromRGB (150, 34, 34));
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::black);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB (37, 37, 37));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour::fromRGB (164, 36, 36));
}

void IonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                       juce::Slider& slider)
{
    const auto radius = juce::jmin (width, height) * 0.5f - 6.0f;
    if (radius <= 2.0f)
        return;

    const auto centre = juce::Point<float> (x + width * 0.5f, y + height * 0.5f);
    const auto bounds = juce::Rectangle<float> (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // Hardware-like scale ticks. They remain subtle at small sizes but are
    // useful on Retina/iPad displays where the old bitmap knobs would blur.
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    constexpr int tickCount = 11;
    for (int tick = 0; tick < tickCount; ++tick)
    {
        const auto proportion = static_cast<float> (tick) / static_cast<float> (tickCount - 1);
        const auto angle = rotaryStartAngle + proportion * (rotaryEndAngle - rotaryStartAngle);
        const auto inner = centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * (radius + 1.0f);
        const auto outer = centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * (radius + 4.5f);
        g.drawLine ({ inner, outer }, tick == 5 ? 1.3f : 0.8f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.20f));
    g.fillEllipse (bounds.translated (0.0f, 1.5f).expanded (1.0f));

    juce::ColourGradient knobGradient (juce::Colour::fromRGB (82, 82, 82), bounds.getX(), bounds.getY(),
                                       juce::Colour::fromRGB (32, 32, 32), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (knobGradient);
    g.fillEllipse (bounds);
    g.setColour (juce::Colour::fromRGB (12, 12, 12));
    g.drawEllipse (bounds, 1.4f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawEllipse (bounds.reduced (2.0f), 0.8f);

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path indicator;
    indicator.addRoundedRectangle (-1.7f, -radius + 4.0f, 3.4f, radius * 0.48f, 1.3f);
    g.setColour (slider.isEnabled() ? juce::Colour::fromRGB (229, 39, 36)
                                    : juce::Colour::fromRGB (120, 75, 75));
    g.fillPath (indicator, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

    if (slider.hasKeyboardFocus (false))
    {
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawEllipse (bounds.expanded (3.0f), 1.0f);
    }
}

void IonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto colour = backgroundColour;
    if (isButtonDown) colour = colour.darker (0.25f);
    else if (isMouseOverButton) colour = colour.brighter (0.12f);

    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.0f), 4.0f);
    g.setColour (colour);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

void IonLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    const auto size = juce::jmin (18.0f, bounds.getHeight() - 4.0f);
    auto box = juce::Rectangle<float> (bounds.getX() + (bounds.getWidth() - size) * 0.5f,
                                       bounds.getY() + (bounds.getHeight() - size) * 0.5f,
                                       size, size);

    auto base = juce::Colour::fromRGB (45, 45, 45);
    if (shouldDrawButtonAsDown) base = base.darker (0.2f);
    else if (shouldDrawButtonAsHighlighted) base = base.brighter (0.08f);

    g.setColour (base);
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawRoundedRectangle (box, 3.0f, 1.0f);

    if (button.getToggleState())
    {
        const auto led = box.reduced (4.0f);
        g.setColour (juce::Colour::fromRGB (232, 44, 39).withAlpha (0.28f));
        g.fillEllipse (led.expanded (2.0f));
        g.setColour (juce::Colour::fromRGB (245, 53, 48));
        g.fillEllipse (led);
    }
}

void IonLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                   int buttonX, int buttonY, int buttonW, int buttonH,
                                   juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (0.5f);
    auto background = box.findColour (juce::ComboBox::backgroundColourId);
    if (isButtonDown)
        background = background.darker (0.18f);

    g.setColour (background);
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    const auto arrowArea = juce::Rectangle<float> (static_cast<float> (buttonX), static_cast<float> (buttonY),
                                                    static_cast<float> (buttonW), static_cast<float> (buttonH)).reduced (5.0f);
    juce::Path arrow;
    arrow.startNewSubPath (arrowArea.getX(), arrowArea.getCentreY() - 2.0f);
    arrow.lineTo (arrowArea.getCentreX(), arrowArea.getCentreY() + 3.0f);
    arrow.lineTo (arrowArea.getRight(), arrowArea.getCentreY() - 2.0f);
    g.setColour (juce::Colour::fromRGB (225, 45, 42));
    g.strokePath (arrow, juce::PathStrokeType (1.6f));
}
}
