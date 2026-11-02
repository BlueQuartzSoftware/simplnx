#include "IsoContourDistanceImageFilter.hpp"

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
std::string IsoContourDistanceImageFilter::name() const
{
  return FilterTraits<IsoContourDistanceImageFilter>::name;
}
std::string IsoContourDistanceImageFilter::className() const
{
  return FilterTraits<IsoContourDistanceImageFilter>::className;
}
Uuid IsoContourDistanceImageFilter::uuid() const
{
  return FilterTraits<IsoContourDistanceImageFilter>::uuid;
}
std::string IsoContourDistanceImageFilter::humanName() const
{
  return "Iso Contour Distance Image Filter";
}
std::vector<std::string> IsoContourDistanceImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "IsoContourDistance", "DistanceMap", "LevelSet", "Morphology"};
}
Parameters IsoContourDistanceImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_LevelSetValue_Key, "Level Set Value", "The value of the level set (iso-contour) whose distance is computed. Default is 0.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_FarValue_Key, "Far Value",
                                                   "The signed value written to voxels not adjacent to the iso-contour: +Far Value above the level set, -Far Value below it. Default is 10.", 10.0));

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
IFilter::VersionType IsoContourDistanceImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer IsoContourDistanceImageFilter::clone() const
{
  return std::make_unique<IsoContourDistanceImageFilter>();
}
IFilter::PreflightResult IsoContourDistanceImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: IsoContourDistance is defined on scalar images. The distance output is a FIXED float32
  // (matching the legacy ITK FilterOutputType), so AlwaysFloat32 maps every input type to a float32 output array.
  // AllNumeric admits ALL scalar types INCLUDING floating point (the legacy ScalarPixelIdTypeList).
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat32>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  return {std::move(resultOutputActions)};
}
Result<> IsoContourDistanceImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto levelSetValue = filterArgs.value<float64>(k_LevelSetValue_Key);
  auto farValue = filterArgs.value<float64>(k_FarValue_Key);

  return ImageProcessing::ExecuteIsoContourDistanceImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, levelSetValue, farValue, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_LevelSetValueKey = "LevelSetValue";
constexpr StringLiteral k_FarValueKey = "FarValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> IsoContourDistanceImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = IsoContourDistanceImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_LevelSetValueKey, k_LevelSetValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_FarValueKey, k_FarValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
