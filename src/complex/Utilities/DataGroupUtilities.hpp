#pragma once

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/complex_export.hpp"

#include <memory>
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

/**
 * @brief This function will gather all of the sibling DataArrays to the input DataPath, then filter out all the 'IgnoredDataPaths`
 * @param dataStructure The DataStructure to operate on
 * @param dataArrayPath The DataArrayPath to find all sibling DataArrays to
 * @param ignoredDataPaths Vector of DataPaths that should be remove from the final vector.
 * @return
 */
COMPLEX_EXPORT std::vector<std::shared_ptr<IDataArray>> GenerateDataArrayList(const DataStructure& dataStructure, const DataPath& dataArrayPath, const std::vector<DataPath>& ignoredDataPaths);

/**
 * @brief This function will return all the DataPaths within a BaseGroup that are of a certain type
 * @param dataStructure The DataStructure to use
 * @param parentGroup The parent group whose children you want to get
 * @param dataObjectType The type of children you want to get
 * @return std::optional<std::vector<DataPath>>  of child paths that meet the DataObjectType requirement if there no errors during the process.
 */
COMPLEX_EXPORT std::optional<std::vector<DataPath>> GetAllChildDataPaths(const DataStructure& dataStructure, const DataPath& parentGroup, DataObject::Type dataObjectType);

} // namespace complex
