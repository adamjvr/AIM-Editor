#include "EditorPage.h"

#include <algorithm>
#include <map>

namespace aim
{
EditorPage::EditorPage (juce::String pageKey,
                        juce::String pageTitle,
                        const ParameterRegistry& registry,
                        ProgramState& state)
    : key (std::move (pageKey)), title (std::move (pageTitle))
{
    if (key == "randomizer")
    {
        auto section = std::make_unique<RandomizerPanel> (registry, state);
        addAndMakeVisible (*section);
        sections.push_back (std::move (section));
        return;
    }

    std::map<juce::String, std::vector<const ParameterDefinition*>> groups;

    for (const auto* parameter : registry.parametersForPage (key))
        groups[groupKeyForSection (parameter->section)].push_back (parameter);

    const juce::StringArray preferredOrder {
        "oscillators", "pre_filter_mix", "filters", "post_filter_mix", "output",
        "effects", "envelopes", "modulators", "voice", "mod_matrix", "tracking_generator"
    };

    for (const auto& group : preferredOrder)
    {
        if (const auto found = groups.find (group); found != groups.end())
        {
            std::unique_ptr<EditorSection> section;
            if (group == "mod_matrix")
                section = std::make_unique<ModMatrixPanel> (registry, state);
            else if (group == "tracking_generator")
                section = std::make_unique<TrackingGeneratorPanel> (registry, state);
            else
                section = std::make_unique<SectionPanel> (titleForGroup (group), found->second, state);

            addAndMakeVisible (*section);
            sections.push_back (std::move (section));
            groups.erase (found);
        }
    }

    for (const auto& [group, parameters] : groups)
    {
        auto section = std::make_unique<SectionPanel> (titleForGroup (group), parameters, state);
        addAndMakeVisible (*section);
        sections.push_back (std::move (section));
    }

}

void EditorPage::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (162, 162, 159));

    auto top = getLocalBounds().removeFromTop (42).reduced (14, 6);
    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.setFont (juce::FontOptions (18.0f));
    g.drawText (title, top, juce::Justification::centredLeft, false);

    g.setColour (juce::Colours::black.withAlpha (0.46f));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("AIM Editor — live semantic state driven from JSON", top,
                juce::Justification::centredRight, false);
}

void EditorPage::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (42);

    constexpr int gap = 10;
    const auto columns = columnCountForWidth (area.getWidth());
    const auto cellWidth = (area.getWidth() - gap * (columns - 1)) / columns;

    std::vector<int> columnY (static_cast<std::size_t> (columns), area.getY());

    for (const auto& section : sections)
    {
        const auto shortest = std::min_element (columnY.begin(), columnY.end());
        const auto column = static_cast<int> (std::distance (columnY.begin(), shortest));
        const auto height = section->preferredHeightForWidth (cellWidth);
        section->setBounds (area.getX() + column * (cellWidth + gap), *shortest, cellWidth, height);
        *shortest += height + gap;
    }
}

int EditorPage::preferredHeightForWidth (int width) const
{
    const auto usableWidth = juce::jmax (1, width - 24);
    constexpr int gap = 10;
    const auto columns = columnCountForWidth (usableWidth);
    const auto cellWidth = juce::jmax (1, (usableWidth - gap * (columns - 1)) / columns);

    std::vector<int> columnHeights (static_cast<std::size_t> (columns), 0);
    for (const auto& section : sections)
    {
        const auto shortest = std::min_element (columnHeights.begin(), columnHeights.end());
        *shortest += section->preferredHeightForWidth (cellWidth) + gap;
    }

    const auto contentHeight = columnHeights.empty() ? 0 : *std::max_element (columnHeights.begin(), columnHeights.end());
    return 54 + juce::jmax (120, contentHeight) + 12;
}

int EditorPage::columnCountForWidth (int width) const
{
    if (key == "randomizer")
        return 1;
    return width >= 1100 ? 3 : (width >= 700 ? 2 : 1);
}

juce::String EditorPage::groupKeyForSection (const juce::String& section)
{
    if (section.startsWith ("osc")) return "oscillators";
    if (section == "filter1" || section == "filter2" || section == "filters") return "filters";
    if (section.startsWith ("env_")) return "envelopes";
    if (section.startsWith ("lfo") || section == "tempo_arp" || section == "sample_hold") return "modulators";
    return section;
}

juce::String EditorPage::titleForGroup (const juce::String& group)
{
    if (group == "oscillators") return "OSC";
    if (group == "pre_filter_mix") return "PRE FILTER MIX";
    if (group == "filters") return "FILTER";
    if (group == "post_filter_mix") return "POST FILTER MIX";
    if (group == "output") return "OUTPUT";
    if (group == "effects") return "EFFECTS";
    if (group == "envelopes") return "ENVELOPES";
    if (group == "modulators") return "LFO / ARP / S&H";
    if (group == "voice") return "VOICE";
    if (group == "mod_matrix") return "MOD MATRIX";
    if (group == "tracking_generator") return "TRACKING GENERATOR";
    return group.toUpperCase().replaceCharacter ('_', ' ');
}
}
