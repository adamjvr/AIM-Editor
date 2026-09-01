#include "ParameterRegistry.h"

#include <algorithm>
#include <set>

namespace aim
{
namespace
{
const juce::DynamicObject* objectOf (const juce::var& value)
{
    return value.getDynamicObject();
}

std::optional<int> optionalInt (const juce::var& value)
{
    if (value.isVoid())
        return std::nullopt;

    if (value.isInt() || value.isInt64() || value.isDouble())
        return static_cast<int> (value);

    return std::nullopt;
}

std::optional<double> optionalDouble (const juce::var& value)
{
    if (value.isVoid())
        return std::nullopt;

    if (value.isInt() || value.isInt64() || value.isDouble())
        return static_cast<double> (value);

    return std::nullopt;
}

juce::String stringProperty (const juce::DynamicObject& object, const juce::Identifier& name)
{
    return object.getProperty (name).toString();
}
}

juce::Result ParameterRegistry::loadFromJson (const juce::String& jsonText)
{
    definitions.clear();
    indexById.clear();

    juce::var root;
    const auto parseResult = juce::JSON::parse (jsonText, root);
    if (parseResult.failed())
        return parseResult;

    const auto* rootObject = objectOf (root);
    if (rootObject == nullptr)
        return juce::Result::fail ("Parameter database root must be a JSON object");

    if (rootObject->getProperty ("format").toString() != "aim-editor.parameter-database")
        return juce::Result::fail ("Unexpected parameter database format");

    const auto parametersValue = rootObject->getProperty ("parameters");
    const auto* parameters = parametersValue.getArray();
    if (parameters == nullptr)
        return juce::Result::fail ("Parameter database must contain a parameters array");

    std::set<std::string> seenIds;

    for (const auto& item : *parameters)
    {
        const auto* parameterObject = objectOf (item);
        if (parameterObject == nullptr)
            return juce::Result::fail ("Every parameter entry must be an object");

        ParameterDefinition definition;
        definition.id = stringProperty (*parameterObject, "id").toStdString();
        definition.name = stringProperty (*parameterObject, "name");
        definition.section = stringProperty (*parameterObject, "section");
        definition.kind = parameterKindFromString (stringProperty (*parameterObject, "type"));

        if (definition.id.empty() || definition.name.isEmpty() || definition.section.isEmpty())
            return juce::Result::fail ("Parameter entries require id, name, and section");

        if (! seenIds.insert (definition.id).second)
            return juce::Result::fail ("Duplicate parameter id: " + juce::String (definition.id));

        if (const auto* domain = objectOf (parameterObject->getProperty ("domain")))
        {
            definition.rawMin = optionalDouble (domain->getProperty ("raw_min"));
            definition.rawMax = optionalDouble (domain->getProperty ("raw_max"));
            definition.defaultRaw = optionalDouble (domain->getProperty ("default_raw"));

            if (const auto* values = domain->getProperty ("values").getArray())
            {
                for (const auto& item : *values)
                {
                    const auto* valueObject = objectOf (item);
                    if (valueObject == nullptr)
                        continue;

                    const auto raw = optionalInt (valueObject->getProperty ("raw"));
                    if (! raw)
                        continue;

                    ParameterEnumValue enumValue;
                    enumValue.raw = *raw;
                    enumValue.id = valueObject->getProperty ("id").toString();
                    enumValue.name = valueObject->getProperty ("name").toString();
                    if (enumValue.name.isEmpty())
                        enumValue.name = enumValue.id.replaceCharacter ('_', ' ');
                    definition.enumValues.push_back (std::move (enumValue));
                }
            }
        }

        if (const auto* display = objectOf (parameterObject->getProperty ("display")))
        {
            definition.unit = display->getProperty ("unit").toString();
            definition.displayTransform = display->getProperty ("transform").toString();
        }

        if (const auto* protocol = objectOf (parameterObject->getProperty ("protocol")))
        {
            definition.mappingStatus = mappingStatusFromString (protocol->getProperty ("status").toString());
            definition.nrpn = optionalInt (protocol->getProperty ("nrpn"));

            if (const auto* nrpnEvidence = objectOf (protocol->getProperty ("nrpn_evidence")))
            {
                definition.nrpnMin = optionalDouble (nrpnEvidence->getProperty ("min"));
                definition.nrpnMax = optionalDouble (nrpnEvidence->getProperty ("max"));
                definition.nrpnValueEncoding = nrpnEvidence->getProperty ("value_encoding").toString();
                definition.nrpnSourceId = nrpnEvidence->getProperty ("source_id").toString();
            }

            if (const auto* sysex = objectOf (protocol->getProperty ("sysex")))
            {
                definition.sysex.offset = optionalInt (sysex->getProperty ("offset"));
                definition.sysex.bits = optionalInt (sysex->getProperty ("bits"));
                definition.sysex.widthBytes = optionalInt (sysex->getProperty ("width_bytes"));
                definition.sysex.mask = optionalInt (sysex->getProperty ("mask"));
                definition.sysex.shift = optionalInt (sysex->getProperty ("shift"));
                definition.sysex.encoding = sysex->getProperty ("encoding").toString();
                definition.sysex.fieldId = sysex->getProperty ("field_id").toString();
            }
        }

        if (const auto* ui = objectOf (parameterObject->getProperty ("ui")))
        {
            definition.control = ui->getProperty ("control").toString();

            if (const auto* pages = ui->getProperty ("pages").getArray())
                for (const auto& page : *pages)
                    definition.pages.push_back (page.toString());
        }

        indexById.emplace (definition.id, definitions.size());
        definitions.push_back (std::move (definition));
    }

    return juce::Result::ok();
}

const ParameterDefinition* ParameterRegistry::find (std::string_view id) const
{
    if (const auto found = indexById.find (id); found != indexById.end())
        return &definitions[found->second];

    return nullptr;
}

const ParameterDefinition* ParameterRegistry::findByNrpn (int nrpn) const
{
    const auto found = std::find_if (definitions.begin(), definitions.end(),
                                     [nrpn] (const ParameterDefinition& definition)
                                     {
                                         return definition.nrpn && *definition.nrpn == nrpn;
                                     });
    return found != definitions.end() ? &*found : nullptr;
}

std::vector<const ParameterDefinition*> ParameterRegistry::parametersForPage (const juce::String& page) const
{
    std::vector<const ParameterDefinition*> result;

    for (const auto& parameter : definitions)
        if (std::find (parameter.pages.begin(), parameter.pages.end(), page) != parameter.pages.end())
            result.push_back (&parameter);

    return result;
}

std::vector<const ParameterDefinition*> ParameterRegistry::parametersForSection (const juce::String& section) const
{
    std::vector<const ParameterDefinition*> result;

    for (const auto& parameter : definitions)
        if (parameter.section == section)
            result.push_back (&parameter);

    return result;
}
}
