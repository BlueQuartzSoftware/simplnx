#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ConnectedComponentImageFilter
 * @brief ITK-free, out-of-core-capable connected-component labeling via a streaming scanline union-find.
 *
 * Labels the objects in a binary image: non-zero input voxels are foreground, zero-valued voxels are background.
 * Each distinct foreground object is assigned a unique consecutive label (1, 2, 3, ...) in raster-scan order of
 * first appearance; background voxels are labeled 0. The input may be ANY integer scalar type; the output is a
 * FIXED uint32 label image regardless of the input element type. Matches the legacy ITK Connected Component Image
 * Filter exactly (byte-identical label values). Out-of-core capable.
 */
class IMAGEPROCESSING_EXPORT ConnectedComponentImageFilter : public IFilter
{
public:
  ConnectedComponentImageFilter() = default;
  ~ConnectedComponentImageFilter() noexcept override = default;

  ConnectedComponentImageFilter(const ConnectedComponentImageFilter&) = delete;
  ConnectedComponentImageFilter(ConnectedComponentImageFilter&&) noexcept = delete;
  ConnectedComponentImageFilter& operator=(const ConnectedComponentImageFilter&) = delete;
  ConnectedComponentImageFilter& operator=(ConnectedComponentImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_FullyConnected_Key = "fully_connected";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ConnectedComponentImageFilter, "288f782b-9b8f-4734-ae66-15ae4a43db41");
