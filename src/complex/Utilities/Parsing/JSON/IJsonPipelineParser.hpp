#pragma once

#include <string>
#include <vector>

namespace SIMPL
{
class FilterList;
class Pipeline;

class IJsonPipelineParser
{
public:
  ~IJsonPipelineParser();

  /**
   * @param json
   * @return SIMPL::Pipeline*
   */
  virtual SIMPL::Pipeline* fromJson(const std::string& json) const = 0;

  /**
   * @param pipeline
   * @return std::string
   */
  virtual std::string toJson(SIMPL::Pipeline* pipeline) const = 0;

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
} // namespace SIMPL
