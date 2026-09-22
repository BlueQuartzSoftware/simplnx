#include "ApproximateSignedDistanceMapImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
std::string ApproximateSignedDistanceMapImageFilter::name() const
{
  return FilterTraits<ApproximateSignedDistanceMapImageFilter>::name;
}
std::string ApproximateSignedDistanceMapImageFilter::className() const
{
  return FilterTraits<ApproximateSignedDistanceMapImageFilter>::className;
}
Uuid ApproximateSignedDistanceMapImageFilter::uuid() const
{
  return FilterTraits<ApproximateSignedDistanceMapImageFilter>::uuid;
}
std::string ApproximateSignedDistanceMapImageFilter::humanName() const
{
  return "Approximate Signed Distance Map Image Filter";
}
std::vector<std::string> ApproximateSignedDistanceMapImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "ApproximateSignedDistanceMap", "DistanceMap", "Morphology"};
}
Parameters ApproximateSignedDistanceMapImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_InsideValue_Key, "Inside Value", "The input intensity value that marks the interior of objects in the mask. Default is 1.", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "Outside Value", "The input intensity value that marks non-objects in the mask. Default is 0.", 0.0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetIntegerScalarTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType ApproximateSignedDistanceMapImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer ApproximateSignedDistanceMapImageFilter::clone() const
{
  return std::make_unique<ApproximateSignedDistanceMapImageFilter>();
}
IFilter::PreflightResult ApproximateSignedDistanceMapImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: the distance transform is defined on scalar images. The distance output is a FIXED float32
  // (matching the legacy ITK FilterOutputType), so AlwaysFloat32 maps every input type to a float32 output array.
  // IntegerOnly restricts the input to the integer scalar types (this mask filter does not accept float input).
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::IntegerOnly, ImageProcessing::AlwaysFloat32>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  return {std::move(resultOutputActions)};
}
Result<> ApproximateSignedDistanceMapImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto insideValue = filterArgs.value<float64>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  return ImageProcessing::ExecuteApproximateSignedDistanceMapImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, insideValue, outsideValue, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InsideValueKey = "InsideValue";
constexpr StringLiteral k_OutsideValueKey = "OutsideValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ApproximateSignedDistanceMapImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ApproximateSignedDistanceMapImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_InsideValueKey, k_InsideValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutsideValueKey, k_OutsideValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
