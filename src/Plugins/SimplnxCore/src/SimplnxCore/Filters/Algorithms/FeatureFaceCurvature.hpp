
#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <mutex>

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

  /**
   * @brief Thread-safe progress update. Safe to call from the per-feature-face workers.
   * @param counter Items completed since the previous call
   */
  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const FeatureFaceCurvatureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessage_Mutex;
  ThrottledMessageHandler m_Throttle;
};

} // namespace nx::core
