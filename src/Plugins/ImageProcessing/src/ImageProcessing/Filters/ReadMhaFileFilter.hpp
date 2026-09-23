#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ReadMhaFileFilter
 * @brief Reads a MHA / MetaImage (attached .mha or detached .mhd + .raw/.zraw)
 *        into an ImageGeom + Cell DataArray. ITK-free (zlib only). Reproduces the
 *        ITKMhaFileReader transform feature: parse the header TransformMatrix
 *        into a 4x4, optionally transpose (pure-rotation determinant check),
 *        optionally save the 16 floats as a float32[16] array, and optionally
 *        apply it to the geometry via ApplyTransformationToGeometry. Param keys
 *        match ITKMhaFileReaderFilter so a UUID redirect is parameter-lossless.
 */
class IMAGEPROCESSING_EXPORT ReadMhaFileFilter : public IFilter
{
public:
  ReadMhaFileFilter() = default;
  ~ReadMhaFileFilter() noexcept override = default;

  ReadMhaFileFilter(const ReadMhaFileFilter&) = delete;
  ReadMhaFileFilter(ReadMhaFileFilter&&) noexcept = delete;
  ReadMhaFileFilter& operator=(const ReadMhaFileFilter&) = delete;
  ReadMhaFileFilter& operator=(ReadMhaFileFilter&&) noexcept = delete;

  // Parameter Keys. Core reader keys + transform keys mirror ITKMhaFileReaderFilter
  // (and ITKImageReaderFilter) verbatim so a retirement UUID redirect is lossless.
  static inline constexpr StringLiteral k_InputFilePath_Key = "file_name";
  static inline constexpr StringLiteral k_CreatedImageGeometryPath_Key = "output_geometry_path";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static inline constexpr StringLiteral k_ImageDataArrayName_Key = "image_data_array_name";
  static inline constexpr StringLiteral k_ApplyImageTransformation_Key = "apply_image_transformation";
  static inline constexpr StringLiteral k_InterpolationType_Key = "interpolation_type_index";
  static inline constexpr StringLiteral k_TransposeTransformMatrix_Key = "transpose_transform_matrix";
  static inline constexpr StringLiteral k_SaveImageTransformation_Key = "save_image_transformation";
  static inline constexpr StringLiteral k_TransformationMatrixPath_Key = "output_transformation_matrix_path";
  // New (our addition); harmless on redirect (defaults with a -5432 warning).
  static inline constexpr StringLiteral k_CroppingOptions_Key = "cropping_options_index";

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
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadMhaFileFilter, "a2da3e27-d5f9-437c-ad02-47d919e1b8a7");
