#include "LabelContourImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ContourEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace
{
// Error/warning codes for this filter. Verified free by grepping the whole src/Plugins/ImageProcessing tree for
// -84xx/-85xx codes: the -84xx blocks up to -8493 and the -8500 block (Binary Contour, -8500..-8503) are taken,
// so this filter uses the still-unused -8510 block.
constexpr int32 k_NonScalarInput = -8510;
constexpr int32 k_NonFiniteBackgroundValue = -8511;
constexpr int32 k_BackgroundValueOutOfRange = -8512;
constexpr int32 k_BackgroundValueTruncated = -8513;
} // namespace

namespace nx::core
{
std::string LabelContourImageFilter::name() const
{
  return FilterTraits<LabelContourImageFilter>::name;
}
std::string LabelContourImageFilter::className() const
{
  return FilterTraits<LabelContourImageFilter>::className;
}
Uuid LabelContourImageFilter::uuid() const
{
  return FilterTraits<LabelContourImageFilter>::uuid;
}
std::string LabelContourImageFilter::humanName() const
{
  return "Label Contour Image Filter";
}
std::vector<std::string> LabelContourImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "LabelContour", "Label", "Contour", "ImageLabel"};
}
Parameters LabelContourImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Whether the connected components are defined strictly by face connectivity (off) or by face+edge+vertex connectivity (on). Default is off. "
                                                "For objects that are 1 pixel wide, use Fully Connected on.",
                                                false));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value",
                                                   "The value that identifies the background. Background voxels are never on a contour, and interior voxels of a region are set to this value. For "
                                                   "an integer input image the value must be finite and within the image type's range.",
                                                   0.0));

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
IFilter::VersionType LabelContourImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer LabelContourImageFilter::clone() const
{
  return std::make_unique<LabelContourImageFilter>();
}
IFilter::PreflightResult LabelContourImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(inputArray.getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(k_NonScalarInput, fmt::format("Label Contour requires a single-component (scalar) input array, but '{}' has {} components.", selectedInputArray.toString(),
                                                                         inputArray.getNumberOfComponents()))};
  }

  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::IntegerOnly, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  // The Float64 background parameter is cast to the (integer) input element type by the contour façade. That
  // conversion is UNDEFINED for a non-finite or out-of-range value, so validate the parameter here (the array is
  // guaranteed present/scalar/integer by the successful checks above). There is no foreground for this filter.
  const DataType inputType = inputArray.getDataType();
  Result<> paramGuard = ImageProcessing::ValidateBackgroundInRange(inputType, backgroundValue, k_NonFiniteBackgroundValue, k_BackgroundValueOutOfRange, k_BackgroundValueTruncated);
  if(paramGuard.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(paramGuard), OutputActions{})};
  }
  for(Warning& warning : paramGuard.warnings())
  {
    resultOutputActions.warnings().push_back(std::move(warning));
  }
  return {std::move(resultOutputActions)};
}
Result<> LabelContourImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  // The factory casts the background to the (integer) element type inside the type-dispatched façade; preflight
  // has already guaranteed the cast is well-defined.
  const ImageProcessing::LabelContourPredicateFactory predicateFactory{backgroundValue};

  return ImageProcessing::ExecuteContourImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, fullyConnected, predicateFactory, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_BackgroundValueKey = "BackgroundValue";
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> LabelContourImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = LabelContourImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float64>>(args, json, SIMPL::k_BackgroundValueKey, k_BackgroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
