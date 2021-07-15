#pragma once

#include "complex/Utilities/Parsing/JSON/IJsonFilterParser.h"

namespace complex
{

/**
 * class JsonFilterParserV7
 *
 */

class JsonFilterParserV7 : virtual public IJsonFilterParser
{
public:
  /**
   * Empty Constructor
   */
  JsonFilterParserV7();

  /**
   * Empty Destructor
   */
  virtual ~JsonFilterParserV7();

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
