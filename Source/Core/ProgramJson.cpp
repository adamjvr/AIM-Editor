#include "ProgramJson.h"

namespace aim
{
juce::String ProgramJson::encode (const IonProgram& program)
{
    auto rootObject = std::make_unique<juce::DynamicObject>();
    rootObject->setProperty ("format", "aim-editor.program");
    rootObject->setProperty ("schema_version", 1);
    rootObject->setProperty ("name", program.getName());
    rootObject->setProperty ("category", program.getCategory());

    auto parametersObject = std::make_unique<juce::DynamicObject>();
    for (const auto& [id, value] : program.getParameters())
        insertNestedParameter (*parametersObject, id, value);

    rootObject->setProperty ("parameters", juce::var (parametersObject.release()));

    juce::Array<juce::var> unknownBytes;
    for (const auto& [offset, value] : program.getUnknownBytes())
    {
        auto byteObject = std::make_unique<juce::DynamicObject>();
        byteObject->setProperty ("offset", offset);
        byteObject->setProperty ("value", static_cast<int> (value));
        unknownBytes.add (juce::var (byteObject.release()));
    }

    rootObject->setProperty ("unmapped_bytes", juce::var (unknownBytes));
    return juce::JSON::toString (juce::var (rootObject.release()), false);
}

juce::Result ProgramJson::decode (const juce::String& jsonText, IonProgram& destination)
{
    juce::var root;
    const auto parseResult = juce::JSON::parse (jsonText, root);
    if (parseResult.failed())
        return parseResult;

    const auto* object = root.getDynamicObject();
    if (object == nullptr)
        return juce::Result::fail ("Program JSON root must be an object");

    if (object->getProperty ("format").toString() != "aim-editor.program")
        return juce::Result::fail ("Unexpected program JSON format");

    IonProgram decoded;
    decoded.setName (object->getProperty ("name").toString());
    decoded.setCategory (object->getProperty ("category").toString());

    const auto parameters = object->getProperty ("parameters");
    if (parameters.getDynamicObject() == nullptr)
        return juce::Result::fail ("Program JSON requires a parameters object");

    flattenParameterObject (parameters, {}, decoded);

    if (const auto* unknown = object->getProperty ("unmapped_bytes").getArray())
    {
        for (const auto& item : *unknown)
        {
            const auto* byteObject = item.getDynamicObject();
            if (byteObject == nullptr)
                continue;

            const auto offset = static_cast<int> (byteObject->getProperty ("offset"));
            const auto value = static_cast<int> (byteObject->getProperty ("value"));
            if (offset >= 0 && value >= 0 && value <= 255)
                decoded.preserveUnknownByte (offset, static_cast<std::uint8_t> (value));
        }
    }

    destination = std::move (decoded);
    return juce::Result::ok();
}

void ProgramJson::insertNestedParameter (juce::DynamicObject& root,
                                         const std::string& dottedId,
                                         const juce::var& value)
{
    auto parts = juce::StringArray::fromTokens (juce::String (dottedId), ".", {});
    parts.removeEmptyStrings();

    if (parts.isEmpty())
        return;

    auto* current = &root;

    for (int i = 0; i < parts.size() - 1; ++i)
    {
        const juce::Identifier key (parts[i]);
        auto childValue = current->getProperty (key);
        auto* child = childValue.getDynamicObject();

        if (child == nullptr)
        {
            auto newChild = std::make_unique<juce::DynamicObject>();
            child = newChild.get();
            current->setProperty (key, juce::var (newChild.release()));
        }

        current = child;
    }

    current->setProperty (juce::Identifier (parts.getLast()), value);
}

void ProgramJson::flattenParameterObject (const juce::var& value,
                                          const juce::String& prefix,
                                          IonProgram& destination)
{
    const auto* object = value.getDynamicObject();
    if (object == nullptr)
    {
        if (prefix.isNotEmpty())
            destination.setParameter (prefix.toStdString(), value);
        return;
    }

    const auto& properties = object->getProperties();
    for (int i = 0; i < properties.size(); ++i)
    {
        const auto childName = properties.getName (i).toString();
        const auto childPrefix = prefix.isEmpty() ? childName : prefix + "." + childName;
        const auto& childValue = properties.getValueAt (i);

        if (childValue.getDynamicObject() != nullptr)
            flattenParameterObject (childValue, childPrefix, destination);
        else
            destination.setParameter (childPrefix.toStdString(), childValue);
    }
}
}
