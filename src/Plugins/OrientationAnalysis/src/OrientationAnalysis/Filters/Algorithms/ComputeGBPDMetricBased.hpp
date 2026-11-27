#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeGBPDMetricBasedInputValues
{
  int32 phase_of_interest{};
  float64 limit_dist{};
  int32 num_sampl_pts{};
  bool exclude_triple_lines{};
  FileSystemPathParameter::ValueType dist_output_file;
  FileSystemPathParameter::ValueType err_output_file;
  bool save_relative_err{};
  DataPath node_types_array_path;
  DataPath surface_mesh_face_labels_array_path;
  DataPath surface_mesh_face_normals_array_path;
  DataPath surface_mesh_face_areas_array_path;
  DataPath surface_mesh_feature_face_labels_array_path;
  DataPath feature_euler_angles_array_path;
  DataPath feature_phases_array_path;
  DataPath crystal_structures_array_path;
  DataPath triangle_geometry_path;
};

/**
 * @class ComputeGBPDMetricBased
 * @brief This filter computes the grain boundary plane distribution (GBPD).
 */

class ORIENTATIONANALYSIS_EXPORT ComputeGBPDMetricBased
{
public:
  ComputeGBPDMetricBased(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBPDMetricBasedInputValues* inputValues);
  ~ComputeGBPDMetricBased() noexcept = default;

  ComputeGBPDMetricBased(const ComputeGBPDMetricBased&) = delete;
  ComputeGBPDMetricBased(ComputeGBPDMetricBased&&) noexcept = delete;
  ComputeGBPDMetricBased& operator=(const ComputeGBPDMetricBased&) = delete;
  ComputeGBPDMetricBased& operator=(ComputeGBPDMetricBased&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  static void AppendSamplePtsFixedZenith(std::vector<float64>& xVec, std::vector<float64>& yVec, std::vector<float64>& zVec, float64 theta, float64 minPhi, float64 maxPhi, float64 step);
  static void AppendSamplePtsFixedAzimuth(std::vector<float64>& xVec, std::vector<float64>& yVec, std::vector<float64>& zVec, float64 phi, float64 minTheta, float64 maxTheta, float64 step);

private:
  DataStructure& m_DataStructure;
  const ComputeGBPDMetricBasedInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
