#include "ProgramJson.h"

namespace aim
{
juce::String ProgramJson::encode (const IonProgram& program)
{
    return juce::JSON::toString (toVar (program), false);
}

juce::var ProgramJson::toVar (const IonProgram& program)
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

    if (program.hasSourcePatchBytes())
    {
        auto sourcePatch = std::make_unique<juce::DynamicObject>();
        sourcePatch->setProperty ("format", "ion-decoded-patch-v1");
        juce::Array<juce::var> bytes;
        bytes.ensureStorageAllocated (static_cast<int> (program.getSourcePatchBytes().size()));
        for (const auto byte : program.getSourcePatchBytes())
            bytes.add (static_cast<int> (byte));
        sourcePatch->setProperty ("bytes", juce::var (bytes));
        rootObject->setProperty ("source_patch", juce::var (sourcePatch.release()));
    }
    else
    {
        rootObject->setProperty ("source_patch", juce::var());
    }

    return juce::var (rootObject.release());
}

juce::Result ProgramJson::decode (const juce::String& jsonText, IonProgram& destination)
{
    juce::var root;
    const auto parseResult = juce::JSON::parse (jsonText, root);
    if (parseResult.failed())
        return parseResult;

    return fromVar (root, destination);
}

juce::Result ProgramJson::fromVar (const juce::var& value, IonProgram& destination)
{
    const auto* object = value.getDynamicObject();
    if (object == nullptr)
        return juce::Result::fail ("Program JSON root must be an object");

    if (object->getProperty ("format").toString() != "aim-editor.program")
        return juce::Result::fail ("Unexpected program JSON format");

    const auto schemaVersion = static_cast<int> (object->getProperty ("schema_version"));
    if (schemaVersion < 1)
        return juce::Result::fail ("Program JSON schema_version must be >= 1");

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
            const auto rawValue = static_cast<int> (byteObject->getProperty ("value"));
            if (offset >= 0 && rawValue >= 0 && rawValue <= 255)
                decoded.preserveUnknownByte (offset, static_cast<std::uint8_t> (rawValue));
        }
    }

    const auto sourcePatchValue = object->getProperty ("source_patch");
    if (! sourcePatchValue.isVoid())
    {
        const auto* sourcePatch = sourcePatchValue.getDynamicObject();
        if (sourcePatch == nullptr
            || sourcePatch->getProperty ("format").toString() != "ion-decoded-patch-v1")
            return juce::Result::fail ("Program source_patch has an unsupported format");

        const auto* rawBytes = sourcePatch->getProperty ("bytes").getArray();
        if (rawBytes == nullptr)
            return juce::Result::fail ("Program source_patch requires a bytes array");

        if (rawBytes->size() != 378)
            return juce::Result::fail ("Program source_patch must contain exactly 378 decoded Ion patch bytes");

        IonProgram::RawPatchBytes patchBytes;
        patchBytes.reserve (static_cast<std::size_t> (rawBytes->size()));
        for (const auto& rawByte : *rawBytes)
        {
            const auto byte = static_cast<int> (rawByte);
            if (byte < 0 || byte > 255)
                return juce::Result::fail ("Program source_patch contains a byte outside 0..255");
            patchBytes.push_back (static_cast<std::uint8_t> (byte));
        }
        decoded.setSourcePatchBytes (std::move (patchBytes));
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

    current->setProperty (juce::Identifier (parts[parts.size() - 1]), value);
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
