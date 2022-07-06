#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FeatureFaceCurvatureFilterInputValues inputValues;

  inputValues.NRing = filterArgs.value<int32>(k_NRing_Key);
  inputValues.ComputePrincipalDirectionVectors = filterArgs.value<bool>(k_ComputePrincipalDirectionVectors_Key);
  inputValues.ComputeGaussianCurvature = filterArgs.value<bool>(k_ComputeGaussianCurvature_Key);
  inputValues.ComputeMeanCurvature = filterArgs.value<bool>(k_ComputeMeanCurvature_Key);
  inputValues.UseNormalsForCurveFitting = filterArgs.value<bool>(k_UseNormalsForCurveFitting_Key);
  inputValues.FaceAttributeMatrixPath = filterArgs.value<DataPath>(k_FaceAttributeMatrixPath_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFeatureFaceIdsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayPath_Key);
  inputValues.SurfaceMeshFaceNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.SurfaceMeshTriangleCentroidsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshTriangleCentroidsArrayPath_Key);
  inputValues.SurfaceMeshPrincipalCurvature1sArrayName = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalCurvature1sArrayName_Key);
  inputValues.SurfaceMeshPrincipalCurvature2sArrayName = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalCurvature2sArrayName_Key);
  inputValues.SurfaceMeshPrincipalDirection1sArrayName = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalDirection1sArrayName_Key);
  inputValues.SurfaceMeshPrincipalDirection2sArrayName = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalDirection2sArrayName_Key);
  inputValues.SurfaceMeshGaussianCurvaturesArrayName = filterArgs.value<DataPath>(k_SurfaceMeshGaussianCurvaturesArrayName_Key);
  inputValues.SurfaceMeshMeanCurvaturesArrayName = filterArgs.value<DataPath>(k_SurfaceMeshMeanCurvaturesArrayName_Key);

  return FeatureFaceCurvatureFilter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT FeatureFaceCurvatureFilterInputValues
{
  int32 NRing;
  bool ComputePrincipalDirectionVectors;
  bool ComputeGaussianCurvature;
  bool ComputeMeanCurvature;
  bool UseNormalsForCurveFitting;
  DataPath FaceAttributeMatrixPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFeatureFaceIdsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath SurfaceMeshTriangleCentroidsArrayPath;
  DataPath SurfaceMeshPrincipalCurvature1sArrayName;
  DataPath SurfaceMeshPrincipalCurvature2sArrayName;
  DataPath SurfaceMeshPrincipalDirection1sArrayName;
  DataPath SurfaceMeshPrincipalDirection2sArrayName;
  DataPath SurfaceMeshGaussianCurvaturesArrayName;
  DataPath SurfaceMeshMeanCurvaturesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT FeatureFaceCurvatureFilter
{
public:
  FeatureFaceCurvatureFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FeatureFaceCurvatureFilterInputValues* inputValues);
  ~FeatureFaceCurvatureFilter() noexcept;

  FeatureFaceCurvatureFilter(const FeatureFaceCurvatureFilter&) = delete;
  FeatureFaceCurvatureFilter(FeatureFaceCurvatureFilter&&) noexcept = delete;
  FeatureFaceCurvatureFilter& operator=(const FeatureFaceCurvatureFilter&) = delete;
  FeatureFaceCurvatureFilter& operator=(FeatureFaceCurvatureFilter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FeatureFaceCurvatureFilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
