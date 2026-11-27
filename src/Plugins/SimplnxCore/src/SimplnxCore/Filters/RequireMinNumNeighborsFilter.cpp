#include "RequireMinNumNeighborsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RequireMinNumNeighbors.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>

#include <any>
#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nx::core
{
namespace
{
constexpr int32 k_InconsistentTupleCount = -252;
constexpr int32 k_FeatureIdsTupleCountMismatch = -59571;

} // namespace

std::string RequireMinNumNeighborsFilter::name() const
{
  return FilterTraits<RequireMinNumNeighborsFilter>::name;
}

std::string RequireMinNumNeighborsFilter::className() const
{
  return FilterTraits<RequireMinNumNeighborsFilter>::className;
}

Uuid RequireMinNumNeighborsFilter::uuid() const
{
  return FilterTraits<RequireMinNumNeighborsFilter>::uuid;
}

std::string RequireMinNumNeighborsFilter::humanName() const
{
  return "Require Minimum Number of Neighbors";
}

std::vector<std::string> RequireMinNumNeighborsFilter::defaultTags() const
{
  return {className(), "Minimum", "Neighbors", "Memory Management", "Cleanup", "Remove Features"};
}

Parameters RequireMinNumNeighborsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt64ParameterType>(k_MinNumNeighbors_Key, "Minimum Number Neighbors", "Number of neighbors a Feature must have to remain as a Feature", 0));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyToSinglePhase_Key, "Apply to Single Phase Only", "Whether to apply minimum to single ensemble or all ensembles", false));
  params.insert(std::make_unique<UInt64ParameterType>(k_PhaseNumber_Key, "Phase Index", "Which Ensemble to apply minimum to. Only needed if Apply to Single Phase Only is checked", 0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumNeighborsPath_Key, "Number of Neighbors", "Number of contiguous neighboring Features for each Feature",
                                                          DataPath({"Data Container", "Feature Data", "NumNeighbors"}), GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesPath_Key, "Feature Phases",
                                                          "Specifies to which Ensemble each Feature belongs. Only required if Apply to Single Phase Only is checked",
                                                          DataPath({"Data Container", "Feature Data", "Phases"}), GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  // Attribute Arrays to Ignore
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredVoxelArrays_Key, "Cell Arrays to Ignore", "The arrays to ignore when applying the minimum neighbors algorithm",
                                                               std::vector<DataPath>{}, MultiArraySelectionParameter::AllowedArrayTypes{IArray::ArrayType::DataArray}, GetAllDataTypes()));

  params.linkParameters(k_ApplyToSinglePhase_Key, k_PhaseNumber_Key, std::make_any<bool>(true));
  params.linkParameters(k_ApplyToSinglePhase_Key, k_FeaturePhasesPath_Key, std::make_any<bool>(true));
  return params;
}

IFilter::VersionType RequireMinNumNeighborsFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointerType RequireMinNumNeighborsFilter::clone() const
{
  return std::make_unique<RequireMinNumNeighborsFilter>();
}

IFilter::PreflightResult RequireMinNumNeighborsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                     const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto applyToSinglePhase = filterArgs.value<bool>(k_ApplyToSinglePhase_Key);
  auto phaseNumber = filterArgs.value<uint64>(k_PhaseNumber_Key);
  auto featureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  auto featurePhasesPath = filterArgs.value<DataPath>(k_FeaturePhasesPath_Key);
  auto numNeighborsPath = filterArgs.value<DataPath>(k_NumNeighborsPath_Key);
  auto minNumNeighbors = filterArgs.value<uint64>(k_MinNumNeighbors_Key);
  auto ignoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredVoxelArrays_Key);

  auto featureIdsResult = ValidateArrayInGeometryCellData(dataStructure, imageGeomPath, featureIdsPath, -55575, "Cell Feature Ids");
  if(featureIdsResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(featureIdsResult), {})};
  }

  // The Cell Attribute Matrix can have more than one DataPath. Use the geometry's Cell Attribute Matrix path for Feature Ids.
  const auto cellDataPath = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath).getCellDataPath();
  featureIdsPath = MapToGeometryCellDataPath(cellDataPath, featureIdsPath);

  const auto* featureIdsPtr = dataStructure.getDataAs<Int32ArrayType>(featureIdsPath);
  if(featureIdsPtr == nullptr)
  {
    const auto& selectedObject = dataStructure.getDataRefAs<DataObject>(featureIdsPath);
    return MakePreflightErrorResult(-59574, fmt::format("The selected Feature Ids object '{}' has type '{}', but a scalar int32 DataArray is required. Select a scalar int32 DataArray.",
                                                        featureIdsPath.toString(), selectedObject.getTypeName()));
  }
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  auto cellTupleResult = ValidateCellTupleCount(imageGeom, imageGeomPath, featureIdsPtr->getNumberOfTuples(), featureIdsPath, k_FeatureIdsTupleCountMismatch);
  if(cellTupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(cellTupleResult), {})};
  }

  auto ignoredArraysResult = ValidateArraysInGeometryCellData(dataStructure, imageGeomPath, ignoredDataArrayPaths, -55573, "Cell Arrays to Ignore");
  if(ignoredArraysResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(ignoredArraysResult), {})};
  }

  auto featureAttributeMatrixResult = ValidateFeatureAttributeMatrixInGeometry(dataStructure, imageGeomPath, numNeighborsPath.getParent(), -55574, "Number of Neighbors", numNeighborsPath);
  if(featureAttributeMatrixResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(featureAttributeMatrixResult), {})};
  }

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;
  nx::core::AppendStaleFeatureArraysPreflightValue(dataStructure, numNeighborsPath.getParent(), featureIdsPath, preflightUpdatedValues);

  std::vector<DataPath> dataArrayPaths;

  dataArrayPaths.push_back(numNeighborsPath);

  if(applyToSinglePhase)
  {
    dataArrayPaths.push_back(featurePhasesPath);
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return MakePreflightErrorResult(k_InconsistentTupleCount, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
  }

  // Report arrays that the filter modifies in place.
  // Cell Data arrays are modified.
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modified_actions, cellDataPath, {});
  // Feature Data arrays are modified.
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modified_actions, numNeighborsPath.getParent(), {});

  // Report removed NeighborList arrays.
  auto result = nx::core::NeighborListRemovalPreflightCode(dataStructure, featureIdsPath, numNeighborsPath, resultOutputActions);
  if(result.output_actions.invalid())
  {
    return result;
  }

  // Return actions and updated preflight values.
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

Result<> RequireMinNumNeighborsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  RequireMinNumNeighborsInputValues inputValues;

  inputValues.apply_to_single_phase = filterArgs.value<bool>(k_ApplyToSinglePhase_Key);
  inputValues.feature_phases_path = filterArgs.value<DataPath>(k_FeaturePhasesPath_Key);
  inputValues.phase_number = filterArgs.value<uint64>(k_PhaseNumber_Key);
  inputValues.min_num_neighbors = filterArgs.value<uint64>(k_MinNumNeighbors_Key);
  inputValues.image_geom_path = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.feature_ids_path = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  inputValues.num_neighbors_path = filterArgs.value<DataPath>(k_NumNeighborsPath_Key);
  inputValues.ignored_voxel_array_paths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredVoxelArrays_Key);

  // The algorithm compares DataPaths and finds the Cell arrays through the parent of Feature Ids. Use the geometry's Cell Attribute Matrix path for Feature Ids and each ignored array.
  const auto cellDataPath = dataStructure.getDataRefAs<ImageGeom>(inputValues.image_geom_path).getCellDataPath();
  inputValues.feature_ids_path = MapToGeometryCellDataPath(cellDataPath, inputValues.feature_ids_path);
  inputValues.ignored_voxel_array_paths = MapToGeometryCellDataPath(cellDataPath, inputValues.ignored_voxel_array_paths);

  return RequireMinNumNeighbors(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_MinNumNeighborsKey = "MinNumNeighbors";
constexpr StringLiteralType k_ApplyToSinglePhaseKey = "ApplyToSinglePhase";
constexpr StringLiteralType k_PhaseNumberKey = "PhaseNumber";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteralType k_NumNeighborsArrayPathKey = "NumNeighborsArrayPath";
constexpr StringLiteralType k_IgnoredDataArrayPathsKey = "IgnoredDataArrayPaths";
} // namespace simpl
} // namespace

Result<Arguments> RequireMinNumNeighborsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RequireMinNumNeighborsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<uint64>>(args, json, simpl::k_MinNumNeighborsKey, k_MinNumNeighbors_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_ApplyToSinglePhaseKey, k_ApplyToSinglePhase_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<uint64>>(args, json, simpl::k_PhaseNumberKey, k_PhaseNumber_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeaturePhasesArrayPathKey, k_FeaturePhasesPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_NumNeighborsArrayPathKey, k_NumNeighborsPath_Key));
  // IgnoredDataArrayPaths was added to the legacy filter after 6.5.49; older pipelines omit the key.
  if(json.contains(simpl::k_IgnoredDataArrayPathsKey))
  {
    results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::MultiDataArraySelectionFilterParameterConverter>(args, json, simpl::k_IgnoredDataArrayPathsKey, k_IgnoredVoxelArrays_Key));
  }

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
