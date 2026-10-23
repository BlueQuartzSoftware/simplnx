#pragma once

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

namespace nx::core::Sampling
{
/**
 * @brief Renumbers destination Feature Ids and removes inactive Feature AM tuples for any geometry type.
 * @param dataStructure Owns the feature arrays and destination Feature AM.
 * @param destFeatureAttributeMatrixPath Destination Feature AM with the original feature tuple count.
 * @param featureIdsArrayPath Source Feature Ids path for error messages.
 * @param destFeatureIdsArrayPath Destination Feature Ids with one component and one tuple per retained element.
 * @param messageHandler Receives compaction messages.
 * @param shouldCancel Stops the Feature Ids scan and the compaction when requested.
 * @return Error -600 for zero features, -605 for negative IDs, -602 for IDs outside the Feature AM, or a compaction result.
 * @throws std::out_of_range if destFeatureAttributeMatrixPath or destFeatureIdsArrayPath does not exist.
 * @throws std::bad_cast if destFeatureAttributeMatrixPath is not an AttributeMatrix or destFeatureIdsArrayPath is not an Int32Array.
 */
inline Result<> RenumberFeatures(DataStructure& dataStructure, const DataPath& destFeatureAttributeMatrixPath, const DataPath& featureIdsArrayPath, const DataPath& destFeatureIdsArrayPath,
                                 const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel = false)
{
  // Renumbering needs at least one feature tuple in the destination Feature Attribute Matrix.
  auto& destCellFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(destFeatureAttributeMatrixPath);

  usize totalFeatures = destCellFeatureAM.getNumberOfTuples();
  std::vector<bool> activeObjects(totalFeatures, false);
  if(0 == totalFeatures)
  {
    return MakeErrorResult(-600, "The number of Features is 0 and should be greater than 0");
  }

  auto& destFeatureIds = dataStructure.getDataRefAs<Int32Array>(destFeatureIdsArrayPath).getDataStoreRef();
  const usize totalPoints = destFeatureIds.getNumberOfTuples();
  // Find the unique set of feature ids
  for(usize i = 0; i < totalPoints; ++i)
  {
    if(shouldCancel)
    {
      break;
    }

    int32 currentFeatureId = destFeatureIds[i];
    if(currentFeatureId < 0)
    {
      std::string ss =
          fmt::format("Feature Ids values must be zero or greater. Destination Feature Ids array '{}' has the negative value {} at index {}.", destFeatureIdsArrayPath.toString(), currentFeatureId, i);
      return MakeErrorResult(-605, ss);
    }
    if(static_cast<usize>(currentFeatureId) < totalFeatures)
    {
      activeObjects[currentFeatureId] = true;
    }
    else
    {
      std::string ss = fmt::format("The total number of Features from {} is {}, but a value of {} was found in DataArray {}.", destFeatureIdsArrayPath.getTargetName(), totalFeatures, currentFeatureId,
                                   featureIdsArrayPath.toString());
      std::cout << ss;
      return MakeErrorResult(-602, ss);
    }
  }

  Result<> removeResult = RemoveInactiveObjects(dataStructure, destFeatureAttributeMatrixPath, activeObjects, destFeatureIds, totalFeatures, messageHandler, shouldCancel);
  if(removeResult.invalid())
  {
    return removeResult;
  }
  // RemoveInactiveObjects reports a cancelled compaction as success. This function has no work
  // left after the call, so a cancelled and a completed run both return an empty valid Result.
  return {};
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
  const auto& destFeatureIds = dataStructure.getDataRefAs<Int32Array>(destFeatureIdsArrayPath);
  if(destImageGeom.getNumberOfCells() != destFeatureIds.getNumberOfTuples())
  {
    return MakeErrorResult(-606, fmt::format("Image geometry '{}' has {} cells, but destination Feature Ids array '{}' has {} tuples. Provide one Feature Ids tuple per image cell.",
                                             newGeomPath.toString(), destImageGeom.getNumberOfCells(), destFeatureIdsArrayPath.toString(), destFeatureIds.getNumberOfTuples()));
  }
  return RenumberFeatures(dataStructure, destCellFeatAttributeMatrixPath, featureIdsArrayPath, destFeatureIdsArrayPath, messageHandler, shouldCancel);
}
} // namespace nx::core::Sampling
