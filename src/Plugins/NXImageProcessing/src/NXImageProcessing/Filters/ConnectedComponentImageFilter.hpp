#pragma once

#include "NXImageProcessing/NXImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ConnectedComponentImageFilter
 * @brief ITK-free, out-of-core-capable connected-component labeling via a streaming scanline union-find.
 *
 * Labels the objects in a binary image: non-zero input voxels are foreground, zero-valued voxels are background.
 * When all provisional IDs fit in uint32, objects receive consecutive labels from 1 in raster order of first appearance.
 * Background voxels receive 0. The input may be ANY integer scalar type; the output is a
 * FIXED uint32 label image regardless of the input element type. Matches the legacy ITK Connected Component Image
 * Filter while all provisional IDs fit in uint32. Error (default) stops on the first ID above that limit. Explicit legacy continuation
 * retains wrap to zero and reused IDs, so segmentation and downstream counts/statistics can be incorrect.
 */
class NXIMAGEPROCESSING_EXPORT ConnectedComponentImageFilter : public IFilter
{
public:
  ConnectedComponentImageFilter() = default;
  ~ConnectedComponentImageFilter() noexcept override = default;

  ConnectedComponentImageFilter(const ConnectedComponentImageFilter&) = delete;
  ConnectedComponentImageFilter(ConnectedComponentImageFilter&&) noexcept = delete;
  ConnectedComponentImageFilter& operator=(const ConnectedComponentImageFilter&) = delete;
  ConnectedComponentImageFilter& operator=(ConnectedComponentImageFilter&&) noexcept = delete;

  // Parameter Keys (reuse the legacy ITK filter's key strings so a shared param-setter drives both).
  static constexpr StringLiteralType k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteralType k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteralType k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteralType k_FullyConnected_Key = "fully_connected";
  // NOLINTNEXTLINE(readability-identifier-naming) -- Use the same k_Name_Key spelling as the other filter argument keys.
  static constexpr StringLiteralType k_ProvisionalLabelLimitHandling_Key = "provisional_label_limit_handling_index";

  std::string name() const override;
  std::string className() const override;
  Uuid uuid() const override;
  std::string humanName() const override;
  std::vector<std::string> defaultTags() const override;
  Parameters parameters() const override;
  VersionType parametersVersion() const override;
  UniquePointerType clone() const override;

protected:
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ConnectedComponentImageFilter, "288f782b-9b8f-4734-ae66-15ae4a43db41");
