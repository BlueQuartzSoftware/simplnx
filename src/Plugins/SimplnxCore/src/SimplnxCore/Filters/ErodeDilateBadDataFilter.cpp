#include "ErodeDilateBadDataFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ErodeDilateBadData.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_NoDirectionsError = -14601;
constexpr int32 k_NoGeometryDimensionsError = -14602;
} // namespace

namespace nx::core
{

std::string ErodeDilateBadDataFilter::name() const
{
  return FilterTraits<ErodeDilateBadDataFilter>::name.str();
}

std::string ErodeDilateBadDataFilter::className() const
{
  return FilterTraits<ErodeDilateBadDataFilter>::className;
}

Uuid ErodeDilateBadDataFilter::uuid() const
{
  return FilterTraits<ErodeDilateBadDataFilter>::uuid;
}

std::string ErodeDilateBadDataFilter::humanName() const
{
  return "Erode/Dilate Bad Data";
}

std::vector<std::string> ErodeDilateBadDataFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Erode", "Dilate"};
}

Parameters ErodeDilateBadDataFilter::parameters() const
{
  Parameters params;

  // Create the filter parameter descriptors.
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<ChoicesParameter>(k_Operation_Key, "Operation", "Whether to dilate or erode", 0ULL, detail::k_OperationChoices));
  params.insert(std::make_unique<Int32ParameterType>(k_NumIterations_Key, "Number of Iterations", "The number of iterations to use for erosion/dilation", 2));
  params.insert(std::make_unique<BoolParameter>(k_XDirOn_Key, "X Direction", "Whether to erode/dilate in the X direction", true));
  params.insert(std::make_unique<BoolParameter>(k_YDirOn_Key, "Y Direction", "Whether to erode/dilate in the Y direction", true));
  params.insert(std::make_unique<BoolParameter>(k_ZDirOn_Key, "Z Direction", "Whether to erode/dilate in the Z direction", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore when performing the algorithm",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypeSetType{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{}));

  return params;
}

IFilter::VersionType ErodeDilateBadDataFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointerType ErodeDilateBadDataFilter::clone() const
{
  return std::make_unique<ErodeDilateBadDataFilter>();
}

IFilter::PreflightResult ErodeDilateBadDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                 const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pOperationValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  auto xDirOn = filterArgs.value<bool>(k_XDirOn_Key);
  auto yDirOn = filterArgs.value<bool>(k_YDirOn_Key);
  auto zDirOn = filterArgs.value<bool>(k_ZDirOn_Key);
  auto imageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  auto featureIdsResult = ValidateArrayInGeometryCellData(dataStructure, imageGeometryPath, pFeatureIdsArrayPathValue, -14605, "Cell Feature Ids");
  if(featureIdsResult.invalid())
  {
    return {.outputActions = ConvertResultTo<OutputActions>(std::move(featureIdsResult), {})};
  }

  auto ignoredArraysResult = ValidateArraysInGeometryCellData(dataStructure, imageGeometryPath, pIgnoredDataArrayPathsValue, -14604, "Attribute Arrays to Ignore");
  if(ignoredArraysResult.invalid())
  {
    return {.outputActions = ConvertResultTo<OutputActions>(std::move(ignoredArraysResult), {})};
  }

  // The Cell Attribute Matrix can have more than one DataPath. Use the geometry's Cell Attribute Matrix path so that the ignored paths match the modified paths.
  const auto cellDataPath = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath).getCellDataPath();
  pIgnoredDataArrayPathsValue = MapToGeometryCellDataPath(cellDataPath, pIgnoredDataArrayPathsValue);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  if(!xDirOn && !yDirOn && !zDirOn)
  {
    return {.outputActions = MakeErrorResult<OutputActions>(k_NoDirectionsError, "ErodeDilateBadData requires at least one direction to operate over")};
  }

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  auto dims = imageGeom.getDimensions();
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {.outputActions = MakeErrorResult<OutputActions>(k_NoGeometryDimensionsError, "ErodeDilateBadData requires that the ImageGeom have its dimensions set. No dimension may be 0.")};
  }

  std::string featureModificationWarning = "By modifying the cell level data, any feature data that was previously computed will most likely be invalid at this point. Filters that compute feature "
                                           "level data should be rerun to ensure accurate final results from your pipeline.";
  preflightUpdatedValues.emplace_back(PreflightValue{.name = "Feature Data Modification Warning", .value = featureModificationWarning});
  resultOutputActions.warnings().push_back(Warning{.code = -14600, .message = featureModificationWarning});

  // Report arrays that the filter modifies in place.
  // Cell Data arrays are modified.
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, cellDataPath, pIgnoredDataArrayPathsValue);

  // Return actions and updated preflight values.
  return {.outputActions = std::move(resultOutputActions), .outputValues = std::move(preflightUpdatedValues)};
}

Result<> ErodeDilateBadDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ErodeDilateBadDataInputValues inputValues;

  inputValues.Operation = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  inputValues.NumIterations = filterArgs.value<int32>(k_NumIterations_Key);
  inputValues.XDirOn = filterArgs.value<bool>(k_XDirOn_Key);
  inputValues.YDirOn = filterArgs.value<bool>(k_YDirOn_Key);
  inputValues.ZDirOn = filterArgs.value<bool>(k_ZDirOn_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  // The algorithm compares DataPaths and finds the Cell arrays through the parent of Feature Ids. Use the geometry's Cell Attribute Matrix path for Feature Ids and each ignored array.
  const auto cellDataPath = dataStructure.getDataRefAs<ImageGeom>(inputValues.InputImageGeometry).getCellDataPath();
  inputValues.FeatureIdsArrayPath = MapToGeometryCellDataPath(cellDataPath, inputValues.FeatureIdsArrayPath);
  inputValues.IgnoredDataArrayPaths = MapToGeometryCellDataPath(cellDataPath, inputValues.IgnoredDataArrayPaths);

  return ErodeDilateBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_DirectionKey = "Direction";
constexpr StringLiteralType k_NumIterationsKey = "NumIterations";
constexpr StringLiteralType k_XDirOnKey = "XDirOn";
constexpr StringLiteralType k_YDirOnKey = "YDirOn";
constexpr StringLiteralType k_ZDirOnKey = "ZDirOn";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_IgnoredDataArrayPathsKey = "IgnoredDataArrayPaths";
} // namespace simpl
} // namespace

Result<Arguments> ErodeDilateBadDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ErodeDilateBadDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_DirectionKey, k_Operation_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_NumIterationsKey, k_NumIterations_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_XDirOnKey, k_XDirOn_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_YDirOnKey, k_YDirOn_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_ZDirOnKey, k_ZDirOn_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  // IgnoredDataArrayPaths was added to the legacy filter after 6.5.49; older pipelines omit the key.
  if(json.contains(simpl::k_IgnoredDataArrayPathsKey))
  {
    results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::MultiDataArraySelectionFilterParameterConverter>(args, json, simpl::k_IgnoredDataArrayPathsKey, k_IgnoredDataArrayPaths_Key));
  }

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
