#include "ErodeDilateCoordinationNumberFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ErodeDilateCoordinationNumber.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::name() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::className() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateCoordinationNumberFilter::uuid() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::humanName() const
{
  return "Erode/Dilate Coordination Number";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateCoordinationNumberFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Erode", "Dilate", "Smooth Bad Data"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateCoordinationNumberFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32ParameterType>(k_CoordinationNumber_Key, "Coordination Number to Consider",
                                                     " Number of neighboring **Cells** that can be of opposite classification before a **Cell** will be removed", 6));
  params.insert(std::make_unique<BoolParameter>(k_Loop_Key, "Loop Until Gone", "Keep looping until all criteria is met", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore when performing the algorithm",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypeSetType{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypesType{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ErodeDilateCoordinationNumberFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ErodeDilateCoordinationNumberFilter::clone() const
{
  return std::make_unique<ErodeDilateCoordinationNumberFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ErodeDilateCoordinationNumberFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                            const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pCoordinationNumberValue = filterArgs.value<int32>(k_CoordinationNumber_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  auto imageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  auto featureIdsResult = ValidateArrayInGeometryCellData(dataStructure, imageGeometryPath, pFeatureIdsArrayPathValue, -16801, "Cell Feature Ids");
  if(featureIdsResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(featureIdsResult), {})};
  }

  auto ignoredArraysResult = ValidateArraysInGeometryCellData(dataStructure, imageGeometryPath, pIgnoredDataArrayPathsValue, -16802, "Attribute Arrays to Ignore");
  if(ignoredArraysResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(ignoredArraysResult), {})};
  }

  // The Cell Attribute Matrix can have more than one DataPath. Use the geometry's Cell Attribute Matrix path so that the ignored paths match the modified paths.
  const auto cellDataPath = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath).getCellDataPath();
  pIgnoredDataArrayPathsValue = MapToGeometryCellDataPath(cellDataPath, pIgnoredDataArrayPathsValue);

  if(pCoordinationNumberValue < 0 || pCoordinationNumberValue > 6)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-16800, fmt::format("Coordination Number must be between 0 and 6. Current value: {}", pCoordinationNumberValue))};
  }

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modified_actions, cellDataPath, pIgnoredDataArrayPathsValue);

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ErodeDilateCoordinationNumberFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ErodeDilateCoordinationNumberInputValues inputValues;

  inputValues.coordination_number = filterArgs.value<int32>(k_CoordinationNumber_Key);
  inputValues.loop = filterArgs.value<bool>(k_Loop_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.ignored_data_array_paths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.input_image_geometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  // The algorithm compares DataPaths and finds the Cell arrays through the parent of Feature Ids. Use the geometry's Cell Attribute Matrix path for Feature Ids and each ignored array.
  const auto cellDataPath = dataStructure.getDataRefAs<ImageGeom>(inputValues.input_image_geometry).getCellDataPath();
  inputValues.feature_ids_array_path = MapToGeometryCellDataPath(cellDataPath, inputValues.feature_ids_array_path);
  inputValues.ignored_data_array_paths = MapToGeometryCellDataPath(cellDataPath, inputValues.ignored_data_array_paths);

  return ErodeDilateCoordinationNumber(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_CoordinationNumberKey = "CoordinationNumber";
constexpr StringLiteralType k_LoopKey = "Loop";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_IgnoredDataArrayPathsKey = "IgnoredDataArrayPaths";
} // namespace simpl
} // namespace

Result<Arguments> ErodeDilateCoordinationNumberFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ErodeDilateCoordinationNumberFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_CoordinationNumberKey, k_CoordinationNumber_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_LoopKey, k_Loop_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::MultiDataArraySelectionFilterParameterConverter>(args, json, simpl::k_IgnoredDataArrayPathsKey, k_IgnoredDataArrayPaths_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
