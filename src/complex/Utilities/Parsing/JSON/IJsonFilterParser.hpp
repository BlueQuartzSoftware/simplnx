#pragma once

#include <string>
#include <vector>

namespace SIMPL
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
   * @return SIMPL::AbstractFilter*
   */
  virtual SIMPL::AbstractFilter* fromJson(const std::string& json) const = 0;

  /**
   * @param  filter
   * @return std::string
   */
  virtual std::string toJson(SIMPL::AbstractFilter* filter) const = 0;
};
} // namespace SIMPL
