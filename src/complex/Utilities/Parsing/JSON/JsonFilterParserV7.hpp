#pragma once

#include "SIMPL/Utilities/Parsing/JSON/IJsonFilterParser.h"

namespace SIMPL
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
  SIMPL::AbstractFilter* fromJson(const std::string& json) const override;

  /**
   * @param AbstractFilter*
   * @return std::string
   */
  std::string toJson(AbstractFilter* filter) const override;

protected:
private:
};
} // namespace SIMPL
