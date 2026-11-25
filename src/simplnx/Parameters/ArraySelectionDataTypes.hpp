#pragma once

#include "simplnx/Common/DataTypeUtilities.hpp"
#include "simplnx/simplnx_export.hpp"

#include <initializer_list>
#include <utility>

namespace nx::core
{
/**
 * @brief Declares the element types for a single- or multi-array selection.
 *
 * Use StringOnly() when the selection accepts only string arrays, which have no element type.
 */
class SIMPLNX_EXPORT ArraySelectionDataTypes
{
public:
  ArraySelectionDataTypes() = delete;
  ArraySelectionDataTypes(const ArraySelectionDataTypes&) = default;
  ArraySelectionDataTypes(ArraySelectionDataTypes&&) noexcept = default;
  ArraySelectionDataTypes& operator=(const ArraySelectionDataTypes&) = default;
  ArraySelectionDataTypes& operator=(ArraySelectionDataTypes&&) noexcept = default;
  ~ArraySelectionDataTypes() noexcept = default;

  /**
   * @brief Declares the listed element types.
   * @param dataTypes Element types accepted by the selection.
   */
  ArraySelectionDataTypes(std::initializer_list<DataType> dataTypes)
  : m_DataTypes(dataTypes)
  {
  }

  /**
   * @brief Accepts a set from a helper such as GetAllDataTypes().
   * @param dataTypes Element types accepted by the selection.
   */
  ArraySelectionDataTypes(DataTypeSetType dataTypes)
  : m_DataTypes(std::move(dataTypes))
  {
  }

  /**
   * @brief Declares an empty element-type set for a selection of only string arrays.
   * @return The element-type declaration for a string-only selection.
   */
  static ArraySelectionDataTypes StringOnly()
  {
    return DataTypeSetType{};
  }

  const DataTypeSetType& dataTypes() const&
  {
    return m_DataTypes;
  }

  const DataTypeSetType& dataTypes() const&& = delete;

private:
  DataTypeSetType m_DataTypes;
};
} // namespace nx::core
