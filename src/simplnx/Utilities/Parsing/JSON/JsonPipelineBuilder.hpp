#pragma once

#include <string>
#include <vector>

namespace nx::core
{
class FilterList;
class JsonObject;
class Pipeline;

class JsonPipelineBuilder
{
public:
  enum class ParserVersion
  {
    V6,
    V7
  };

  /**
   * @brief
   * @param filterList
   */
  JsonPipelineBuilder(FilterList* filterList);

  /**
   * @brief
   * @param other
   */
  JsonPipelineBuilder(const JsonPipelineBuilder& other);

  /**
   * @brief
   * @param other
   */
  JsonPipelineBuilder(JsonPipelineBuilder&& other) noexcept;

  virtual ~JsonPipelineBuilder();

  /**
   * @param path
   * @return Pipeline*
   */
  Pipeline* loadPipelineFromFile(const std::string& path) const;

  /**
   * @param path
   * @param pipeline
   * @param version
   */
  void savePipelineToFile(const std::string& path, Pipeline* pipeline, ParserVersion version = ParserVersion::V7) const;

  /**
   * @param json
   * @return Pipeline*
   */
  Pipeline* fromJson(const std::string& json) const;

  /**
   * @param AbstractFilter*
   * @param version
   * @return std::string
   */
  std::string toJson(Pipeline* pipeline, ParserVersion version = ParserVersion::V7) const;

protected:
private:
  FilterList* m_FilterList;
};
} // namespace nx::core
