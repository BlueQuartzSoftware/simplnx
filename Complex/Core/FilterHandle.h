#pragma once

#include <string>


namespace Complex
{
/**
 * @brief
 */
class FilterHandle
{
public:
  using FilterIdType = size_t;
  using PluginIdType = std::string;

  /**
   * @brief
   */
  FilterHandle();

  /**
   * @brief
   * @param filterName
   * @param filterId
   * @param pluginId
   */
  FilterHandle(const std::string& filterName, const FilterIdType& filterId, const PluginIdType& pluginId);

  /**
   * @brief
   * @param rhs
   */
  FilterHandle(const FilterHandle& rhs);

  /**
   * @brief
   * @param rhs
   */
  FilterHandle(FilterHandle&& rhs);

  ~FilterHandle();

  /**
   * @brief
   * @return
   */
  std::string getFilterName() const;

  /**
   * @brief
   * @return
   */
  FilterIdType getFilterId() const;

  /**
   * @brief
   * @return
   */
  PluginIdType getPluginId() const;

  /**
   * @brief
   * @return
   */
  bool isValid() const;

  bool operator<(const FilterHandle& rhs) const noexcept;

private:
  std::string m_FilterName;
  FilterIdType m_FilterId;
  PluginIdType m_PluginId;
};
} // namespace Complex

/**
 * @brief
 * @param lhs
 * @param rhs
 * @return
 */
bool operator==(const Complex::FilterHandle& lhs, const Complex::FilterHandle& rhs) noexcept;
