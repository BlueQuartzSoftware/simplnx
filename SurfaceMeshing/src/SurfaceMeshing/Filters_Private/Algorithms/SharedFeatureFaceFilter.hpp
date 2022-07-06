#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  SharedFeatureFaceFilterInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFeatureFaceIdsArrayName = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayName_Key);
  inputValues.FaceFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_FaceFeatureAttributeMatrixName_Key);
  inputValues.SurfaceMeshFeatureFaceLabelsArrayName = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceLabelsArrayName_Key);
  inputValues.SurfaceMeshFeatureFaceNumTrianglesArrayName = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceNumTrianglesArrayName_Key);

  return SharedFeatureFaceFilter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT SharedFeatureFaceFilterInputValues
{
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFeatureFaceIdsArrayName;
  DataPath FaceFeatureAttributeMatrixName;
  DataPath SurfaceMeshFeatureFaceLabelsArrayName;
  DataPath SurfaceMeshFeatureFaceNumTrianglesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT SharedFeatureFaceFilter
{
public:
  SharedFeatureFaceFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SharedFeatureFaceFilterInputValues* inputValues);
  ~SharedFeatureFaceFilter() noexcept;

  SharedFeatureFaceFilter(const SharedFeatureFaceFilter&) = delete;
  SharedFeatureFaceFilter(SharedFeatureFaceFilter&&) noexcept = delete;
  SharedFeatureFaceFilter& operator=(const SharedFeatureFaceFilter&) = delete;
  SharedFeatureFaceFilter& operator=(SharedFeatureFaceFilter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SharedFeatureFaceFilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
