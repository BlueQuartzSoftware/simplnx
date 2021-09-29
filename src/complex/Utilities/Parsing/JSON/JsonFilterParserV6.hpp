#pragma once

#include "complex/Utilities/Parsing/JSON/IJsonFilterParser.h"

namespace complex
{

/**
 * class JsonFilterParserV6
 *
 */

class JsonFilterParserV6 : public IJsonFilterParser
{
public:
  /**
   * @brief
   */
  JsonFilterParserV6();

  /**
   * Empty Destructor
   */
  ~JsonFilterParserV6() override;

  /**
   * @param json
   * @return AbstractFilter*
   * @param  json
   */
  AbstractFilter* fromJson(const std::string& json) const override;

  /**
   * @param AbstractFilter*
   * @return std::string
   */
  std::string toJson(AbstractFilter* filter) const;

protected:
private:
};
} // namespace complex
