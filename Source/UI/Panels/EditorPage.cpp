#include "EditorPage.h"

#include <map>

namespace aim
{
EditorPage::EditorPage (juce::String pageKey,
                        juce::String pageTitle,
                        const ParameterRegistry& registry)
    : key (std::move (pageKey)), title (std::move (pageTitle))
{
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
            auto section = std::make_unique<SectionPanel> (titleForGroup (group), found->second);
            addAndMakeVisible (*section);
            sections.push_back (std::move (section));
            groups.erase (found);
        }
    }

    for (const auto& [group, parameters] : groups)
    {
        auto section = std::make_unique<SectionPanel> (titleForGroup (group), parameters);
        addAndMakeVisible (*section);
        sections.push_back (std::move (section));
    }

    if (key == "randomizer" && sections.empty())
    {
        std::vector<const ParameterDefinition*> none;
        auto section = std::make_unique<SectionPanel> ("Randomizer — editor feature", none);
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
    g.drawText ("AIM Editor — parameter model driven from JSON", top,
                juce::Justification::centredRight, false);
}

void EditorPage::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (42);

    const int gap = 10;
    const int columns = area.getWidth() >= 1100 ? 3 : (area.getWidth() >= 700 ? 2 : 1);
    const int cellWidth = (area.getWidth() - gap * (columns - 1)) / columns;
    constexpr int cellHeight = 226;

    for (int i = 0; i < static_cast<int> (sections.size()); ++i)
    {
        const auto column = i % columns;
        const auto row = i / columns;
        sections[static_cast<std::size_t> (i)]->setBounds (area.getX() + column * (cellWidth + gap),
                                                           area.getY() + row * (cellHeight + gap),
                                                           cellWidth,
                                                           cellHeight);
    }
}

int EditorPage::preferredHeightForWidth (int width) const
{
    const auto usable = juce::jmax (1, width - 24);
    const int columns = usable >= 1100 ? 3 : (usable >= 700 ? 2 : 1);
    const int rows = (static_cast<int> (sections.size()) + columns - 1) / columns;
    return 54 + juce::jmax (1, rows) * 236 + 12;
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
