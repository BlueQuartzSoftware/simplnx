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
  params.insert(std::make_unique<Float32Parameter>(k_CAxisTolerance_Key, "C-Axis Alignment Tolerance (Degrees)", "C-Axis Alignment Tolerance (Degrees)", 0.0f));
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_ContiguousNeighborListArrayPath_Key, "Contiguous Neighbor List", "List of contiguous neighbors for each Feature.", DataPath{},
                                                                 NeighborListSelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Non-Contiguous Neighborhood Option"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNonContiguousNeighbors_Key, "Use Non-Contiguous Neighbors", "Use non-contiguous neighborhoods", false));
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_NonContiguousNeighborListArrayPath_Key, "Non-Contiguous Neighbor List", "List of non-contiguous neighbors for each Feature.",
                                                                 DataPath{}, NeighborListSelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Parent Id Randomization"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_RandomizeParentIds_Key, "Randomize Parent Ids",
      "When true, the final parent ids assigned to each group are randomly permuted. Disabled by default so identical inputs produce identical parent id assignments.", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_UseSeed_Key, "Use Seed for Random Generation",
      "When true the user-supplied Seed value is used for randomization and the random walk through the feature ids; otherwise the seed is derived from the system clock.", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "_Group_MicroTexture_Regions_Seed_Value_"));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "Specifies to which Ensemble each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VolumesArrayPath_Key, "Volumes", "The Feature Volumes Data Array", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Specifies the average orientation of each Feature in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellParentIdsArrayName_Key, "Cell Parent Ids Array name", "Output Cell Parent Ids Data Array", "Cell Parent Ids"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureParentIdsArrayName_Key, "Feature Parent Ids Array Name", "Output Feature Parent Ids Data Array", "Feature Parent Ids"));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewCellFeatureAttributeMatrixName_Key, "Created Microtexture Feature Attribute Matrix",
                                                             "Output Feature Attribute Matrix for Microtexture Regions", DataPath{}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ActiveArrayName_Key, "Active Array Name", "Output Active Array", "Active"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_NonContiguousNeighborListArrayPath_Key, true);
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GroupMicroTextureRegionsFilter::clone() const
{
  return std::make_unique<GroupMicroTextureRegionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::VersionType GroupMicroTextureRegionsFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GroupMicroTextureRegionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pFeatureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeaturePhasesPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pNewCellFeatureAMPath = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pCellParentIdsName = filterArgs.value<std::string>(k_CellParentIdsArrayName_Key);
  auto pFeatureParentIdsName = filterArgs.value<std::string>(k_FeatureParentIdsArrayName_Key);
  auto pActiveName = filterArgs.value<std::string>(k_ActiveArrayName_Key);
  auto pSeedArrayName = filterArgs.value<std::string>(k_SeedArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  {
    auto* featureIds = dataStructure.getDataAs<IDataArray>(pFeatureIdsPath);
    auto createAction = std::make_unique<CreateArrayAction>(DataType::int32, featureIds->getTupleShape(), std::vector<usize>{1}, pFeatureIdsPath.replaceName(pCellParentIdsName));
    resultOutputActions.value().appendAction(std::move(createAction));
  }
  {
    auto* featurePhases = dataStructure.getDataAs<IDataArray>(pFeaturePhasesPath);
    auto createAction = std::make_unique<CreateArrayAction>(DataType::int32, featurePhases->getTupleShape(), std::vector<usize>{1}, pFeaturePhasesPath.replaceName(pFeatureParentIdsName));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  {
    auto createAction = std::make_unique<CreateAttributeMatrixAction>(pNewCellFeatureAMPath, ShapeType{1});
    resultOutputActions.value().appendAction(std::move(createAction));
  }
  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::boolean, std::vector<usize>{1}, std::vector<usize>{1}, pNewCellFeatureAMPath.createChildPath(pActiveName));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayName}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GroupMicroTextureRegionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto seed = filterArgs.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  dataStructure.getDataRefAs<UInt64Array>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  GroupMicroTextureRegionsInputValues inputValues;

  inputValues.UseNonContiguousNeighbors = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  inputValues.NonContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  inputValues.ContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  inputValues.UseRunningAverage = filterArgs.value<bool>(k_UseRunningAverage_Key);
  inputValues.CAxisTolerance = filterArgs.value<float32>(k_CAxisTolerance_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.VolumesArrayPath = filterArgs.value<DataPath>(k_VolumesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.NewCellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  inputValues.CellParentIdsArrayName = inputValues.FeatureIdsArrayPath.replaceName(filterArgs.value<std::string>(k_CellParentIdsArrayName_Key));
  inputValues.FeatureParentIdsArrayName = inputValues.FeaturePhasesArrayPath.replaceName(filterArgs.value<std::string>(k_FeatureParentIdsArrayName_Key));
  inputValues.RandomizeParentIds = filterArgs.value<bool>(k_RandomizeParentIds_Key);
  inputValues.SeedValue = seed;

  return GroupMicroTextureRegions(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_ActiveArrayNameKey = "ActiveArrayName";
constexpr StringLiteral k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteral k_CAxisToleranceKey = "CAxisTolerance";
constexpr StringLiteral k_CellParentIdsArrayNameKey = "CellParentIdsArrayName";
constexpr StringLiteral k_ContiguousNeighborListArrayPathKey = "ContiguousNeighborListArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_FeatureParentIdsArrayNameKey = "FeatureParentIdsArrayName";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_NewCellFeatureAttributeMatrixNameKey = "NewCellFeatureAttributeMatrixName";
constexpr StringLiteral k_NonContiguousNeighborListArrayPathKey = "NonContiguousNeighborListArrayPath";
constexpr StringLiteral k_UseNonContiguousNeighborsKey = "UseNonContiguousNeighbors";
constexpr StringLiteral k_UseRunningAverageKey = "UseRunningAverage";
constexpr StringLiteral k_VolumesArrayPathKey = "VolumesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> GroupMicroTextureRegionsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = GroupMicroTextureRegionsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_ActiveArrayNameKey, k_ActiveArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_CAxisToleranceKey, k_CAxisTolerance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellParentIdsArrayNameKey, k_CellParentIdsArrayName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ContiguousNeighborListArrayPathKey, k_ContiguousNeighborListArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FeatureParentIdsArrayNameKey, k_FeatureParentIdsArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_NewCellFeatureAttributeMatrixNameKey,
                                                                                                                      k_NewCellFeatureAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NonContiguousNeighborListArrayPathKey,
                                                                                                                   k_NonContiguousNeighborListArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseNonContiguousNeighborsKey, k_UseNonContiguousNeighbors_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UseRunningAverageKey, k_UseRunningAverage_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_VolumesArrayPathKey, k_VolumesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
