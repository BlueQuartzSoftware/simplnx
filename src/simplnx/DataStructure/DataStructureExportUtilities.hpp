#pragma once
/**
 * @file DataStructureExportUtilities.hpp
 * @brief Defines shared implementation helpers for DataStructure hierarchy exporters.
 */

#include "simplnx/DataStructure/DataMap.hpp"
#include "simplnx/DataStructure/DataObject.hpp"

#include <algorithm>
#include <vector>

/**
 * @namespace nx::core::data_structure_export
 * @brief Contains implementation helpers shared by hierarchy exporters.
 */
namespace nx::core::data_structure_export
{
/**
 * @brief Gets non-null objects in alphabetic order.
 * @param dataMap Map to inspect.
 * @return Object pointers sorted by name.
 */
[[nodiscard]] inline std::vector<const DataObject*> GetSortedObjectPointers(const DataMap& dataMap)
{
  std::vector<const DataObject*> sortedObjectPtrs;
  sortedObjectPtrs.reserve(dataMap.getSize());
  for(const auto& entry : dataMap)
  {
    if(entry.second != nullptr)
    {
      sortedObjectPtrs.emplace_back(entry.second.get());
    }
  }
  std::ranges::sort(sortedObjectPtrs, [](const DataObject* lhsPtr, const DataObject* rhsPtr) { return lhsPtr->getName() < rhsPtr->getName(); });
  return sortedObjectPtrs;
}
} // namespace nx::core::data_structure_export
