#include "MainEditor.h"

namespace aim
{
MainEditor::MainEditor (const ParameterRegistry& registry, IonMidiService& midiService)
    : controlBar (midiService)
{
    pages[0] = std::make_unique<EditorPage> ("front", "Front", registry);
    pages[1] = std::make_unique<EditorPage> ("dual1", "Dual 1", registry);
    pages[2] = std::make_unique<EditorPage> ("dual2", "Dual 2", registry);
    pages[3] = std::make_unique<EditorPage> ("randomizer", "Randomizer", registry);
    pages[4] = std::make_unique<EditorPage> ("rear", "Rear", registry);

    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);
    viewport.setViewedComponent (pages[0].get(), false);
    addAndMakeVisible (viewport);

    controlBar.onPageChanged = [this] (int pageIndex) { showPage (pageIndex); };
    addAndMakeVisible (controlBar);
}

void MainEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (155, 155, 152));
}

void MainEditor::resized()
{
    auto area = getLocalBounds();

    const auto controlHeight = getWidth() < 1000 ? 116 : (getHeight() < 620 ? 74 : 82);
    controlBar.setBounds (area.removeFromBottom (controlHeight));
    viewport.setBounds (area);
    updateViewedPageSize();
}

void MainEditor::showPage (int pageIndex)
{
    if (! juce::isPositiveAndBelow (pageIndex, static_cast<int> (pages.size())))
        return;

    currentPage = pageIndex;
    viewport.setViewedComponent (pages[static_cast<std::size_t> (currentPage)].get(), false);
    viewport.setViewPosition (0, 0);
    updateViewedPageSize();
}

void MainEditor::updateViewedPageSize()
{
    if (! juce::isPositiveAndBelow (currentPage, static_cast<int> (pages.size())))
        return;

    auto* page = pages[static_cast<std::size_t> (currentPage)].get();
    const auto width = juce::jmax (480, viewport.getWidth() - viewport.getScrollBarThickness());
    const auto height = juce::jmax (viewport.getHeight(), page->preferredHeightForWidth (width));
    page->setSize (width, height);
}
}
