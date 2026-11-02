#include "RegionalMaximaImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/RegionalExtremaEngine.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
// fg/bg range-guard error codes (the Float64 foreground/background are cast to the input element type; for an
// integer type the guard makes that narrowing well-defined). -8573..-8575 verified free.
constexpr int32 k_NonFiniteValue = -8573;
constexpr int32 k_ValueOutOfRange = -8574;
constexpr int32 k_ValueTruncated = -8575;
} // namespace

namespace nx::core
{
std::string RegionalMaximaImageFilter::name() const
{
  return FilterTraits<RegionalMaximaImageFilter>::name;
}
std::string RegionalMaximaImageFilter::className() const
{
  return FilterTraits<RegionalMaximaImageFilter>::className;
}
Uuid RegionalMaximaImageFilter::uuid() const
{
  return FilterTraits<RegionalMaximaImageFilter>::uuid;
}
std::string RegionalMaximaImageFilter::humanName() const
{
  return "Regional Maxima Image Filter";
}
std::vector<std::string> RegionalMaximaImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "RegionalMaxima", "MathematicalMorphology", "Morphology"};
}
Parameters RegionalMaximaImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "Foreground Value",
                                                   "The value written to pixels that are part of a regional maximum. For an integer input image the value "
                                                   "must be finite and within the type's range.",
                                                   1.0));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value",
                                                   "The value written to pixels that are not part of a regional maximum. For an integer input image the "
                                                   "value must be finite and within the type's range.",
                                                   0.0));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Whether the flat zones are connected strictly by face connectivity (Off) or by face+edge+vertex connectivity (On). Default is Off.", false));
  params.insert(
      std::make_unique<BoolParameter>(k_FlatIsMaxima_Key, "Flat Is Maxima", "Whether a completely flat input image is considered a regional maximum (all foreground) or not (all background).", true));

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
IFilter::VersionType RegionalMaximaImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer RegionalMaximaImageFilter::clone() const
{
  return std::make_unique<RegionalMaximaImageFilter>();
}
IFilter::PreflightResult RegionalMaximaImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: the regional-extrema engine is defined on scalar images. The binary label output is a
  // FIXED uint32 (matching the legacy ITK RegionalMaxima FilterOutputType), so AlwaysUInt32 maps every input type
  // to a uint32 output array.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysUInt32>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  // The Float64 foreground/background are written into the uint32 output; that narrowing is UNDEFINED for a
  // non-finite or out-of-uint32-range value, so validate them against the uint32 output type.
  Result<> fgGuard = ImageProcessing::ValidateScalarValueForType(DataType::uint32, foregroundValue, "Foreground Value", k_NonFiniteValue, k_ValueOutOfRange, k_ValueTruncated);
  if(fgGuard.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(fgGuard), OutputActions{})};
  }
  Result<> bgGuard = ImageProcessing::ValidateScalarValueForType(DataType::uint32, backgroundValue, "Background Value", k_NonFiniteValue, k_ValueOutOfRange, k_ValueTruncated);
  if(bgGuard.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(bgGuard), OutputActions{})};
  }
  for(Warning& warning : fgGuard.warnings())
  {
    resultOutputActions.warnings().push_back(std::move(warning));
  }
  for(Warning& warning : bgGuard.warnings())
  {
    resultOutputActions.warnings().push_back(std::move(warning));
  }
  return {std::move(resultOutputActions)};
}
Result<> RegionalMaximaImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto flatIsMaxima = filterArgs.value<bool>(k_FlatIsMaxima_Key);

  return ImageProcessing::ExecuteRegionalExtremaImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, ImageProcessing::RegionalExtremaOp::Maxima, fullyConnected,
                                                            foregroundValue, backgroundValue, flatIsMaxima, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_ForegroundValueKey = "ForegroundValue";
constexpr StringLiteral k_BackgroundValueKey = "BackgroundValue";
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_FlatIsMaximaKey = "FlatIsMaxima";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> RegionalMaximaImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RegionalMaximaImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_ForegroundValueKey, k_ForegroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_BackgroundValueKey, k_BackgroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FlatIsMaximaKey, k_FlatIsMaxima_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
