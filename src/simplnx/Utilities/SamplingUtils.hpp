#pragma once

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>

namespace nx::core::sampling
{
/**
 * @brief Scans destination Feature Ids without changing the arrays or Feature AM.
 * @param dataStructure Owns the destination Feature Ids and Feature AM.
 * @param destFeatureAttributeMatrixPath Destination Feature AM with the original feature tuple count.
 * @param featureIdsArrayPath Source Feature Ids path for error messages.
 * @param destFeatureIdsArrayPath Destination Feature Ids with one component and one tuple per retained element.
 * @param activeObjects Receives one flag per feature. Use the flags only after a successful scan with no cancellation.
 * @param shouldCancel Stops the scan when requested.
 * @return Error -600 for zero features, -605 for negative IDs, -602 for IDs outside the Feature AM, or the store's first read failure.
 * @throws std::out_of_range if destFeatureAttributeMatrixPath or destFeatureIdsArrayPath does not exist.
 * @throws std::bad_cast if destFeatureAttributeMatrixPath is not an AttributeMatrix or destFeatureIdsArrayPath is not an Int32Array.
 */
[[nodiscard]] inline Result<> FindActiveFeatures(const DataStructure& dataStructure, const DataPath& destFeatureAttributeMatrixPath, const DataPath& featureIdsArrayPath,
                                                 const DataPath& destFeatureIdsArrayPath, std::vector<bool>& activeObjects, const std::atomic_bool& shouldCancel)
{
  const usize totalFeatures = dataStructure.getDataRefAs<AttributeMatrix>(destFeatureAttributeMatrixPath).getNumberOfTuples();
  if(0 == totalFeatures)
  {
    return MakeErrorResult(-600, "The number of Features is 0 and should be greater than 0");
  }

  const auto& destFeatureIds = dataStructure.getDataRefAs<Int32ArrayType>(destFeatureIdsArrayPath).getDataStoreRef();
  activeObjects.assign(totalFeatures, false);
  const usize totalPoints = destFeatureIds.getNumberOfTuples();
  constexpr usize k_BatchSize = 40000;
  std::vector<int32> batch(std::min(k_BatchSize, totalPoints));
  for(usize offset = 0; offset < totalPoints;)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_BatchSize, totalPoints - offset);
    auto readResult = destFeatureIds.copyIntoBuffer(offset, nonstd::span<int32>(batch.data(), count));
    if(readResult.invalid() || shouldCancel)
    {
      return readResult;
    }
    for(usize i = 0; i < count; ++i)
    {
      const int32 currentFeatureId = batch[i];
      if(currentFeatureId < 0)
      {
        return MakeErrorResult(-605, fmt::format("Feature Ids values must be zero or greater. Destination Feature Ids array '{}' has the negative value {} at index {}.",
                                                 destFeatureIdsArrayPath.toString(), currentFeatureId, offset + i));
      }
      const auto featureIndex = static_cast<usize>(currentFeatureId);
      if(featureIndex >= totalFeatures)
      {
        return MakeErrorResult(-602, fmt::format("The total number of Features from {} is {}, but a value of {} was found in DataArray {}.", destFeatureIdsArrayPath.getTargetName(), totalFeatures,
                                                 currentFeatureId, featureIdsArrayPath.toString()));
      }
      activeObjects[featureIndex] = true;
    }
    offset += count;
  }
  return {};
}

/**
 * @brief Renumbers destination Feature Ids and removes inactive Feature AM tuples for any geometry type.
 * @param dataStructure Owns the feature arrays and destination Feature AM.
 * @param destFeatureAttributeMatrixPath Destination Feature AM with the original feature tuple count.
 * @param featureIdsArrayPath Source Feature Ids path for error messages.
 * @param destFeatureIdsArrayPath Destination Feature Ids with one component and one tuple per retained element.
 * @param messageHandler Receives compaction messages.
 * @param shouldCancel Stops the Feature Ids scan and the compaction when requested.
 * @return The first scan or compaction failure, or a valid result if completed or cancelled.
 * @throws std::out_of_range if destFeatureAttributeMatrixPath or destFeatureIdsArrayPath does not exist.
 * @throws std::bad_cast if destFeatureAttributeMatrixPath is not an AttributeMatrix or destFeatureIdsArrayPath is not an Int32Array.
 */
inline Result<> RenumberFeatures(DataStructure& dataStructure, const DataPath& destFeatureAttributeMatrixPath, const DataPath& featureIdsArrayPath, const DataPath& destFeatureIdsArrayPath,
                                 const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel = false)
{
  std::vector<bool> activeObjects;
  auto scanResult = FindActiveFeatures(dataStructure, destFeatureAttributeMatrixPath, featureIdsArrayPath, destFeatureIdsArrayPath, activeObjects, shouldCancel);
  if(scanResult.invalid() || shouldCancel)
  {
    return scanResult;
  }
  auto& destFeatureIds = dataStructure.getDataRefAs<Int32ArrayType>(destFeatureIdsArrayPath).getDataStoreRef();
  return MergeResults(std::move(scanResult), RemoveInactiveObjects(dataStructure, destFeatureAttributeMatrixPath, activeObjects, destFeatureIds, activeObjects.size(), messageHandler, shouldCancel));
}

/**
 * @brief Checks image cell and Feature Ids tuple counts, then renumbers features with the geometry-independent overload.
 * @param dataStructure Owns the destination geometry, feature arrays, and Feature AM.
 * @param newGeomPath Destination image geometry with one cell per destination Feature Ids tuple.
 * @param destCellFeatAttributeMatrixPath Destination Feature AM with the original feature tuple count.
 * @param featureIdsArrayPath Source Feature Ids path for error messages.
 * @param destFeatureIdsArrayPath Destination Feature Ids with one component per tuple.
 * @param messageHandler Receives compaction messages.
 * @param shouldCancel Stops the Feature Ids scan and the compaction when requested.
 * @return Error -606 if the cell and tuple counts differ, or the shared renumbering result.
 * @throws std::out_of_range if newGeomPath, destCellFeatAttributeMatrixPath or destFeatureIdsArrayPath does not exist.
 * @throws std::bad_cast if newGeomPath is not an ImageGeom, destCellFeatAttributeMatrixPath is not an AttributeMatrix,
 * or destFeatureIdsArrayPath is not an Int32Array.
 */
inline Result<> RenumberFeatures(DataStructure& dataStructure, const DataPath& newGeomPath, const DataPath& destCellFeatAttributeMatrixPath, const DataPath& featureIdsArrayPath,
                                 const DataPath& destFeatureIdsArrayPath, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel = false)
{
  const auto& destImageGeom = dataStructure.getDataRefAs<ImageGeom>(newGeomPath);
  const auto& destFeatureIds = dataStructure.getDataRefAs<Int32ArrayType>(destFeatureIdsArrayPath);
  if(destImageGeom.getNumberOfCells() != destFeatureIds.getNumberOfTuples())
  {
    return MakeErrorResult(-606, fmt::format("Image geometry '{}' has {} cells, but destination Feature Ids array '{}' has {} tuples. Provide one Feature Ids tuple per image cell.",
                                             newGeomPath.toString(), destImageGeom.getNumberOfCells(), destFeatureIdsArrayPath.toString(), destFeatureIds.getNumberOfTuples()));
  }
  return RenumberFeatures(dataStructure, destCellFeatAttributeMatrixPath, featureIdsArrayPath, destFeatureIdsArrayPath, messageHandler, shouldCancel);
}
} // namespace nx::core::sampling
