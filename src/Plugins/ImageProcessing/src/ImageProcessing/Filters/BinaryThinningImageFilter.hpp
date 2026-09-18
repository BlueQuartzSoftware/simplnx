#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class BinaryThinningImageFilter
 * @brief ITK-free, out-of-core-capable binary thinning (skeletonization) of an integer image.
 *
 * Thins the foreground (any non-zero pixel) of a binary image down to a 1-pixel-wide skeleton using ITK's
 * per-slice 2D sequential (Gonzalez-Woods) thinning algorithm, transcribed from itkBinaryThinningImageFilter.hxx.
 * The output is binary (values 0/1) and has the SAME type as the input. This filter is parameterless (thinning
 * has no tunable options). A 3D image is thinned per-z-slice independently -- ITK's neighbor offsets are 2D, so
 * there is no coupling between z-slices. The input may be ANY integer scalar type. Matches the legacy ITK Binary
 * Thinning Image Filter exactly (byte-identical output values).
 */
class IMAGEPROCESSING_EXPORT BinaryThinningImageFilter : public IFilter
{
public:
  BinaryThinningImageFilter() = default;
  ~BinaryThinningImageFilter() noexcept override = default;

  BinaryThinningImageFilter(const BinaryThinningImageFilter&) = delete;
  BinaryThinningImageFilter(BinaryThinningImageFilter&&) noexcept = delete;
  BinaryThinningImageFilter& operator=(const BinaryThinningImageFilter&) = delete;
  BinaryThinningImageFilter& operator=(BinaryThinningImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, BinaryThinningImageFilter, "863b660a-56dd-4edf-b6bc-d5af5c19c87c");
