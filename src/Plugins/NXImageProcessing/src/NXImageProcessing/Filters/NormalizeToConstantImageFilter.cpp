#include "NormalizeToConstantImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

using namespace nx::core;

namespace
{
struct NormalizeToConstantOperation
{
  float64 constant = 1.0;
  template <class T>
  Result<ImageProcessing::ArrayStatistics<T>> computeStatistics(const AbstractDataStore<T>& inputStore, const std::atomic_bool& shouldCancel) const
  {
    auto sumResult = ImageProcessing::ComputeArraySum(inputStore, shouldCancel);
    if(sumResult.invalid())
    {
      return {nonstd::make_unexpected(std::move(sumResult.errors()))};
    }
    ImageProcessing::ArrayStatistics<T> stats;
    stats.count = inputStore.getSize();
    stats.sum = sumResult.value();
    return {stats};
  }
  template <class T>
  Result<> validate(const ImageProcessing::ArrayStatistics<T>& stats) const
  {
    if(stats.sum == 0.0)
    {
      return MakeErrorResult(-8200, "NormalizeToConstant: the sum of all input values is zero; cannot normalize (division by zero).");
    }
    return {};
  }
  template <class T, class U>
  auto makeMapOp(const ImageProcessing::ArrayStatistics<T>& stats) const
  {
    const double factor = constant / stats.sum; // out = x * (constant / sum)
    return [factor](T value) -> U { return static_cast<U>(static_cast<double>(value) * factor); };
  }
};
} // namespace

namespace nx::core
{
std::string NormalizeToConstantImageFilter::name() const
{
  return FilterTraits<NormalizeToConstantImageFilter>::name;
}
std::string NormalizeToConstantImageFilter::className() const
{
  return FilterTraits<NormalizeToConstantImageFilter>::className;
}
Uuid NormalizeToConstantImageFilter::uuid() const
{
  return FilterTraits<NormalizeToConstantImageFilter>::uuid;
}
std::string NormalizeToConstantImageFilter::humanName() const
{
  return "Normalize To Constant Image Filter";
}
std::vector<std::string> NormalizeToConstantImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "NormalizeToConstant", "Pointwise"};
}
Parameters NormalizeToConstantImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Constant_Key, "Constant", "Set/get the normalization constant.", 1.0));

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
IFilter::VersionType NormalizeToConstantImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer NormalizeToConstantImageFilter::clone() const
{
  return std::make_unique<NormalizeToConstantImageFilter>();
}
IFilter::PreflightResult NormalizeToConstantImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
Result<> NormalizeToConstantImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto constant = filterArgs.value<float64>(k_Constant_Key);

  const NormalizeToConstantOperation operation{constant};
  return ImageProcessing::ExecuteTwoPassImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat64>(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel,
                                                                                                                 messageHandler);
}
} // namespace nx::core
