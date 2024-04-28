
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

  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const FeatureFaceCurvatureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  std::chrono::steady_clock::time_point m_InitialPoint = std::chrono::steady_clock::now();
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
  size_t m_LastProgressInt = 0;
};

} // namespace nx::core
