#pragma once

#include <string>
#include <vector>

namespace nx::core
{
class FilterList;
class Pipeline;

class IJsonPipelineParser
{
public:
  ~IJsonPipelineParser();

  /**
   * @param json
   * @return Pipeline*
   */
  virtual Pipeline* fromJson(const std::string& json) const = 0;

  /**
   * @param pipeline
   * @return std::string
   */
  virtual std::string toJson(Pipeline* pipeline) const = 0;

protected:
  /**
   * @brief
   * @param filterList
   */
  IJsonPipelineParser(FilterList* filterList);

  /**
   * @brief
   * @param other
   */
  IJsonPipelineParser(const IJsonPipelineParser& other);

  /**
   * @brief
   * @param other
   */
  IJsonPipelineParser(IJsonPipelineParser&& other) noexcept;

  /**
   * @brief
   * @return FilterList*
   */
  FilterList* getFilterList() const;

private:
  FilterList* m_FilterList;
};
} // namespace nx::core
