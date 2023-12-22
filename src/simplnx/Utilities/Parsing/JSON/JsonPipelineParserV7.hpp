#pragma once

#include "simplnx/Utilities/Parsing/JSON/IJsonPipelineParser.h"

namespace nx::core
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

  ~JsonPipelineParserV7() override;

  /**
   * @param json
   * @return Pipeline*
   */
  Pipeline* fromJson(const std::string& json) const override;

  /**
   * @param pipeline*
   * @return std::string
   */
  std::string toJson(Pipeline* pipeline) const override;

protected:
private:
};
} // namespace nx::core
