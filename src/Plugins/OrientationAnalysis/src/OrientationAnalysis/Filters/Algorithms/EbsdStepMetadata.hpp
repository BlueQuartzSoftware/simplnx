#pragma once

#include "simplnx/Common/Types.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

namespace nx::core::ebsd_step_metadata
{
/**
 * @brief Captures the stored scan step before EbsdLib validates or uses it.
 */
template <class HeaderEntry>
class StepEntry : public HeaderEntry
{
public:
  StepEntry(const std::string& key, std::optional<float32> replacement)
  : HeaderEntry(key)
  , m_Replacement(replacement)
  {
    if(replacement.has_value())
    {
      this->setValue(*replacement);
    }
  }

  void parseValue(std::string& text) override
  {
    std::string numericText = text;
    if(!numericText.empty() && numericText.front() == ':')
    {
      numericText.erase(0, 1);
    }
    std::replace(numericText.begin(), numericText.end(), ',', '.');
    const char* beginPtr = numericText.c_str();
    char* endPtr = nullptr;
    const float32 value = std::strtof(beginPtr, &endPtr);
    if(endPtr != beginPtr)
    {
      while(std::isspace(static_cast<unsigned char>(*endPtr)) != 0)
      {
        ++endPtr;
      }
      if(*endPtr == '\0')
      {
        m_Stored = value;
        if(!std::isfinite(value) || !(value > 0.0F))
        {
          // A safe placeholder lets the header reader finish so the filter can
          // report the original value or use the selected spacing.
          std::string safeValue = std::to_string(m_Replacement.value_or(1.0F));
          HeaderEntry::parseValue(safeValue);
          this->setValue(m_Replacement.value_or(1.0F));
          return;
        }
      }
    }
    HeaderEntry::parseValue(text);
    m_Stored = this->getValue();
  }

  [[nodiscard]] float32 storedOr(float32 missing) const
  {
    return m_Stored.value_or(missing);
  }

private:
  std::optional<float32> m_Replacement;
  std::optional<float32> m_Stored;
};

template <class HeaderEntry, class Reader>
[[nodiscard]] auto Observe(Reader& reader, const std::string& key, std::optional<float32> replacement = {})
{
  auto entry = std::make_shared<StepEntry<HeaderEntry>>(key, replacement);
  reader.getHeaderMap().at(key) = entry;
  return entry;
}
} // namespace nx::core::EbsdStepMetadata
