#include "EditorPage.h"

#include <algorithm>
#include <map>
#include <set>

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
        sections.push_back ({ "randomizer", std::move (section) });
        return;
    }

    std::map<juce::String, std::vector<const ParameterDefinition*>> groups;
    for (const auto* parameter : registry.parametersForPage (key))
        groups[groupKeyForSection (parameter->section)].push_back (parameter);

    const juce::StringArray preferredOrder {
        "oscillators", "pre_filter_mix", "filters", "post_filter_mix", "output",
        "effects", "envelopes", "modulators", "voice", "mod_matrix", "tracking_generator"
    };

    auto addGroup = [&] (const juce::String& group,
                         const std::vector<const ParameterDefinition*>& definitions)
    {
        std::unique_ptr<EditorSection> section;
        if (group == "oscillators")
            section = std::make_unique<OscillatorPanel> (registry, state);
        else if (group == "filters")
            section = std::make_unique<FilterPanel> (registry, state);
        else if (group == "envelopes")
            section = std::make_unique<EnvelopePanel> (registry, state);
        else if (group == "modulators")
            section = std::make_unique<ModulatorPanel> (registry, state);
        else if (group == "voice")
            section = std::make_unique<VoicePanel> (registry, state);
        else if (group == "effects")
            section = std::make_unique<EffectsPanel> (registry, state);
        else if (group == "pre_filter_mix")
            section = std::make_unique<MixerPanel> (MixerPanel::Mode::preFilter, registry, state);
        else if (group == "post_filter_mix")
            section = std::make_unique<MixerPanel> (MixerPanel::Mode::postFilter, registry, state);
        else if (group == "output")
            section = std::make_unique<OutputPanel> (registry, state);
        else if (group == "mod_matrix")
            section = std::make_unique<ModMatrixPanel> (registry, state);
        else if (group == "tracking_generator")
            section = std::make_unique<TrackingGeneratorPanel> (registry, state);
        else
            section = std::make_unique<SectionPanel> (titleForGroup (group), definitions, state);

        addAndMakeVisible (*section);
        sections.push_back ({ group, std::move (section) });
    };

    for (const auto& group : preferredOrder)
    {
        if (const auto found = groups.find (group); found != groups.end())
        {
            addGroup (group, found->second);
            groups.erase (found);
        }
    }

    for (const auto& [group, parameters] : groups)
        addGroup (group, parameters);
}

void EditorPage::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (162, 162, 159));

    auto top = getLocalBounds().removeFromTop (30).reduced (14, 3);
    auto titleArea = top.removeFromLeft (juce::jmax (280, top.getWidth() / 2));
    auto metaArea = top;

    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
    g.drawText (title, titleArea, juce::Justification::centredLeft, false);

    g.setColour (juce::Colours::black.withAlpha (0.46f));
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("shared JSON state / desktop + iPad touch surface", metaArea,
                juce::Justification::centredRight, false);

    // A faint signal-flow rail visually ties the purpose-built desktop layout
    // together without hard-coding protocol or parameter behavior into paint().
    if ((key == "front" || key == "dual1" || key == "dual2") && getWidth() >= 900)
    {
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.drawHorizontalLine (35, 18.0f, static_cast<float> (getWidth() - 18));
    }
}

void EditorPage::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (30);

    if (key == "randomizer")
    {
        if (auto* randomizer = sectionFor ("randomizer"))
            randomizer->setBounds (area.getX(), area.getY(), area.getWidth(),
                                   randomizer->preferredHeightForWidth (area.getWidth()));
        return;
    }

    if (key == "rear" && area.getWidth() >= 760)
    {
        constexpr int gap = 10;
        const auto leftWidth = static_cast<int> ((area.getWidth() - gap) * 0.57f);
        const auto rightWidth = area.getWidth() - gap - leftWidth;
        auto* matrix = sectionFor ("mod_matrix");
        auto* tracking = sectionFor ("tracking_generator");
        const auto matrixHeight = matrix != nullptr ? matrix->preferredHeightForWidth (leftWidth) : 0;
        const auto trackingHeight = tracking != nullptr ? tracking->preferredHeightForWidth (rightWidth) : 0;
        const auto height = juce::jmax (matrixHeight, trackingHeight);
        if (matrix != nullptr)
            matrix->setBounds (area.getX(), area.getY(), leftWidth, height);
        if (tracking != nullptr)
            tracking->setBounds (area.getX() + leftWidth + gap, area.getY(), rightWidth, height);
        return;
    }

    if (area.getWidth() >= 900 && (key == "front" || key == "dual1" || key == "dual2"))
    {
        constexpr int gap = 10;
        int y = area.getY();

        if (key == "front")
        {
            y = layoutRow (area, y, { { "oscillators", 0.36f }, { "pre_filter_mix", 0.24f }, { "filters", 0.40f } });
            y += gap;
            y = layoutRow (area, y, { { "modulators", 0.22f }, { "voice", 0.18f }, { "post_filter_mix", 0.22f },
                                      { "output", 0.18f }, { "effects", 0.20f } });
            y += gap;
            if (auto* envelopes = sectionFor ("envelopes"))
            {
                const auto h = envelopes->preferredHeightForWidth (area.getWidth());
                envelopes->setBounds (area.getX(), y, area.getWidth(), h);
                y += h + gap;
            }
            if (auto* matrix = sectionFor ("mod_matrix"))
                matrix->setBounds (area.getX(), y, area.getWidth(), matrix->preferredHeightForWidth (area.getWidth()));
            return;
        }

        if (key == "dual1")
        {
            y = layoutRow (area, y, { { "oscillators", 0.36f }, { "pre_filter_mix", 0.24f }, { "filters", 0.40f } });
            y += gap;
            y = layoutRow (area, y, { { "modulators", 0.25f }, { "voice", 0.20f }, { "post_filter_mix", 0.25f },
                                      { "output", 0.30f } });
            y += gap;
            if (auto* matrix = sectionFor ("mod_matrix"))
                matrix->setBounds (area.getX(), y, area.getWidth(), matrix->preferredHeightForWidth (area.getWidth()));
            return;
        }

        y = layoutRow (area, y, { { "pre_filter_mix", 0.24f }, { "filters", 0.38f },
                                  { "post_filter_mix", 0.22f }, { "output", 0.16f } });
        y += gap;
        y = layoutRow (area, y, { { "modulators", 0.30f }, { "voice", 0.25f }, { "effects", 0.45f } });
        y += gap;
        if (auto* envelopes = sectionFor ("envelopes"))
        {
            const auto h = envelopes->preferredHeightForWidth (area.getWidth());
            envelopes->setBounds (area.getX(), y, area.getWidth(), h);
            y += h + gap;
        }
        if (auto* matrix = sectionFor ("mod_matrix"))
            matrix->setBounds (area.getX(), y, area.getWidth(), matrix->preferredHeightForWidth (area.getWidth()));
        return;
    }

    layoutMasonry (area);
}

int EditorPage::preferredHeightForWidth (int width) const
{
    const auto usableWidth = juce::jmax (1, width - 24);
    constexpr int gap = 10;

    if (key == "randomizer")
    {
        if (auto* randomizer = sectionFor ("randomizer"))
            return 42 + randomizer->preferredHeightForWidth (usableWidth) + 12;
    }

    if (key == "rear" && usableWidth >= 760)
    {
        const auto leftWidth = static_cast<int> ((usableWidth - gap) * 0.57f);
        const auto rightWidth = usableWidth - gap - leftWidth;
        const auto* matrix = sectionFor ("mod_matrix");
        const auto* tracking = sectionFor ("tracking_generator");
        const auto matrixHeight = matrix != nullptr ? matrix->preferredHeightForWidth (leftWidth) : 0;
        const auto trackingHeight = tracking != nullptr ? tracking->preferredHeightForWidth (rightWidth) : 0;
        return 42 + juce::jmax (matrixHeight, trackingHeight) + 12;
    }

    if (usableWidth >= 900 && key == "front")
    {
        auto total = rowPreferredHeight (usableWidth, { { "oscillators", 0.36f }, { "pre_filter_mix", 0.24f }, { "filters", 0.40f } });
        total += gap + rowPreferredHeight (usableWidth, { { "modulators", 0.22f }, { "voice", 0.18f },
                                                           { "post_filter_mix", 0.22f }, { "output", 0.18f }, { "effects", 0.20f } });
        if (auto* envelopes = sectionFor ("envelopes")) total += gap + envelopes->preferredHeightForWidth (usableWidth);
        if (auto* matrix = sectionFor ("mod_matrix")) total += gap + matrix->preferredHeightForWidth (usableWidth);
        return 42 + total + 12;
    }

    if (usableWidth >= 900 && key == "dual1")
    {
        auto total = rowPreferredHeight (usableWidth, { { "oscillators", 0.36f }, { "pre_filter_mix", 0.24f }, { "filters", 0.40f } });
        total += gap + rowPreferredHeight (usableWidth, { { "modulators", 0.25f }, { "voice", 0.20f },
                                                           { "post_filter_mix", 0.25f }, { "output", 0.30f } });
        if (auto* matrix = sectionFor ("mod_matrix")) total += gap + matrix->preferredHeightForWidth (usableWidth);
        return 42 + total + 12;
    }

    if (usableWidth >= 900 && key == "dual2")
    {
        auto total = rowPreferredHeight (usableWidth, { { "pre_filter_mix", 0.24f }, { "filters", 0.38f },
                                                         { "post_filter_mix", 0.22f }, { "output", 0.16f } });
        total += gap + rowPreferredHeight (usableWidth, { { "modulators", 0.30f }, { "voice", 0.25f }, { "effects", 0.45f } });
        if (auto* envelopes = sectionFor ("envelopes")) total += gap + envelopes->preferredHeightForWidth (usableWidth);
        if (auto* matrix = sectionFor ("mod_matrix")) total += gap + matrix->preferredHeightForWidth (usableWidth);
        return 42 + total + 12;
    }

    return masonryPreferredHeight (width);
}

EditorSection* EditorPage::sectionFor (const juce::String& group) const
{
    const auto found = std::find_if (sections.begin(), sections.end(), [&] (const SectionEntry& entry)
    {
        return entry.group == group;
    });
    return found != sections.end() ? found->section.get() : nullptr;
}

int EditorPage::rowPreferredHeight (int width,
                                    std::initializer_list<std::pair<const char*, float>> groups) const
{
    constexpr int gap = 10;
    int count = 0;
    float totalWeight = 0.0f;
    for (const auto& [group, weight] : groups)
        if (sectionFor (group) != nullptr)
        {
            ++count;
            totalWeight += weight;
        }

    if (count == 0 || totalWeight <= 0.0f)
        return 0;

    const auto usable = juce::jmax (1, width - gap * (count - 1));
    int maximum = 0;
    for (const auto& [group, weight] : groups)
        if (auto* section = sectionFor (group))
        {
            const auto sectionWidth = juce::jmax (1, static_cast<int> (usable * weight / totalWeight));
            maximum = juce::jmax (maximum, section->preferredHeightForWidth (sectionWidth));
        }
    return maximum;
}

int EditorPage::layoutRow (juce::Rectangle<int> area,
                           int y,
                           std::initializer_list<std::pair<const char*, float>> groups)
{
    constexpr int gap = 10;
    int count = 0;
    float totalWeight = 0.0f;
    for (const auto& [group, weight] : groups)
        if (sectionFor (group) != nullptr)
        {
            ++count;
            totalWeight += weight;
        }

    if (count == 0 || totalWeight <= 0.0f)
        return y;

    const auto usableWidth = juce::jmax (1, area.getWidth() - gap * (count - 1));
    const auto rowHeight = rowPreferredHeight (area.getWidth(), groups);
    int x = area.getX();
    int remaining = usableWidth;
    int placed = 0;

    for (const auto& [group, weight] : groups)
    {
        auto* section = sectionFor (group);
        if (section == nullptr)
            continue;

        ++placed;
        const auto width = placed == count ? remaining
                                           : juce::jmax (1, static_cast<int> (usableWidth * weight / totalWeight));
        section->setBounds (x, y, width, rowHeight);
        x += width + gap;
        remaining -= width;
    }
    return y + rowHeight;
}

void EditorPage::layoutMasonry (juce::Rectangle<int> area)
{
    constexpr int gap = 10;
    const auto columns = columnCountForWidth (area.getWidth());
    const auto cellWidth = (area.getWidth() - gap * (columns - 1)) / columns;
    std::vector<int> columnY (static_cast<std::size_t> (columns), area.getY());

    for (const auto& entry : sections)
    {
        const auto shortest = std::min_element (columnY.begin(), columnY.end());
        const auto column = static_cast<int> (std::distance (columnY.begin(), shortest));
        const auto height = entry.section->preferredHeightForWidth (cellWidth);
        entry.section->setBounds (area.getX() + column * (cellWidth + gap), *shortest, cellWidth, height);
        *shortest += height + gap;
    }
}

int EditorPage::masonryPreferredHeight (int width) const
{
    const auto usableWidth = juce::jmax (1, width - 24);
    constexpr int gap = 10;
    const auto columns = columnCountForWidth (usableWidth);
    const auto cellWidth = juce::jmax (1, (usableWidth - gap * (columns - 1)) / columns);

    std::vector<int> columnHeights (static_cast<std::size_t> (columns), 0);
    for (const auto& entry : sections)
    {
        const auto shortest = std::min_element (columnHeights.begin(), columnHeights.end());
        *shortest += entry.section->preferredHeightForWidth (cellWidth) + gap;
    }

    const auto contentHeight = columnHeights.empty() ? 0 : *std::max_element (columnHeights.begin(), columnHeights.end());
    return 42 + juce::jmax (120, contentHeight) + 12;
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
