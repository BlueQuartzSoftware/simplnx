#include "BoundedReciprocalImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

using namespace nx::core;

namespace
{
struct BoundedReciprocalOperation
{
  template <class T, class U>
  auto makeMapOp() const
  {
    return [](T value) -> U { return static_cast<U>(1.0 / (1.0 + static_cast<double>(value))); };
  }
};
} // namespace

namespace nx::core
{
std::string BoundedReciprocalImageFilter::name() const
{
  return FilterTraits<BoundedReciprocalImageFilter>::name;
}
std::string BoundedReciprocalImageFilter::className() const
{
  return FilterTraits<BoundedReciprocalImageFilter>::className;
}
Uuid BoundedReciprocalImageFilter::uuid() const
{
  return FilterTraits<BoundedReciprocalImageFilter>::uuid;
}
std::string BoundedReciprocalImageFilter::humanName() const
{
  return "Bounded Reciprocal Image Filter";
}
std::vector<std::string> BoundedReciprocalImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "BoundedReciprocal", "Pointwise"};
}
Parameters BoundedReciprocalImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType BoundedReciprocalImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer BoundedReciprocalImageFilter::clone() const
{
  return std::make_unique<BoundedReciprocalImageFilter>();
}
IFilter::PreflightResult BoundedReciprocalImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat64>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> BoundedReciprocalImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const BoundedReciprocalOperation operation{};
  return ImageProcessing::ExecuteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat64>(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
}
} // namespace nx::core
