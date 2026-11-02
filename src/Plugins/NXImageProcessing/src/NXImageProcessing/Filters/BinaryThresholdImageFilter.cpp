#include "BinaryThresholdImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace
{
struct BinaryThresholdOperation
{
  float64 lowerThreshold = 0.0;
  float64 upperThreshold = 255.0;
  uint8 insideValue = 1;
  uint8 outsideValue = 0;

  template <class T, class U>
  auto makeMapOp() const
  {
    const float64 lo = lowerThreshold, hi = upperThreshold;
    const uint8 inV = insideValue, outV = outsideValue;
    return [lo, hi, inV, outV](T value) -> U {
      const double d = static_cast<double>(value);
      return static_cast<U>((d >= lo && d <= hi) ? inV : outV);
    };
  }
};
} // namespace

namespace nx::core
{
std::string BinaryThresholdImageFilter::name() const
{
  return FilterTraits<BinaryThresholdImageFilter>::name;
}
std::string BinaryThresholdImageFilter::className() const
{
  return FilterTraits<BinaryThresholdImageFilter>::className;
}
Uuid BinaryThresholdImageFilter::uuid() const
{
  return FilterTraits<BinaryThresholdImageFilter>::uuid;
}
std::string BinaryThresholdImageFilter::humanName() const
{
  return "Binary Threshold Image Filter";
}
std::vector<std::string> BinaryThresholdImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "BinaryThreshold", "Pointwise"};
}
Parameters BinaryThresholdImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_LowerThreshold_Key, "Lower Threshold", "The lower threshold that a pixel value could be and still be considered 'Inside Value'", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_UpperThreshold_Key, "Upper Threshold",
                                                   "Set the thresholds. The default lower threshold is NumericTraits<InputPixelType>::NonpositiveMin() . The default upper threshold is "
                                                   "NumericTraits<InputPixelType>::max . An exception is thrown if the lower threshold is greater than the upper threshold.",
                                                   255.0));
  params.insert(std::make_unique<UInt8Parameter>(k_InsideValue_Key, "Inside Value", "Set the 'inside' pixel value. The default value NumericTraits<OutputPixelType>::max()", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_OutsideValue_Key, "Outside Value", "Set the 'outside' pixel value. The default value NumericTraits<OutputPixelType>::ZeroValue() .", 0u));

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
IFilter::VersionType BinaryThresholdImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer BinaryThresholdImageFilter::clone() const
{
  return std::make_unique<BinaryThresholdImageFilter>();
}
IFilter::PreflightResult BinaryThresholdImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // Match ITK's BinaryThresholdImageFilter (and this filter's documented contract): reject Lower > Upper.
  const auto lowerThreshold = filterArgs.value<float64>(k_LowerThreshold_Key);
  const auto upperThreshold = filterArgs.value<float64>(k_UpperThreshold_Key);
  if(lowerThreshold > upperThreshold)
  {
    return {MakeErrorResult<OutputActions>(-8330, fmt::format("Lower Threshold ({}) cannot be greater than Upper Threshold ({}).", lowerThreshold, upperThreshold))};
  }

  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysUInt8>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> BinaryThresholdImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto lowerThreshold = filterArgs.value<float64>(k_LowerThreshold_Key);
  auto upperThreshold = filterArgs.value<float64>(k_UpperThreshold_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);

  const BinaryThresholdOperation operation{lowerThreshold, upperThreshold, insideValue, outsideValue};
  return ImageProcessing::ExecuteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysUInt8>(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_LowerThresholdKey = "LowerThreshold";
constexpr StringLiteral k_UpperThresholdKey = "UpperThreshold";
constexpr StringLiteral k_InsideValueKey = "InsideValue";
constexpr StringLiteral k_OutsideValueKey = "OutsideValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> BinaryThresholdImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = BinaryThresholdImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_LowerThresholdKey, k_LowerThreshold_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_UpperThresholdKey, k_UpperThreshold_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint8>>(args, json, SIMPL::k_InsideValueKey, k_InsideValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint8>>(args, json, SIMPL::k_OutsideValueKey, k_OutsideValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
