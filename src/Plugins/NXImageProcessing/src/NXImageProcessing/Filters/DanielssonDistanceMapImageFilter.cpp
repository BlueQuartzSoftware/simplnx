#include "DanielssonDistanceMapImageFilter.hpp"

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
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
std::string DanielssonDistanceMapImageFilter::name() const
{
  return FilterTraits<DanielssonDistanceMapImageFilter>::name;
}
std::string DanielssonDistanceMapImageFilter::className() const
{
  return FilterTraits<DanielssonDistanceMapImageFilter>::className;
}
Uuid DanielssonDistanceMapImageFilter::uuid() const
{
  return FilterTraits<DanielssonDistanceMapImageFilter>::uuid;
}
std::string DanielssonDistanceMapImageFilter::humanName() const
{
  return "Danielsson Distance Map Image Filter";
}
std::vector<std::string> DanielssonDistanceMapImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "DanielssonDistanceMap", "DistanceMap", "MathematicalMorphology", "Morphology"};
}
Parameters DanielssonDistanceMapImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_InputIsBinary_Key, "Input Is Binary",
                                                "Whether the input is treated as a binary image for the (unemitted) Voronoi label map. This has no effect on the distance output and is provided only "
                                                "for parity with the legacy filter. Default is Off.",
                                                false));
  params.insert(std::make_unique<BoolParameter>(k_SquaredDistance_Key, "Squared Distance",
                                                "Whether the output stores the squared Euclidean distance (On) or the true Euclidean distance (Off). Default is Off.", false));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "Use Image Spacing",
                                                "Whether distances are measured in physical units using the Image Geometry's per-axis spacing (On) or in voxel units (Off). Default is Off.", false));

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
IFilter::VersionType DanielssonDistanceMapImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer DanielssonDistanceMapImageFilter::clone() const
{
  return std::make_unique<DanielssonDistanceMapImageFilter>();
}
IFilter::PreflightResult DanielssonDistanceMapImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: the Danielsson distance transform is defined on scalar images. The distance output is a FIXED
  // float32 (matching the legacy ITK Danielsson FilterOutputType), so AlwaysFloat32 maps every integer input type to a
  // float32 output array. IntegerOnly restricts the input to the integer scalar types.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::IntegerOnly, ImageProcessing::AlwaysFloat32>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  return {std::move(resultOutputActions)};
}
Result<> DanielssonDistanceMapImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto inputIsBinary = filterArgs.value<bool>(k_InputIsBinary_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  return ImageProcessing::ExecuteDanielssonDistanceMapImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, inputIsBinary, squaredDistance, useImageSpacing, shouldCancel,
                                                                  messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputIsBinaryKey = "InputIsBinary";
constexpr StringLiteral k_SquaredDistanceKey = "SquaredDistance";
constexpr StringLiteral k_UseImageSpacingKey = "UseImageSpacing";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> DanielssonDistanceMapImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = DanielssonDistanceMapImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_InputIsBinaryKey, k_InputIsBinary_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_SquaredDistanceKey, k_SquaredDistance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UseImageSpacingKey, k_UseImageSpacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
