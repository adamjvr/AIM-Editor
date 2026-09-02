#include "BankJson.h"
#include "Core/ProgramJson.h"

namespace aim
{
namespace
{
bool isHardwareBankId (const juce::String& id)
{
    return id == "red" || id == "green" || id == "blue" || id == "yellow" || id == "edit";
}
}
juce::String BankJson::encode (const ProgramBank& bank)
{
    return juce::JSON::toString (toVar (bank), false);
}

juce::var BankJson::toVar (const ProgramBank& bank)
{
    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty ("format", "aim-editor.bank");
    root->setProperty ("schema_version", 1);
    root->setProperty ("name", bank.getName());

    if (bank.getHardwareBank().isNotEmpty())
        root->setProperty ("hardware_bank", bank.getHardwareBank());
    else
        root->setProperty ("hardware_bank", juce::var());

    juce::Array<juce::var> programs;
    for (const auto slot : bank.occupiedSlots())
    {
        const auto* program = bank.programAt (slot);
        if (program == nullptr)
            continue;

        auto item = std::make_unique<juce::DynamicObject>();
        item->setProperty ("slot", slot);
        item->setProperty ("program", ProgramJson::toVar (*program));
        programs.add (juce::var (item.release()));
    }

    root->setProperty ("programs", juce::var (programs));
    return juce::var (root.release());
}

juce::Result BankJson::decode (const juce::String& jsonText, ProgramBank& destination)
{
    juce::var root;
    if (const auto result = juce::JSON::parse (jsonText, root); result.failed())
        return result;

    return fromVar (root, destination);
}

juce::Result BankJson::fromVar (const juce::var& value, ProgramBank& destination)
{
    const auto* root = value.getDynamicObject();
    if (root == nullptr)
        return juce::Result::fail ("Bank JSON root must be an object");

    if (root->getProperty ("format").toString() != "aim-editor.bank")
        return juce::Result::fail ("Unexpected bank JSON format");

    if (static_cast<int> (root->getProperty ("schema_version")) < 1)
        return juce::Result::fail ("Bank JSON schema_version must be >= 1");

    ProgramBank decoded;
    decoded.setName (root->getProperty ("name").toString());

    const auto hardwareBank = root->getProperty ("hardware_bank");
    if (! hardwareBank.isVoid())
    {
        const auto bankId = hardwareBank.toString();
        if (! isHardwareBankId (bankId))
            return juce::Result::fail ("Bank JSON hardware_bank is not one of red/green/blue/yellow/edit/null");
        decoded.setHardwareBank (bankId);
    }

    const auto* programs = root->getProperty ("programs").getArray();
    if (programs == nullptr)
        return juce::Result::fail ("Bank JSON requires a programs array");

    for (const auto& entry : *programs)
    {
        const auto* item = entry.getDynamicObject();
        if (item == nullptr)
            return juce::Result::fail ("Bank program entry must be an object");

        const auto slot = static_cast<int> (item->getProperty ("slot"));
        if (! ProgramBank::isValidSlot (slot))
            return juce::Result::fail ("Bank program slot is outside 0..127");

        if (decoded.isOccupied (slot))
            return juce::Result::fail ("Bank JSON contains duplicate slot " + juce::String (slot));

        IonProgram program;
        if (const auto result = ProgramJson::fromVar (item->getProperty ("program"), program); result.failed())
            return juce::Result::fail ("Could not decode bank slot " + juce::String (slot) + ": " + result.getErrorMessage());

        if (const auto result = decoded.setProgram (slot, program); result.failed())
            return result;
    }

    destination = std::move (decoded);
    return juce::Result::ok();
}
}
