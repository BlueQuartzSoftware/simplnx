#pragma once

#include "complex/Utilities/Parsing/JSON/IJsonFilterParser.h"

namespace complex
{

/**
 * class JsonFilterParserV7
 *
 */

class JsonFilterParserV7 : public IJsonFilterParser
{
public:
  /**
   * Empty Constructor
   */
  JsonFilterParserV7();

  /**
   * Empty Destructor
   */
  ~JsonFilterParserV7() override;

  /**
   * @param json
   * @return AbstractFilter*
   */
  AbstractFilter* fromJson(const std::string& json) const override;

  /**
   * @param AbstractFilter*
   * @return std::string
   */
  std::string toJson(AbstractFilter* filter) const override;

protected:
private:
};
} // namespace complex
