#pragma once

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataMap.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/complex_export.hpp"

#include <vector>

namespace complex
{

/**
 * @brief RemoveInactiveObjects This assumes a single Dimension TupleShape, i.e., a Linear array, (1D).
 *
 * @param dataStructure
 * @param featureDataGroupPath
 * @param activeObjects
 * @param cellFeatureIds
 * @return
 */
COMPLEX_EXPORT bool RemoveInactiveObjects(DataStructure& dataStructure, DataPath& featureDataGroupPath, const std::vector<bool>& activeObjects, Int32Array& cellFeatureIds, size_t currentFeatureCount);

} // namespace complex
