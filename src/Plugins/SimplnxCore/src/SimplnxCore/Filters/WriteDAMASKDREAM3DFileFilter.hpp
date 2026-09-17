#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WriteDAMASKDREAM3DFileFilter
 * @brief Writes a minimal DREAM3D file for DAMASK's DREAM3D importer.
 */
class SIMPLNXCORE_EXPORT WriteDAMASKDREAM3DFileFilter : public IFilter
{
public:
  WriteDAMASKDREAM3DFileFilter() = default;
  ~WriteDAMASKDREAM3DFileFilter() noexcept override = default;

  WriteDAMASKDREAM3DFileFilter(const WriteDAMASKDREAM3DFileFilter&) = delete;
  WriteDAMASKDREAM3DFileFilter(WriteDAMASKDREAM3DFileFilter&&) noexcept = delete;
  WriteDAMASKDREAM3DFileFilter& operator=(const WriteDAMASKDREAM3DFileFilter&) = delete;
  WriteDAMASKDREAM3DFileFilter& operator=(WriteDAMASKDREAM3DFileFilter&&) noexcept = delete;

  static constexpr StringLiteral k_OutputFile_Key = "output_file";
  static constexpr StringLiteral k_UseCompression_Key = "use_compression";
  static constexpr StringLiteral k_CompressionLevel_Key = "compression_level";
  static constexpr StringLiteral k_Representation_Key = "representation_index";
  static constexpr StringLiteral k_WritePhaseNames_Key = "write_phase_names";
  static constexpr StringLiteral k_ScaleToMeters_Key = "scale_to_meters";
  static constexpr StringLiteral k_ImageGeometryPath_Key = "image_geometry_path";
  static constexpr StringLiteral k_CellEulerAnglesArrayPath_Key = "cell_euler_angles_array_path";
  static constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";
  static constexpr StringLiteral k_FeatureEulerAnglesArrayPath_Key = "feature_euler_angles_array_path";
  static constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static constexpr StringLiteral k_PhaseNamesArrayPath_Key = "phase_names_array_path";

  std::string name() const override;
  std::string className() const override;
  Uuid uuid() const override;
  std::string humanName() const override;
  std::vector<std::string> defaultTags() const override;
  Parameters parameters() const override;
  VersionType parametersVersion() const override;
  UniquePointer clone() const override;

protected:
  /**
   * @brief Validates the selected representation and canonical export inputs.
   * @param dataStructure Contains the selected geometry and arrays.
   * @param filterArgs Contains the parameter values.
   * @param messageHandler Receives preflight messages.
   * @param shouldCancel Indicates that preflight must stop.
   * @param executionContext Resolves relative paths.
   * @return Empty output actions or validation errors.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Writes the selected data to a canonical DREAM3D file.
   * @param dataStructure Contains the selected geometry and arrays.
   * @param filterArgs Contains the parameter values.
   * @param pipelineNode Identifies the executing pipeline node.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Indicates that execution must stop.
   * @param executionContext Resolves relative paths.
   * @return Success, cancellation, or an export error.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteDAMASKDREAM3DFileFilter, "af751ada-0e38-405f-8109-a28d5da5b786");
