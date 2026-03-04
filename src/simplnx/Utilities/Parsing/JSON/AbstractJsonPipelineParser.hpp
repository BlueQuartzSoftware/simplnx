#pragma once

#include "simplnx/Utilities/Parsing/JSON/IJsonPipelineParser.hpp"

namespace nx::core
{
class FilterList;

/**
 * @brief Abstract base class for JSON pipeline parsers. Provides storage for the FilterList.
 */
class AbstractJsonPipelineParser : public IJsonPipelineParser
{
public:
  ~AbstractJsonPipelineParser() override;

protected:
  /**
   * @brief
   * @param filterList
   */
  AbstractJsonPipelineParser(FilterList* filterList);

  /**
   * @brief
   * @param other
   */
  AbstractJsonPipelineParser(const AbstractJsonPipelineParser& other);

  /**
   * @brief
   * @param other
   */
  AbstractJsonPipelineParser(AbstractJsonPipelineParser&& other) noexcept;

  /**
   * @brief
   * @return FilterList*
   */
  FilterList* getFilterList() const;

private:
  FilterList* m_FilterList;
};
} // namespace nx::core
