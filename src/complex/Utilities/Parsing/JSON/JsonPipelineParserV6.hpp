#pragma once

#include "SIMPL/Utilities/Parsing/JSON/IJsonPipelineParser.h"

namespace SIMPL
{

/**
 * class JsonPipelineParserV6
 *
 */

class JsonPipelineParserV6 : virtual public IJsonPipelineParser
{
public:
  /**
   * @brief
   * @param filterList
   */
  JsonPipelineParserV6(FilterList* filterList);

  /**
   * @brief
   * @param other
   */
  JsonPipelineParserV6(const JsonPipelineParserV6& other);

  /**
   * @brief
   * @param other
   */
  JsonPipelineParserV6(JsonPipelineParserV6&& other) noexcept;

  virtual ~JsonPipelineParserV6();

  /**
   * @param json
   * @return Pipeline*
   */
  SIMPL::Pipeline* fromJson(const std::string& json) const override;

  /**
   * @param Pipeline*
   * @return std::string
   */
  std::string toJson(SIMPL::Pipeline* pipeline) const override;

protected:
private:
};
} // namespace SIMPL
