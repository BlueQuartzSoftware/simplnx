#pragma once

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
namespace Sampling
{
inline Result<> RenumberFeatures(DataStructure& dataStructure, const DataPath& newGeomPath, const DataPath& destCellFeatureGroupPath, const DataPath& featureIdsArrayPath,
                                 const std::atomic_bool& shouldCancel = false)
{
  auto& destImageGeom = dataStructure.getDataRefAs<ImageGeom>(newGeomPath);
  // This just sanity checks to make sure there were existing features before the cropping
  auto& destCellFeatureGroup = dataStructure.getDataRefAs<DataGroup>(destCellFeatureGroupPath);

  usize totalPoints = destImageGeom.getNumberOfElements();

  auto& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(featureIdsArrayPath);
  usize totalFeatures = featureIdsArray.getNumberOfTuples();
  std::vector<bool> activeObjects(totalFeatures, false);
  if(0 == totalFeatures)
  {
    return MakeErrorResult(-600, "The number of Features is 0 and should be greater than 0");
  }

  auto destFeatureIdsPath = destCellFeatureGroupPath.createChildPath(featureIdsArrayPath.getTargetName());
  auto& destFeatureIdsRef = dataStructure.getDataRefAs<Int32Array>(destFeatureIdsPath);

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
      std::string ss = fmt::format("The total number of Features from {} is {}, but a value of {} was found in DataArray {}.", destFeatureIdsPath.getTargetName(), totalFeatures, currentFeatureId,
                                   featureIdsArrayPath.toString());
      std::cout << ss;
      return MakeErrorResult(-602, ss);
    }
  }
  return {};
}
} // namespace Sampling
} // namespace complex
