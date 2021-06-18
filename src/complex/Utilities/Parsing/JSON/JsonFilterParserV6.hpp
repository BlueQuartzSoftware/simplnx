#pragma once

#include "SIMPL/Utilities/Parsing/JSON/IJsonFilterParser.h"

namespace SIMPL
{

/**
 * class JsonFilterParserV6
 *
 */

class JsonFilterParserV6 : virtual public IJsonFilterParser
{
public:
  /**
   * @brief
   */
  JsonFilterParserV6();

  /**
   * Empty Destructor
   */
  virtual ~JsonFilterParserV6();

  /**
   * @param json
   * @return AbstractFilter*
   * @param  json
   */
  SIMPL::AbstractFilter* fromJson(const std::string& json) const override;

  /**
   * @param AbstractFilter*
   * @return std::string
   */
  std::string toJson(AbstractFilter* filter) const;

protected:
private:
};
} // namespace SIMPL
