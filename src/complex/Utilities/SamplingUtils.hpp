#pragma once

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
namespace Sampling
{
static Result<> RenumberFeatures(DataStructure& dataStructure, const DataPath& newGeomPath, const DataPath& destCellFeatureGroupPath, const DataPath& featureIdsArrayPath)
{
  // Get the resampled data container, by default it would have been inserted into the Data Container Array
  auto* destImageGeom = dataStructure.getDataAs<ImageGeom>(newGeomPath);
  // This just sanity checks to make sure there were existing features before the cropping
  auto* destCellFeatureGroup = dataStructure.getDataAs<DataGroup>(destCellFeatureGroupPath);

  if(nullptr == destCellFeatureGroup)
  {
    return MakeErrorResult(-610, fmt::format("The Cell Feature Attribute Matrix '{}' was not found.", destCellFeatureGroupPath.toString()));
  }

  usize totalPoints = destImageGeom->getNumberOfElements();

  auto* featureIdsArray = dataStructure.getDataAs<IDataArray>(featureIdsArrayPath);
  usize totalFeatures = featureIdsArray->getNumberOfTuples();
  std::vector<bool> activeObjects(totalFeatures, false);
  if(0 == totalFeatures)
  {
    return MakeErrorResult(-600, "The number of Features is 0 and should be greater than 0");
  }

  // std::vector<usize> cDims(1, 1);
  auto destFeatureIdsPath = destCellFeatureGroupPath.createChildPath(featureIdsArrayPath.getTargetName());
  auto destFeatureIdsPtr = dataStructure.getDataAs<Int32Array>(destFeatureIdsPath);
  if(nullptr == destFeatureIdsPtr)
  {
    std::string ss = fmt::format("The FeatureIds array with name '{}' was not found in the destination DataContainer. The expected path was '{}'", featureIdsArrayPath.getTargetName(),
                                 destFeatureIdsPath.toString());
    return MakeErrorResult(-601, ss);
  }

  auto& featureIds = destFeatureIdsPtr->getDataStoreRef();
  // Find the unique set of feature ids
  for(usize i = 0; i < totalPoints; ++i)
  {
    // if(filter->getCancel())
    //{
    //  break;
    //}

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
