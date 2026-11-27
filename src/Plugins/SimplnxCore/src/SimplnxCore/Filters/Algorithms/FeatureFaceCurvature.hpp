
#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <mutex>
#include <string>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FeatureFaceCurvatureInputValues
{
  int64 n_ring_count = 0;
  DataPath triangle_geom_path;
  DataPath surface_mesh_principal_curvature1_path;
  DataPath surface_mesh_feature_face_ids_path;
  DataPath surface_mesh_face_labels_path;
  DataPath surface_mesh_face_normals_path;
  DataPath surface_mesh_triangle_centroids_path;

  bool use_normals_for_curve_fitting{};
  DataPath surface_mesh_principal_curvature1s_path;
  DataPath surface_mesh_principal_curvature2s_path;
  DataPath surface_mesh_principal_direction1s_path;
  DataPath surface_mesh_principal_direction2s_path;
  DataPath surface_mesh_mean_curvatures_path;
  DataPath surface_mesh_gaussian_curvatures_path;
  DataPath surface_mesh_weingarten_matrix_path;
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

  /**
   * @brief Sends an Info message under the worker progress mutex.
   * @param message Status text to send.
   *
   * The driver thread calls this while workers run, so it takes the same mutex as
   * sendThreadSafeProgressMessage().
   */
  void sendThreadSafeStatusMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const FeatureFaceCurvatureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessageMutex;
  ThrottledMessageHandler m_Throttle;
};

} // namespace nx::core
