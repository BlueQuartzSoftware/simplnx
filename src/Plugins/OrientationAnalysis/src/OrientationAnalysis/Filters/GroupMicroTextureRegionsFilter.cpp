#include "GroupMicroTextureRegionsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/GroupMicroTextureRegions.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NeighborListSelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <random>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string GroupMicroTextureRegionsFilter::name() const
{
  return FilterTraits<GroupMicroTextureRegionsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GroupMicroTextureRegionsFilter::className() const
{
  return FilterTraits<GroupMicroTextureRegionsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GroupMicroTextureRegionsFilter::uuid() const
{
  return FilterTraits<GroupMicroTextureRegionsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GroupMicroTextureRegionsFilter::humanName() const
{
  return "Group MicroTexture Regions";
}

//------------------------------------------------------------------------------
std::vector<std::string> GroupMicroTextureRegionsFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Grouping"};
}

//------------------------------------------------------------------------------
Parameters GroupMicroTextureRegionsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<BoolParameter>(k_UseRunningAverage_Key, "Group C-Axes With Running Average", "Group C-Axes With Running Average", true));
  params.insert(std::make_unique<Float32ParameterType>(k_CAxisTolerance_Key, "C-Axis Alignment Tolerance (Degrees)", "C-Axis Alignment Tolerance (Degrees)", 20.0f));
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_ContiguousNeighborListArrayPath_Key, "Contiguous Neighbor List", "List of contiguous neighbors for each Feature.", DataPath{},
                                                                 NeighborListSelectionParameter::AllowedTypeSetType{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Non-Contiguous Neighborhood Option"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNonContiguousNeighbors_Key, "Use Non-Contiguous Neighbors", "Use non-contiguous neighborhoods", false));
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_NonContiguousNeighborListArrayPath_Key, "Non-Contiguous Neighbor List", "List of non-contiguous neighbors for each Feature.",
                                                                 DataPath{}, NeighborListSelectionParameter::AllowedTypeSetType{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Parent Id Randomization"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_RandomizeParentIds_Key, "Randomize Parent Ids",
      "When true, the final parent ids assigned to each group are randomly permuted. Disabled by default so identical inputs produce identical parent id assignments.", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_UseSeed_Key, "Use Seed for Random Generation",
      "When true the user-supplied Seed value is used for randomization and the random walk through the feature ids; otherwise the seed is derived from the system clock. Enabled by "
      "default so that repeated runs on the same input reproduce the same grouping.",
      true));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "_Group_MicroTexture_Regions_Seed_Value_"));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "Specifies to which Ensemble each Feature belongs", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{nx::core::DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VolumesArrayPath_Key, "Volumes", "The Feature Volumes Data Array", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{nx::core::DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Specifies the average orientation of each Feature in quaternion representation", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{nx::core::DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{4}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{nx::core::DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellParentIdsArrayName_Key, "Cell Parent Ids Array name", "Output Cell Parent Ids Data Array", "Cell Parent Ids"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureParentIdsArrayName_Key, "Feature Parent Ids Array Name", "Output Feature Parent Ids Data Array", "Feature Parent Ids"));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewCellFeatureAttributeMatrixName_Key, "Created Microtexture Feature Attribute Matrix",
                                                             "Output Feature Attribute Matrix for Microtexture Regions", DataPath{}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_NonContiguousNeighborListArrayPath_Key, true);
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType GroupMicroTextureRegionsFilter::clone() const
{
  return std::make_unique<GroupMicroTextureRegionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::VersionType GroupMicroTextureRegionsFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GroupMicroTextureRegionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                       const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pFeatureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeaturePhasesPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pNewCellFeatureAMPath = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pCellParentIdsName = filterArgs.value<std::string>(k_CellParentIdsArrayName_Key);
  auto pFeatureParentIdsName = filterArgs.value<std::string>(k_FeatureParentIdsArrayName_Key);
  auto pSeedArrayName = filterArgs.value<std::string>(k_SeedArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  {
    const auto* featureIdsPtr = dataStructure.getDataAs<IDataArray>(pFeatureIdsPath);
    auto createAction = std::make_unique<CreateArrayAction>(DataType::int32, featureIdsPtr->getTupleShape(), std::vector<usize>{1}, pFeatureIdsPath.replaceName(pCellParentIdsName));
    resultOutputActions.value().appendAction(std::move(createAction));
  }
  {
    const auto* featurePhasesPtr = dataStructure.getDataAs<IDataArray>(pFeaturePhasesPath);
    auto createAction = std::make_unique<CreateArrayAction>(DataType::int32, featurePhasesPtr->getTupleShape(), std::vector<usize>{1}, pFeaturePhasesPath.replaceName(pFeatureParentIdsName));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  {
    auto createAction = std::make_unique<CreateAttributeMatrixAction>(pNewCellFeatureAMPath, ShapeType{1});
    resultOutputActions.value().appendAction(std::move(createAction));
  }
  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayName}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GroupMicroTextureRegionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto seed = filterArgs.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  GroupMicroTextureRegionsInputValues inputValues;

  inputValues.use_non_contiguous_neighbors = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  inputValues.non_contiguous_neighbor_list_array_path = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  inputValues.contiguous_neighbor_list_array_path = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  inputValues.use_running_average = filterArgs.value<bool>(k_UseRunningAverage_Key);
  inputValues.c_axis_tolerance = filterArgs.value<float32>(k_CAxisTolerance_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.feature_phases_array_path = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.volumes_array_path = filterArgs.value<DataPath>(k_VolumesArrayPath_Key);
  inputValues.avg_quats_array_path = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.new_cell_feature_attribute_matrix_name = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  inputValues.cell_parent_ids_array_name = inputValues.feature_ids_array_path.replaceName(filterArgs.value<std::string>(k_CellParentIdsArrayName_Key));
  inputValues.feature_parent_ids_array_name = inputValues.feature_phases_array_path.replaceName(filterArgs.value<std::string>(k_FeatureParentIdsArrayName_Key));
  inputValues.randomize_parent_ids = filterArgs.value<bool>(k_RandomizeParentIds_Key);
  inputValues.seed_value = seed;

  return GroupMicroTextureRegions(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteralType k_CAxisToleranceKey = "CAxisTolerance";
constexpr StringLiteralType k_CellParentIdsArrayNameKey = "CellParentIdsArrayName";
constexpr StringLiteralType k_ContiguousNeighborListArrayPathKey = "ContiguousNeighborListArrayPath";
constexpr StringLiteralType k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_FeatureParentIdsArrayNameKey = "FeatureParentIdsArrayName";
constexpr StringLiteralType k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteralType k_NewCellFeatureAttributeMatrixNameKey = "NewCellFeatureAttributeMatrixName";
constexpr StringLiteralType k_NonContiguousNeighborListArrayPathKey = "NonContiguousNeighborListArrayPath";
constexpr StringLiteralType k_UseNonContiguousNeighborsKey = "UseNonContiguousNeighbors";
constexpr StringLiteralType k_UseRunningAverageKey = "UseRunningAverage";
constexpr StringLiteralType k_VolumesArrayPathKey = "VolumesArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> GroupMicroTextureRegionsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = GroupMicroTextureRegionsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatFilterParameterConverter<float32>>(args, json, simpl::k_CAxisToleranceKey, k_CAxisTolerance_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellParentIdsArrayNameKey, k_CellParentIdsArrayName_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_ContiguousNeighborListArrayPathKey, k_ContiguousNeighborListArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_FeatureParentIdsArrayNameKey, k_FeatureParentIdsArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerCreationFilterParameterConverter>(args, json, simpl::k_NewCellFeatureAttributeMatrixNameKey,
                                                                                                                        k_NewCellFeatureAttributeMatrixName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_NonContiguousNeighborListArrayPathKey,
                                                                                                                     k_NonContiguousNeighborListArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseNonContiguousNeighborsKey, k_UseNonContiguousNeighbors_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_UseRunningAverageKey, k_UseRunningAverage_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_VolumesArrayPathKey, k_VolumesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
