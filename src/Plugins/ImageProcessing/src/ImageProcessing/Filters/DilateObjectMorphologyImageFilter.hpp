#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class DilateObjectMorphologyImageFilter
 * @brief ITK-free, out-of-core-capable object dilation of an image.
 *
 * Object morphology operates on a single labeled object: a voxel equal to the Object Value is a boundary
 * voxel when at least one of its immediate neighbors (the fixed 26-connected / 8-connected radius-1 box) is
 * NOT the object value. Each boundary voxel paints the structuring element (Annulus/Ball/Box/Cross at the
 * chosen per-axis radius) centered on it with the Object Value, growing the object outward by one kernel
 * around its border. Boundary/object state is read from the input and painted into a separate output that
 * starts as a copy of the input, so the growth does not cascade within one pass.
 *
 * This reproduces the intended legacy ITK Dilate Object Morphology operation with three documented
 * differences: the Annulus kernel is a proper (non-empty) thickness-1 shell rather than the legacy empty
 * kernel, copy-then-paint execution is deterministic rather than subject to the legacy multithreaded race,
 * and the filter is out-of-core capable (the legacy filter rejected chunked arrays).
 */
class IMAGEPROCESSING_EXPORT DilateObjectMorphologyImageFilter : public IFilter
{
public:
  DilateObjectMorphologyImageFilter() = default;
  ~DilateObjectMorphologyImageFilter() noexcept override = default;

  DilateObjectMorphologyImageFilter(const DilateObjectMorphologyImageFilter&) = delete;
  DilateObjectMorphologyImageFilter(DilateObjectMorphologyImageFilter&&) noexcept = delete;
  DilateObjectMorphologyImageFilter& operator=(const DilateObjectMorphologyImageFilter&) = delete;
  DilateObjectMorphologyImageFilter& operator=(DilateObjectMorphologyImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputImageGeomPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_InputImageDataPath_Key = "input_image_data_path";
  static constexpr StringLiteral k_OutputImageArrayName_Key = "output_array_name";
  static constexpr StringLiteral k_KernelRadius_Key = "kernel_radius";
  static constexpr StringLiteral k_KernelType_Key = "kernel_type_index";
  static constexpr StringLiteral k_ObjectValue_Key = "object_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, DilateObjectMorphologyImageFilter, "4e00bb78-e3cc-41ee-b6f6-86e49fd34b4c");
