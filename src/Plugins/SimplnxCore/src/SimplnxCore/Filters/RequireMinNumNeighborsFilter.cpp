#include "RequireMinNumNeighborsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RequireMinNumNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
namespace
{
constexpr int32 k_InconsistentTupleCount = -252;

} // namespace

//------------------------------------------------------------------------------
std::string RequireMinNumNeighborsFilter::name() const
{
  return FilterTraits<RequireMinNumNeighborsFilter>::name;
}

//------------------------------------------------------------------------------
std::string RequireMinNumNeighborsFilter::className() const
{
  return FilterTraits<RequireMinNumNeighborsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RequireMinNumNeighborsFilter::uuid() const
{
  return FilterTraits<RequireMinNumNeighborsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RequireMinNumNeighborsFilter::humanName() const
{
  return "Require Minimum Number of Neighbors";
}

//------------------------------------------------------------------------------
std::vector<std::string> RequireMinNumNeighborsFilter::defaultTags() const
{
  return {className(), "Minimum", "Neighbors", "Memory Management", "Cleanup", "Remove Features"};
}

//------------------------------------------------------------------------------
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
                                                               std::vector<DataPath>{}, MultiArraySelectionParameter::AllowedTypes{AbstractArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{}));

  params.linkParameters(k_ApplyToSinglePhase_Key, k_PhaseNumber_Key, std::make_any<bool>(true));
  params.linkParameters(k_ApplyToSinglePhase_Key, k_FeaturePhasesPath_Key, std::make_any<bool>(true));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RequireMinNumNeighborsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RequireMinNumNeighborsFilter::clone() const
{
  return std::make_unique<RequireMinNumNeighborsFilter>();
}

//------------------------------------------------------------------------------
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

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;
  std::string featureModificationWarning = "By modifying the cell level data, any feature data that was previously computed will most likely be invalid at this point. Filters that compute feature "
                                           "level data should be rerun to ensure accurate final results from your pipeline.";
  preflightUpdatedValues.emplace_back(PreflightValue{"Feature Data Modification Warning", featureModificationWarning});

  std::vector<DataPath> dataArrayPaths;

  ShapeType cDims = {1};
  auto& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);

  auto& numNeighborsArray = dataStructure.getDataRefAs<Int32Array>(numNeighborsPath);
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

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, featureIdsPath.getParent(), {});
  // Feature Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, numNeighborsPath.getParent(), {});

  // This section will warn the user about the removal of NeighborLists
  auto result = nx::core::NeighborListRemovalPreflightCode(dataStructure, featureIdsPath, numNeighborsPath, resultOutputActions);
  if(result.outputActions.invalid())
  {
    return result;
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
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
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredVoxelArrays_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
