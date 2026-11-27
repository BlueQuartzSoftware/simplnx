#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeGBCDMetricBasedInputValues
{
  int32 phase_of_interest{};
  VectorFloat32ParameterType::ValueType misorientation_rotation;
  ChoicesParameter::ValueType chosen_limit_dists{};
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
 * @class ComputeGBCDMetricBased
 * @brief This filter computes a section through the five-dimensional grain boundary distribution for a fixed mis orientation.
 */

class ORIENTATIONANALYSIS_EXPORT ComputeGBCDMetricBased
{
public:
  ComputeGBCDMetricBased(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBCDMetricBasedInputValues* inputValues);
  ~ComputeGBCDMetricBased() noexcept = default;

  ComputeGBCDMetricBased(const ComputeGBCDMetricBased&) = delete;
  ComputeGBCDMetricBased(ComputeGBCDMetricBased&&) noexcept = delete;
  ComputeGBCDMetricBased& operator=(const ComputeGBCDMetricBased&) = delete;
  ComputeGBCDMetricBased& operator=(ComputeGBCDMetricBased&&) noexcept = delete;

  /**
   * @brief Computes the metric-based GBCD section.
   * @return An error for an invalid Laue index, file operation, or bulk I/O.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  static constexpr int k_NumberResolutionChoices = 7;
  static constexpr float k_ResolutionChoices[k_NumberResolutionChoices][2] = {{3.0f, 7.0f}, {5.0f, 5.0f}, {5.0f, 7.0f}, {5.0f, 8.0f},
                                                                              {6.0f, 7.0f}, {7.0f, 7.0f}, {8.0f, 8.0f}}; // { for misorient., for planes }

private:
  DataStructure& m_DataStructure;
  const ComputeGBCDMetricBasedInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
