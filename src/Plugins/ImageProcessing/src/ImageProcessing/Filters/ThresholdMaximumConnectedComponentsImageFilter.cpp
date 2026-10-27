#include "ThresholdMaximumConnectedComponentsImageFilter.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

#include <cmath>
#include <memory>
#include <utility>

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
      "Count face-connected nonzero output components with at least this many pixels or voxels. This changes the search score; it does not remove small objects from the output. Default: 0 pixels.",
      0u));
  params.insert(
      std::make_unique<Float64Parameter>(k_UpperBoundary_Key, "Upper Boundary",
                                         "Inclusive upper cutoff in input intensity units. Default: 65536. Integer inputs truncate toward zero. High values or positive infinity select the native "
                                         "maximum; low/empty bands or negative infinity produce Outside Value with a warning. NaN is an error. Float64 cannot express every 64-bit integer cutoff.",
                                         65536.0));
  params.insert(
      std::make_unique<UInt8Parameter>(k_InsideValue_Key, "Inside Value",
                                       "Dimensionless uint8 value assigned inside the inclusive threshold band. Default: 1. Only nonzero output pixels participate in the component-count score.", 1u));
  params.insert(std::make_unique<UInt8Parameter>(
      k_OutsideValue_Key, "Outside Value",
      "Dimensionless uint8 value assigned outside the band. Default: 0. A nonzero value participates in scoring and can connect regions, including nonfinite pixels in Warn mode.", 0u));
  params.insert(std::make_unique<ChoicesParameter>(k_NonfiniteInputHandling_Key, "Nonfinite Input Handling",
                                                   "Error stops on image NaN or either infinity before search/output. Warn and Continue uses finite bounds and assigns nonfinite pixels Outside Value. "
                                                   "With no finite pixels, it skips search, fills Outside Value and warns that no threshold was calculated.",
                                                   0, ChoicesParameter::Choices{"Error", "Warn and Continue"}));

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
  return 2;
}
IFilter::UniquePointer ThresholdMaximumConnectedComponentsImageFilter::clone() const
{
  return std::make_unique<ThresholdMaximumConnectedComponentsImageFilter>();
}

IFilter::PreflightResult ThresholdMaximumConnectedComponentsImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  if(std::isnan(filterArgs.value<float64>(k_UpperBoundary_Key)))
  {
    return {.outputActions = MakeErrorResult<OutputActions>(
                -28610, "Upper Boundary is NaN. Enter a valid numeric cutoff; positive infinity selects the native maximum and negative infinity requests an empty band.")};
  }
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
                                                                                insideValue, outsideValue, shouldCancel, messageHandler,
                                                                                filterArgs.value<ChoicesParameter::ValueType>(k_NonfiniteInputHandling_Key) == 1);
}
} // namespace nx::core
