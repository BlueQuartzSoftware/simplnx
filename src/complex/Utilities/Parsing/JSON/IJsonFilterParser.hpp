#pragma once

#include <string>
#include <vector>

namespace complex
{
class AbstractFilter;
class FilterList;

/**
 * class IJsonFilterParser
 *
 */

class IJsonFilterParser
{
public:
  virtual ~IJsonFilterParser() = default;

  /**
   * @param json
   * @return AbstractFilter*
   */
  virtual AbstractFilter* fromJson(const std::string& json) const = 0;

  /**
   * @param  filter
   * @return std::string
   */
  virtual std::string toJson(AbstractFilter* filter) const = 0;
};
} // namespace complex
