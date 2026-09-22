#include "MaskImageFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
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

namespace nx::core
{
std::string MaskImageFilter::name() const
{
  return FilterTraits<MaskImageFilter>::name;
}
std::string MaskImageFilter::className() const
{
  return FilterTraits<MaskImageFilter>::className;
}
Uuid MaskImageFilter::uuid() const
{
  return FilterTraits<MaskImageFilter>::uuid;
}
std::string MaskImageFilter::humanName() const
{
  return "Mask Image Filter";
}
std::vector<std::string> MaskImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Mask", "Pointwise"};
}
Parameters MaskImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "Outside Value", "The value assigned to output pixels where the mask is zero.", 0.0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskImageDataPath_Key, "Mask Cell Data",
                                                          "The mask image data; must be uint8, uint16, or uint32 and have the same number of tuples as the input.", DataPath{},
                                                          nx::core::ImageProcessing::GetIntegerScalarTypes()));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType MaskImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer MaskImageFilter::clone() const
{
  return std::make_unique<MaskImageFilter>();
}
IFilter::PreflightResult MaskImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                        const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto maskArrayPath = filterArgs.value<DataPath>(k_MaskImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const auto& maskArray = dataStructure.getDataRefAs<IDataArray>(maskArrayPath);
  const DataType maskType = maskArray.getDataType();
  if(maskType != DataType::uint8 && maskType != DataType::uint16 && maskType != DataType::uint32)
  {
    return {MakeErrorResult<OutputActions>(-8100, fmt::format("Mask array '{}' must be uint8, uint16, or uint32, but is '{}'.", maskArrayPath.toString(), DataTypeToString(maskType)))};
  }
  // The mask engine reads the mask as a single-component scalar (ApplyMask/ReadMaskKeepChunk index by tuple);
  // a multi-component mask would be mis-read, so require scalar here.
  if(maskArray.getNumberOfComponents() != 1)
  {
    return {
        MakeErrorResult<OutputActions>(-8105, fmt::format("Mask array '{}' must be single-component (scalar), but has {} components.", maskArrayPath.toString(), maskArray.getNumberOfComponents()))};
  }
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(maskArray.getNumberOfTuples() != inputArray.getNumberOfTuples())
  {
    return {
        MakeErrorResult<OutputActions>(-8101, fmt::format("Mask array tuple count ({}) does not match input array tuple count ({}).", maskArray.getNumberOfTuples(), inputArray.getNumberOfTuples()))};
  }

  // The Outside Value is a Float64 parameter cast to the input element type by the mask engine. That conversion
  // is undefined for a non-finite or out-of-range value on an integer type (and a fractional value truncates),
  // so validate it up front via the shared guard, mirroring the object-morphology / contour filters.
  const auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  Result<> outsideGuard = ImageProcessing::ValidateScalarValueForType(inputArray.getDataType(), outsideValue, "Outside Value", -8102, -8103, -8104);
  if(outsideGuard.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(outsideGuard), OutputActions{})};
  }

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }
  for(Warning& warning : outsideGuard.warnings())
  {
    resultOutputActions.warnings().push_back(std::move(warning));
  }
  return {std::move(resultOutputActions)};
}
Result<> MaskImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto maskArrayPath = filterArgs.value<DataPath>(k_MaskImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  return ImageProcessing::ExecuteMaskImageFilter(dataStructure, selectedInputArray, maskArrayPath, outputArrayPath, outsideValue, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutsideValueKey = "OutsideValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_MaskCellArrayPathKey = "MaskCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> MaskImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = MaskImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutsideValueKey, k_OutsideValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskCellArrayPathKey, k_MaskImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
