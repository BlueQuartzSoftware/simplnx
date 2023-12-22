#pragma once

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

namespace nx::core
{
namespace Sampling
{
inline Result<> RenumberFeatures(DataStructure& dataStructure, const DataPath& newGeomPath, const DataPath& destCellFeatAttributeMatrixPath, const DataPath& featureIdsArrayPath,
                                 const DataPath& destFeatureIdsArrayPath, const std::atomic_bool& shouldCancel = false)
{
  auto& destImageGeom = dataStructure.getDataRefAs<ImageGeom>(newGeomPath);
  // This just sanity checks to make sure there were existing features before the cropping
  auto& destCellFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(destCellFeatAttributeMatrixPath);

  usize totalPoints = destImageGeom.getNumberOfCells();

  auto& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(featureIdsArrayPath);
  usize totalFeatures = destCellFeatureAM.getNumTuples();
  std::vector<bool> activeObjects(totalFeatures, false);
  if(0 == totalFeatures)
  {
    return MakeErrorResult(-600, "The number of Features is 0 and should be greater than 0");
  }

  auto& destFeatureIdsRef = dataStructure.getDataRefAs<Int32Array>(destFeatureIdsArrayPath);

  auto& featureIds = destFeatureIdsRef.getDataStoreRef();
  // Find the unique set of feature ids
  for(usize i = 0; i < totalPoints; ++i)
  {
    if(shouldCancel)
    {
      break;
    }

    int32 currentFeatureId = featureIds[i];
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

  if(!RemoveInactiveObjects(dataStructure, destCellFeatAttributeMatrixPath, activeObjects, destFeatureIdsRef, totalFeatures))
  {
    std::string ss = fmt::format("An error occurred while trying to remove the inactive objects from Attribute Matrix '{}'", destCellFeatAttributeMatrixPath.toString());
    return MakeErrorResult(-606, ss);
  }
  return {};
}
} // namespace Sampling
} // namespace nx::core
