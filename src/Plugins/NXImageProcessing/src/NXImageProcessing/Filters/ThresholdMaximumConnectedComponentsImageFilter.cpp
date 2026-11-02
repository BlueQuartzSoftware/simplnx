#include "ThresholdMaximumConnectedComponentsImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

using namespace nx::core;

namespace nx::core
{
std::string ThresholdMaximumConnectedComponentsImageFilter::name() const
{
  return FilterTraits<ThresholdMaximumConnectedComponentsImageFilter>::name;
}
std::string ThresholdMaximumConnectedComponentsImageFilter::className() const
{
  return FilterTraits<ThresholdMaximumConnectedComponentsImageFilter>::className;
}
Uuid ThresholdMaximumConnectedComponentsImageFilter::uuid() const
{
  return FilterTraits<ThresholdMaximumConnectedComponentsImageFilter>::uuid;
}
std::string ThresholdMaximumConnectedComponentsImageFilter::humanName() const
{
  return "Threshold Maximum Connected Components Image Filter";
}
std::vector<std::string> ThresholdMaximumConnectedComponentsImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Segmentation", "Threshold"};
}

Parameters ThresholdMaximumConnectedComponentsImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt32Parameter>(
      k_MinimumObjectSizeInPixels_Key, "Minimum Object Size In Pixels",
      "The pixel type must support comparison operators. Set the minimum pixel area used to count objects on the image. Thus, only objects that have a pixel area greater than the minimum pixel area "
      "will be counted as an object in the optimization portion of this filter. Essentially, it eliminates noise from being counted as an object. The default value is zero.",
      0u));
  params.insert(std::make_unique<Float64Parameter>(
      k_UpperBoundary_Key, "Upper Boundary",
      "The following Set/Get methods are for the binary threshold function. This class automatically calculates the lower threshold boundary. The upper threshold boundary, inside value, and outside "
      "value can be defined by the user, however the standard values are used as default if not set by the user. The default value of the: Inside value is the maximum pixel type intensity. Outside "
      "value is the minimum pixel type intensity. Upper threshold boundary is the maximum pixel type intensity.",
      65536.0));
  params.insert(std::make_unique<UInt8Parameter>(
      k_InsideValue_Key, "Inside Value",
      "The following Set/Get methods are for the binary threshold function. This class automatically calculates the lower threshold boundary. The upper threshold boundary, inside value, and outside "
      "value can be defined by the user, however the standard values are used as default if not set by the user. The default value of the: Inside value is the maximum pixel type intensity. Outside "
      "value is the minimum pixel type intensity. Upper threshold boundary is the maximum pixel type intensity.",
      1u));
  params.insert(std::make_unique<UInt8Parameter>(
      k_OutsideValue_Key, "Outside Value",
      "The following Set/Get methods are for the binary threshold function. This class automatically calculates the lower threshold boundary. The upper threshold boundary, inside value, and outside "
      "value can be defined by the user, however the standard values are used as default if not set by the user. The default value of the: Inside value is the maximum pixel type intensity. Outside "
      "value is the minimum pixel type intensity. Upper threshold boundary is the maximum pixel type intensity.",
      0u));

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

IFilter::VersionType ThresholdMaximumConnectedComponentsImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer ThresholdMaximumConnectedComponentsImageFilter::clone() const
{
  return std::make_unique<ThresholdMaximumConnectedComponentsImageFilter>();
}

IFilter::PreflightResult ThresholdMaximumConnectedComponentsImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: the bisection search operates on a scalar image. The output is a FIXED uint8 binary image
  // (matching the legacy ITK FilterOutputType), so AlwaysUInt8 maps every input type to a uint8 output array.
  // AllNumeric admits every scalar numeric type (the legacy ScalarPixelIdTypeList), incl. float32/float64.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysUInt8>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  return {std::move(resultOutputActions)};
}

Result<> ThresholdMaximumConnectedComponentsImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode,
                                                                     const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto minimumObjectSizeInPixels = filterArgs.value<uint32>(k_MinimumObjectSizeInPixels_Key);
  auto upperBoundary = filterArgs.value<float64>(k_UpperBoundary_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);

  return ImageProcessing::ExecuteThresholdMaximumConnectedComponentsImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, minimumObjectSizeInPixels, upperBoundary,
                                                                                insideValue, outsideValue, shouldCancel, messageHandler);
}
} // namespace nx::core
