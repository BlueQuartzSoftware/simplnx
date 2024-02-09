
#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FeatureFaceCurvatureInputValues
{
  int64 NRingCount = 0;
  DataPath triangleGeomPath;
  DataPath surfaceMeshPrincipalCurvature1Path;
  DataPath surfaceMeshFeatureFaceIdsPath;
  DataPath surfaceMeshFaceLabelsPath;
  DataPath surfaceMeshFaceNormalsPath;
  DataPath surfaceMeshTriangleCentroidsPath;

  bool useNormalsForCurveFitting;
  DataPath surfaceMeshPrincipalCurvature1sPath;
  DataPath surfaceMeshPrincipalCurvature2sPath;
  DataPath surfaceMeshPrincipalDirection1sPath;
  DataPath surfaceMeshPrincipalDirection2sPath;
  DataPath surfaceMeshMeanCurvaturesPath;
  DataPath surfaceMeshGaussianCurvaturesPath;
  DataPath surfaceMeshWeingartenMatrixPath;
};

/**
 * @class FillBadData
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */
class SIMPLNXCORE_EXPORT FeatureFaceCurvature
{
public:
  FeatureFaceCurvature(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FeatureFaceCurvatureInputValues* inputValues);
  ~FeatureFaceCurvature() noexcept;

  FeatureFaceCurvature(const FeatureFaceCurvature&) = delete;
  FeatureFaceCurvature(FeatureFaceCurvature&&) noexcept = delete;
  FeatureFaceCurvature& operator=(const FeatureFaceCurvature&) = delete;
  FeatureFaceCurvature& operator=(FeatureFaceCurvature&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FeatureFaceCurvatureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
