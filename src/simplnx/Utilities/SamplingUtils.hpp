#pragma once

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

namespace nx::core::Sampling
{
inline Result<> RenumberFeatures(DataStructure& dataStructure, const DataPath& newGeomPath, const DataPath& destCellFeatAttributeMatrixPath, const DataPath& featureIdsArrayPath,
                                 const DataPath& destFeatureIdsArrayPath, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel = false)
{
  auto& destImageGeom = dataStructure.getDataRefAs<ImageGeom>(newGeomPath);
  // This just sanity checks to make sure there were existing features before the cropping
  auto& destCellFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(destCellFeatAttributeMatrixPath);

  usize totalPoints = destImageGeom.getNumberOfCells();

  auto& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(featureIdsArrayPath);
  usize totalFeatures = destCellFeatureAM.getNumberOfTuples();
  std::vector<bool> activeObjects(totalFeatures, false);
  if(0 == totalFeatures)
  {
    return MakeErrorResult(-600, "The number of Features is 0 and should be greater than 0");
  }

  auto& destFeatureIds = dataStructure.getDataAs<Int32Array>(destFeatureIdsArrayPath)->getDataStoreRef();
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
      std::string ss = fmt::format("FeatureIds values MUST be >= ZERO. Negative FeatureId found at index {} into the resampled feature ids array", i);
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

  Result<> removeResult = RemoveInactiveObjects(dataStructure, destCellFeatAttributeMatrixPath, activeObjects, destFeatureIds, totalFeatures, messageHandler, shouldCancel);
  if(removeResult.invalid())
  {
    return removeResult;
  }
  // RemoveInactiveObjects reports a cancelled compaction as success. This function has no work
  // left after the call, so a cancelled and a completed run both return an empty valid Result.
  return {};
}
} // namespace nx::core::Sampling
