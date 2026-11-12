#include "NormalizeImageFilter.hpp"

#include "NXImageProcessing/Filters/Algorithms/NormalizeImage.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
constexpr int32 k_InvalidInputArrayError = -94002;

NormalizeImageInputValues CreateInputValues(const Arguments& filterArgs)
{
  const auto inputPath = filterArgs.value<DataPath>(NormalizeImageFilter::k_InputImageDataPath_Key);
  const auto outputName = filterArgs.value<DataObjectNameParameter::ValueType>(NormalizeImageFilter::k_OutputImageArrayName_Key);
  return {inputPath, inputPath.replaceName(outputName)};
}
} // namespace

namespace nx::core
{
std::string NormalizeImageFilter::name() const
{
  return FilterTraits<NormalizeImageFilter>::name;
}
std::string NormalizeImageFilter::className() const
{
  return FilterTraits<NormalizeImageFilter>::className;
}
Uuid NormalizeImageFilter::uuid() const
{
  return FilterTraits<NormalizeImageFilter>::uuid;
}
std::string NormalizeImageFilter::humanName() const
{
  return "Normalize Image Filter";
}
std::vector<std::string> NormalizeImageFilter::defaultTags() const
{
  return {className(), "NXImageProcessing", "Normalize", "Pointwise"};
}
Parameters NormalizeImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType NormalizeImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer NormalizeImageFilter::clone() const
{
  return std::make_unique<NormalizeImageFilter>();
}
IFilter::PreflightResult NormalizeImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  const auto inputValues = CreateInputValues(filterArgs);
  // ArraySelectionParameter checks type and component shape only for an IDataArray, so a StringArray passes it.
  if(dataStructure.getDataAs<IDataArray>(inputValues.InputArrayPath) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(k_InvalidInputArrayError,
                                           fmt::format("Input path '{}' is missing or is not a numeric data array. Select an existing numeric data array.", inputValues.InputArrayPath.toString()))};
  }

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat64>(
      dataStructure, inputValues.InputArrayPath, imageGeomPath, inputValues.OutputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> NormalizeImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const auto inputValues = CreateInputValues(filterArgs);
  return NormalizeImage(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> NormalizeImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = NormalizeImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
