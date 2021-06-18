#pragma once

#include "SIMPL/Utilities/Parsing/JSON/IJsonPipelineParser.h"

namespace SIMPL
{

/**
 * class JsonPipelineParserV7
 *
 */

class JsonPipelineParserV7 : virtual public IJsonPipelineParser
{
public:
  /**
   * @brief 
   * @param filterList 
  */
  JsonPipelineParserV7(FilterList* filterList);

  /**
   * @brief 
   * @param other
  */
  JsonPipelineParserV7(const JsonPipelineParserV7& other);

  /**
   * @brief 
   * @param other 
  */
  JsonPipelineParserV7(JsonPipelineParserV7&& other) noexcept;

  virtual ~JsonPipelineParserV7();

  /**
   * @param json
   * @return Pipeline*
   */
  SIMPL::Pipeline* fromJson(const std::string& json) const override;

  /**
   * @param pipeline*
   * @return std::string
   */
  std::string toJson(SIMPL::Pipeline* pipeline) const override;

protected:
private:
};
} // namespace SIMPL
