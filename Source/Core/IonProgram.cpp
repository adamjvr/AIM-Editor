#include "IonProgram.h"

namespace aim
{
void IonProgram::setParameter (std::string id, juce::var value)
{
    parameters.insert_or_assign (std::move (id), std::move (value));
}

const juce::var* IonProgram::getParameter (std::string_view id) const
{
    if (const auto found = parameters.find (id); found != parameters.end())
        return &found->second;

    return nullptr;
}

void IonProgram::preserveUnknownByte (int offset, std::uint8_t value)
{
    if (offset >= 0)
        unknownBytes.insert_or_assign (offset, value);
}

void IonProgram::clear()
{
    name.clear();
    category.clear();
    parameters.clear();
    unknownBytes.clear();
}
}
