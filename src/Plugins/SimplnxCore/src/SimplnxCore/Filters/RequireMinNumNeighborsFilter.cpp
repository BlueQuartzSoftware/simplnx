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
constexpr int32 k_FeatureIdsTupleCountMismatch = -55571;

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
  params.insert(std::make_unique<UInt64Parameter>(k_MinNumNeighbors_Key, "Minimum Number Neighbors", "Number of neighbors a Feature must have to remain as a Feature", 0));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyToSinglePhase_Key, "Apply to Single Phase Only", "Whether to apply minimum to single ensemble or all ensembles", false));
  params.insert(std::make_unique<UInt64Parameter>(k_PhaseNumber_Key, "Phase Index", "Which Ensemble to apply minimum to. Only needed if Apply to Single Phase Only is checked", 0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumNeighborsPath_Key, "Number of Neighbors", "Number of contiguous neighboring Features for each Feature",
                                                          DataPath({"Data Container", "Feature Data", "NumNeighbors"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_FeaturePhasesPath_Key, "Feature Phases", "Specifies to which Ensemble each Feature belongs. Only required if Apply to Single Phase Only is checked",
      DataPath({"Data Container", "Feature Data", "Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Attribute Arrays to Ignore
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredVoxelArrays_Key, "Cell Arrays to Ignore", "The arrays to ignore when applying the minimum neighbors algorithm",
                                                               std::vector<DataPath>{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{}));

  params.linkParameters(k_ApplyToSinglePhase_Key, k_PhaseNumber_Key, std::make_any<bool>(true));
  params.linkParameters(k_ApplyToSinglePhase_Key, k_FeaturePhasesPath_Key, std::make_any<bool>(true));
  return params;
}

IFilter::VersionType RequireMinNumNeighborsFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer RequireMinNumNeighborsFilter::clone() const
{
  return std::make_unique<RequireMinNumNeighborsFilter>();
}

IFilter::PreflightResult RequireMinNumNeighborsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
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
    return {ConvertResultTo<OutputActions>(std::move(featureIdsResult), {})};
  }

  // The Cell Attribute Matrix can have more than one DataPath. Use the geometry's Cell Attribute Matrix path for Feature Ids.
  const auto cellDataPath = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath).getCellDataPath();
  featureIdsPath = MapToGeometryCellDataPath(cellDataPath, featureIdsPath);

  // ArraySelectionParameter also accepts StringArray and NeighborList for numeric selections.
  // Keep the local type checks until the parameter validates supported array kinds.
  // TODO(#1833): Revisit these guards after the shared fix preserves generic array support.
  const auto* featureIdsPtr = dataStructure.getDataAs<Int32Array>(featureIdsPath);
  if(featureIdsPtr == nullptr)
  {
    const auto& selectedObject = dataStructure.getDataRefAs<DataObject>(featureIdsPath);
    return MakePreflightErrorResult(-55574, fmt::format("The selected Feature Ids object '{}' has type '{}', but a scalar int32 DataArray is required. Select a scalar int32 DataArray.",
                                                        featureIdsPath.toString(), selectedObject.getTypeName()));
  }
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  auto cellTupleResult = ValidateCellTupleCount(imageGeom, imageGeomPath, featureIdsPtr->getNumberOfTuples(), featureIdsPath, k_FeatureIdsTupleCountMismatch);
  if(cellTupleResult.invalid())
  {
    return {.outputActions = ConvertResultTo<OutputActions>(std::move(cellTupleResult), {})};
  }

  auto ignoredArraysResult = ValidateArraysInGeometryCellData(dataStructure, imageGeomPath, ignoredDataArrayPaths, -55573, "Cell Arrays to Ignore");
  if(ignoredArraysResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ignoredArraysResult), {})};
  }

  auto featureAttributeMatrixResult = ValidateFeatureAttributeMatrixInGeometry(dataStructure, imageGeomPath, numNeighborsPath.getParent(), -55574, "Number of Neighbors", numNeighborsPath);
  if(featureAttributeMatrixResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(featureAttributeMatrixResult), {})};
  }

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

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

  if(dataStructure.getDataAs<Int32Array>(numNeighborsPath) == nullptr)
  {
    const auto& selectedObject = dataStructure.getDataRefAs<DataObject>(numNeighborsPath);
    return MakePreflightErrorResult(-55575, fmt::format("The selected Number of Neighbors object '{}' has type '{}', but a scalar int32 DataArray is required. Select a scalar int32 DataArray.",
                                                        numNeighborsPath.toString(), selectedObject.getTypeName()));
  }
  if(applyToSinglePhase && dataStructure.getDataAs<Int32Array>(featurePhasesPath) == nullptr)
  {
    const auto& selectedObject = dataStructure.getDataRefAs<DataObject>(featurePhasesPath);
    return MakePreflightErrorResult(-55576, fmt::format("The selected Feature Phases object '{}' has type '{}', but a scalar int32 DataArray is required. Select a scalar int32 DataArray.",
                                                        featurePhasesPath.toString(), selectedObject.getTypeName()));
  }

  // Report arrays that the filter modifies in place.
  // Cell Data arrays are modified.
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, cellDataPath, {});
  // Feature Data arrays are modified.
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, numNeighborsPath.getParent(), {});

  // Warn about stale IDataArrays in the Feature AM; NeighborListRemovalPreflightCode handles NeighborList deletion below
  nx::core::AppendRenumberedFeatureAMWarnings(dataStructure, numNeighborsPath.getParent(), featureIdsPath, preflightUpdatedValues, false);

  // Report removed NeighborList arrays.
  auto result = nx::core::NeighborListRemovalPreflightCode(dataStructure, featureIdsPath, numNeighborsPath, resultOutputActions);
  if(result.outputActions.invalid())
  {
    return result;
  }

  // Return actions and updated preflight values.
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

Result<> RequireMinNumNeighborsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  RequireMinNumNeighborsInputValues inputValues;

  inputValues.ApplyToSinglePhase = filterArgs.value<bool>(k_ApplyToSinglePhase_Key);
  inputValues.FeaturePhasesPath = filterArgs.value<DataPath>(k_FeaturePhasesPath_Key);
  inputValues.PhaseNumber = filterArgs.value<uint64>(k_PhaseNumber_Key);
  inputValues.MinNumNeighbors = filterArgs.value<uint64>(k_MinNumNeighbors_Key);
  inputValues.ImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.FeatureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  inputValues.NumNeighborsPath = filterArgs.value<DataPath>(k_NumNeighborsPath_Key);
  inputValues.IgnoredVoxelArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredVoxelArrays_Key);

  // The algorithm compares DataPaths and finds the Cell arrays through the parent of Feature Ids. Use the geometry's Cell Attribute Matrix path for Feature Ids and each ignored array.
  const auto cellDataPath = dataStructure.getDataRefAs<ImageGeom>(inputValues.ImageGeomPath).getCellDataPath();
  inputValues.FeatureIdsPath = MapToGeometryCellDataPath(cellDataPath, inputValues.FeatureIdsPath);
  inputValues.IgnoredVoxelArrayPaths = MapToGeometryCellDataPath(cellDataPath, inputValues.IgnoredVoxelArrayPaths);

  return RequireMinNumNeighbors(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MinNumNeighborsKey = "MinNumNeighbors";
constexpr StringLiteral k_ApplyToSinglePhaseKey = "ApplyToSinglePhase";
constexpr StringLiteral k_PhaseNumberKey = "PhaseNumber";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_NumNeighborsArrayPathKey = "NumNeighborsArrayPath";
constexpr StringLiteral k_IgnoredDataArrayPathsKey = "IgnoredDataArrayPaths";
} // namespace SIMPL
} // namespace

Result<Arguments> RequireMinNumNeighborsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RequireMinNumNeighborsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_MinNumNeighborsKey, k_MinNumNeighbors_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_ApplyToSinglePhaseKey, k_ApplyToSinglePhase_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_PhaseNumberKey, k_PhaseNumber_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NumNeighborsArrayPathKey, k_NumNeighborsPath_Key));
  // IgnoredDataArrayPaths was added to the legacy filter after 6.5.49; older pipelines omit the key.
  if(json.contains(SIMPL::k_IgnoredDataArrayPathsKey))
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredVoxelArrays_Key));
  }

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
