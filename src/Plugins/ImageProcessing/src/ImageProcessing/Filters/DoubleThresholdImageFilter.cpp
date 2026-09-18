#include "DoubleThresholdImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

using namespace nx::core;

namespace
{
// The four thresholds share ONE help string (transcribed verbatim from the legacy ITKDoubleThresholdImageFilter),
// differing only in default value.
constexpr StringLiteral k_ThresholdHelp = "Set the thresholds. Four thresholds should be specified. The two lower thresholds default to NumericTraits<InputPixelType>::NonpositiveMin() . The "
                                          "two upper thresholds default NumericTraits<InputPixelType>::max . Threshold1 <= Threshold2 <= Threshold3 <= Threshold4.";
} // namespace

namespace nx::core
{
std::string DoubleThresholdImageFilter::name() const
{
  return FilterTraits<DoubleThresholdImageFilter>::name;
}
std::string DoubleThresholdImageFilter::className() const
{
  return FilterTraits<DoubleThresholdImageFilter>::className;
}
Uuid DoubleThresholdImageFilter::uuid() const
{
  return FilterTraits<DoubleThresholdImageFilter>::uuid;
}
std::string DoubleThresholdImageFilter::humanName() const
{
  return "Double Threshold Image Filter";
}
std::vector<std::string> DoubleThresholdImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "DoubleThreshold", "MathematicalMorphology", "Morphology"};
}

Parameters DoubleThresholdImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Threshold1_Key, "Threshold1", k_ThresholdHelp, 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold2_Key, "Threshold2", k_ThresholdHelp, 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold3_Key, "Threshold3", k_ThresholdHelp, 254.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold4_Key, "Threshold4", k_ThresholdHelp, 255.0));
  params.insert(std::make_unique<UInt8Parameter>(k_InsideValue_Key, "Inside Value", "Set the 'inside' pixel value. The default value NumericTraits<OutputPixelType>::max()", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_OutsideValue_Key, "Outside Value", "Set the 'outside' pixel value. The default value NumericTraits<OutputPixelType>::ZeroValue() .", 0u));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. "
                                                "For objects that are 1 pixel wide, use FullyConnectedOn.",
                                                false));

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

IFilter::VersionType DoubleThresholdImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer DoubleThresholdImageFilter::clone() const
{
  return std::make_unique<DoubleThresholdImageFilter>();
}

IFilter::PreflightResult DoubleThresholdImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: double thresholding is defined on scalar images. The output is a FIXED uint8 (matching the
  // legacy ITK FilterOutputType), so AlwaysUInt8 maps every input type to a uint8 output array. AllNumeric admits
  // all 10 scalar types (matching the legacy ScalarPixelIdTypeList). There is no minimum-dimension requirement.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysUInt8>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  return {std::move(resultOutputActions)};
}

Result<> DoubleThresholdImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto threshold1 = filterArgs.value<float64>(k_Threshold1_Key);
  auto threshold2 = filterArgs.value<float64>(k_Threshold2_Key);
  auto threshold3 = filterArgs.value<float64>(k_Threshold3_Key);
  auto threshold4 = filterArgs.value<float64>(k_Threshold4_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  return ImageProcessing::ExecuteDoubleThresholdImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, threshold1, threshold2, threshold3, threshold4, insideValue,
                                                            outsideValue, fullyConnected, shouldCancel, messageHandler);
}
} // namespace nx::core
