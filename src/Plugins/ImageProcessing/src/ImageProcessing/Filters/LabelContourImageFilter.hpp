#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class LabelContourImageFilter
 * @brief ITK-free, out-of-core-capable extraction of the contours (borders) of the objects in a labeled image.
 *
 * Every value other than the Background Value is its own object/region. A background-valued voxel is never on a
 * contour (it always stays the Background Value, even next to a labeled region). A labeled voxel is kept (with
 * its OWN label) only when at least one of its connectivity neighbors has a different value (a different label
 * or the background); an interior voxel whose neighbors are all its own label becomes the Background Value.
 * Labels are preserved exactly on the contour, matching the legacy ITK Label Contour Image Filter. The
 * connectivity (Fully Connected off = face; on = face+edge+vertex) matches the legacy filter.
 */
class IMAGEPROCESSING_EXPORT LabelContourImageFilter : public IFilter
{
public:
  LabelContourImageFilter() = default;
  ~LabelContourImageFilter() noexcept override = default;

  LabelContourImageFilter(const LabelContourImageFilter&) = delete;
  LabelContourImageFilter(LabelContourImageFilter&&) noexcept = delete;
  LabelContourImageFilter& operator=(const LabelContourImageFilter&) = delete;
  LabelContourImageFilter& operator=(LabelContourImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_FullyConnected_Key = "fully_connected";
  static constexpr StringLiteral k_BackgroundValue_Key = "background_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, LabelContourImageFilter, "060339d1-56c9-4253-9b52-d4be6a58eafb");
