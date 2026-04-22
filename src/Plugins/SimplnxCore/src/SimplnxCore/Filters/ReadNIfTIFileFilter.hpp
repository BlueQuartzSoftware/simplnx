#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ReadNIfTIFileFilter
 * @brief Reads a single-file NIfTI-1 (.nii or .nii.gz) volume into an ImageGeom + DataArray.
 *
 * Supports 3D NIfTI-1 volumes only. Recognized voxel datatypes: uint8/int8,
 * uint16/int16, uint32/int32, uint64/int64, float32, float64, RGB24, RGBA32.
 * When the input file specifies a non-identity scaling transform
 * (scl_slope != 0 and (slope != 1 or inter != 0)) and the user elects to
 * apply it, the output array is promoted to float32.
 */
class SIMPLNXCORE_EXPORT ReadNIfTIFileFilter : public IFilter
{
public:
  ReadNIfTIFileFilter();
  ~ReadNIfTIFileFilter() noexcept override;

  ReadNIfTIFileFilter(const ReadNIfTIFileFilter&) = delete;
  ReadNIfTIFileFilter(ReadNIfTIFileFilter&&) noexcept = delete;

  ReadNIfTIFileFilter& operator=(const ReadNIfTIFileFilter&) = delete;
  ReadNIfTIFileFilter& operator=(ReadNIfTIFileFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFilePath_Key = "input_file_path";
  static inline constexpr StringLiteral k_UseAffineIfPresent_Key = "use_affine_if_present";
  static inline constexpr StringLiteral k_ApplyScalingTransform_Key = "apply_scaling_transform";
  static inline constexpr StringLiteral k_CroppingOptions_Key = "cropping_options_index";
  static inline constexpr StringLiteral k_CreatedImageGeometryPath_Key = "output_image_geometry_path";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static inline constexpr StringLiteral k_ImageDataArrayName_Key = "image_data_array_name";

  /**
   * @brief Reads SIMPL json and converts it to simplnx Arguments.
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  std::string name() const override;
  std::string className() const override;
  Uuid uuid() const override;
  std::string humanName() const override;
  std::vector<std::string> defaultTags() const override;
  Parameters parameters() const override;
  VersionType parametersVersion() const override;
  UniquePointer clone() const override;

protected:
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;

private:
  int32 m_InstanceId;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadNIfTIFileFilter, "e69e51e5-2a9f-40ca-8038-1d2be7925c62");
