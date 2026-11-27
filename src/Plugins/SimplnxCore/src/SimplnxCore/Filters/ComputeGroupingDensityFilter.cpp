#include "ComputeGroupingDensityFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeGroupingDensity.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NeighborListSelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
// Throwaway DataPaths used when the user opts out of either output. The
// underlying algorithm always expects a valid Int32Array for CheckedFeatures
// and a valid Int32NeighborList for NonContiguousNeighbors, even when those
// outputs are unused. preflightImpl() creates these temporary objects when
// the user opts out and schedules a deferred delete; the algorithm writes
// to them (the writes are harmless) and they are cleaned up at the end of
// execute. This keeps the algorithm interface free of std::optional or
// nullable references.
const auto k_ThrowawayCheckedFeatures = DataPath({"HiddenTempCheckedFeatures"});
const auto k_ThrowawayNonContiguous = DataPath({"HiddenNonContiguousNL"});
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeGroupingDensityFilter::name() const
{
  return FilterTraits<ComputeGroupingDensityFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeGroupingDensityFilter::className() const
{
  return FilterTraits<ComputeGroupingDensityFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeGroupingDensityFilter::uuid() const
{
  return FilterTraits<ComputeGroupingDensityFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeGroupingDensityFilter::humanName() const
{
  return "Compute Grouping Densities";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeGroupingDensityFilter::defaultTags() const
{
  return {className(), "Statistics", "Reconstruction", "Microtexture"};
}

//------------------------------------------------------------------------------
Parameters ComputeGroupingDensityFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindCheckedFeatures_Key, "Find Checked Features", "Find checked features", false));

  params.insertSeparator(Parameters::Separator{"Non-Contiguous Neighborhood Option"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNonContiguousNeighbors_Key, "Use Non-Contiguous Neighbors", "Use non-contiguous neighborhoods for computations", false));
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_NonContiguousNeighborListArrayPath_Key, "Non-Contiguous Neighborhoods", "List of non-contiguous neighbors for each Feature.",
                                                                 DataPath{}, NeighborListSelectionParameter::AllowedTypeSetType{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ParentIdsPath_Key, "Feature Parent Ids", "Input Feature level ParentIds", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureVolumesArrayPath_Key, "Feature Volumes", "The Feature Volumes Data Array", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{nx::core::DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insert(std::make_unique<NeighborListSelectionParameter>(k_ContiguousNeighborListArrayPath_Key, "Contiguous Neighbor List", "List of contiguous neighbors for each Feature.", DataPath{},
                                                                 NeighborListSelectionParameter::AllowedTypeSetType{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Input Parent Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ParentVolumesPath_Key, "Parent Volumes", "Input Parent feature level volumes data array", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});

  params.insert(std::make_unique<DataObjectNameParameter>(k_CheckedFeaturesName_Key, "Checked Features Name", "Output feature level data array to hold 'Checked Features' values", "Checked Features"));

  params.insertSeparator(Parameters::Separator{"Output Parent Feature Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_GroupingDensitiesName_Key, "Grouping Densities Name", "Output feature level data array to hold 'Grouping Densities' values", "Grouping Densities"));

  // Link params
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_NonContiguousNeighborListArrayPath_Key, true);
  params.linkParameters(k_FindCheckedFeatures_Key, k_CheckedFeaturesName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeGroupingDensityFilter::clone() const
{
  return std::make_unique<ComputeGroupingDensityFilter>();
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeGroupingDensityFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeGroupingDensityFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                     const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pParentIdsPath = filterArgs.value<DataPath>(k_ParentIdsPath_Key);
  auto pFeatureVolumesPath = filterArgs.value<DataPath>(k_FeatureVolumesArrayPath_Key);
  auto pContiguousNLPath = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);

  auto pParentVolumesPath = filterArgs.value<DataPath>(k_ParentVolumesPath_Key);

  auto pUseNonContiguousNeighbors = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  auto pNonContiguousNLPath = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);

  auto pFindCheckedFeatures = filterArgs.value<bool>(k_FindCheckedFeatures_Key);
  auto pCheckedFeaturesName = filterArgs.value<std::string>(k_CheckedFeaturesName_Key);
  auto pGroupingDensitiesName = filterArgs.value<std::string>(k_GroupingDensitiesName_Key);

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Selection parameters auto-validate existence, so use references directly
  const auto& parentIds = dataStructure.getDataRefAs<IDataArray>(pParentIdsPath);
  const auto& featureVolumes = dataStructure.getDataRefAs<IDataArray>(pFeatureVolumesPath);
  const auto& contiguousNL = dataStructure.getDataRefAs<INeighborList>(pContiguousNLPath);

  // Make sure all these arrays and neighbor lists all come from the same attribute matrix or at least have the same number of tuples
  if(parentIds.getNumberOfTuples() != featureVolumes.getNumberOfTuples() || parentIds.getNumberOfTuples() != contiguousNL.getNumberOfTuples())
  {
    return MakePreflightErrorResult(-15671, fmt::format("All Input Feature level data arrays and neighbor lists MUST have the same number of tuples.\n{}: {}\n{}: {}\n{}: {}",
                                                        pParentIdsPath.toString(), parentIds.getNumberOfTuples(), pFeatureVolumesPath.toString(), featureVolumes.getNumberOfTuples(),
                                                        pContiguousNLPath.toString(), contiguousNL.getNumberOfTuples()));
  }
  if(pUseNonContiguousNeighbors)
  {
    const auto& nonContiguousNL = dataStructure.getDataRefAs<INeighborList>(pNonContiguousNLPath);
    if(parentIds.getNumberOfTuples() != nonContiguousNL.getNumberOfTuples())
    {
      return MakePreflightErrorResult(-15672, fmt::format("All Input Feature level data arrays and neighbor lists MUST have the same number of tuples.\n{}: {}\n{}: {}", pParentIdsPath.toString(),
                                                          parentIds.getNumberOfTuples(), pNonContiguousNLPath.toString(), nonContiguousNL.getNumberOfTuples()));
    }
  }

  const auto* pFeatureAmPtr = dataStructure.getDataAs<AttributeMatrix>(pFeatureVolumesPath.getParent());
  if(pFeatureAmPtr == nullptr)
  {
    return MakePreflightErrorResult(-15673, fmt::format("Feature Volumes [{}] must be stored in an Attribute Matrix.", pFeatureVolumesPath.toString()));
  }

  // CheckedFeatures output: create the real output when requested; otherwise
  // create the throwaway placeholder (see the k_ThrowawayCheckedFeatures
  // comment block at the top of this file) and schedule its deletion.
  if(pFindCheckedFeatures)
  {
    DataPath checkedFeaturesPath = pFeatureVolumesPath.replaceName(pCheckedFeaturesName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, pFeatureAmPtr->getShape(), ShapeType{1}, checkedFeaturesPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  else
  {
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, ShapeType{1}, ShapeType{1}, k_ThrowawayCheckedFeatures);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    {
      auto removeAction = std::make_unique<DeleteDataAction>(k_ThrowawayCheckedFeatures);
      resultOutputActions.value().appendDeferredAction(std::move(removeAction));
    }
  }

  // Non-contiguous neighbor list: when the user has opted out, create a
  // throwaway 1-tuple neighbor list and schedule its deletion. See the
  // k_ThrowawayNonContiguous comment block at the top of this file.
  if(!pUseNonContiguousNeighbors)
  {
    {
      auto createArrayAction = std::make_unique<CreateNeighborListAction>(nx::core::DataType::int32, ShapeType{1}, k_ThrowawayNonContiguous);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    {
      auto removeAction = std::make_unique<DeleteDataAction>(k_ThrowawayNonContiguous);
      resultOutputActions.value().appendDeferredAction(std::move(removeAction));
    }
  }

  const auto* pParentAmPtr = dataStructure.getDataAs<AttributeMatrix>(pParentVolumesPath.getParent());
  if(pParentAmPtr == nullptr)
  {
    return MakePreflightErrorResult(-15670, fmt::format("Parent Volumes [{}] must be stored in an Attribute Matrix.", pParentVolumesPath.toString()));
  }

  {
    DataPath groupingDataPath = pParentVolumesPath.replaceName(pGroupingDensitiesName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, pParentAmPtr->getShape(), std::vector<usize>{1}, groupingDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeGroupingDensityFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeGroupingDensityInputValues inputValues;

  inputValues.parent_ids_path = filterArgs.value<DataPath>(k_ParentIdsPath_Key);
  inputValues.parent_volumes_path = filterArgs.value<DataPath>(k_ParentVolumesPath_Key);
  inputValues.contiguous_nl_path = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  inputValues.volumes_path = filterArgs.value<DataPath>(k_FeatureVolumesArrayPath_Key);
  inputValues.grouping_densities_path = inputValues.parent_volumes_path.replaceName(filterArgs.value<std::string>(k_GroupingDensitiesName_Key));

  inputValues.use_non_contiguous_neighbors = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  if(inputValues.use_non_contiguous_neighbors)
  {
    inputValues.non_contiguous_nl_path = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  }
  else
  {
    inputValues.non_contiguous_nl_path = k_ThrowawayNonContiguous;
  }

  inputValues.find_checked_features = filterArgs.value<bool>(k_FindCheckedFeatures_Key);
  if(inputValues.find_checked_features)
  {
    inputValues.checked_features_path = inputValues.volumes_path.replaceName(filterArgs.value<std::string>(k_CheckedFeaturesName_Key));
  }
  else
  {
    inputValues.checked_features_path = k_ThrowawayCheckedFeatures;
  }

  return ComputeGroupingDensity(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_CheckedFeaturesArrayNameKey = "CheckedFeaturesArrayName";
constexpr StringLiteralType k_ContiguousNeighborListArrayPathKey = "ContiguousNeighborListArrayPath";
constexpr StringLiteralType k_FindCheckedFeaturesKey = "FindCheckedFeatures";
constexpr StringLiteralType k_NonContiguousNeighborListArrayPathKey = "NonContiguousNeighborListArrayPath";
constexpr StringLiteralType k_ParentDensitiesArrayNameKey = "ParentDensitiesArrayName";
constexpr StringLiteralType k_ParentIdsArrayPathKey = "ParentIdsArrayPath";
constexpr StringLiteralType k_ParentVolumesArrayPathKey = "ParentVolumesArrayPath";
constexpr StringLiteralType k_UseNonContiguousNeighborsKey = "UseNonContiguousNeighbors";
constexpr StringLiteralType k_VolumesArrayPathKey = "VolumesArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> ComputeGroupingDensityFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeGroupingDensityFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CheckedFeaturesArrayNameKey, k_CheckedFeaturesName_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_ContiguousNeighborListArrayPathKey, k_ContiguousNeighborListArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_FindCheckedFeaturesKey, k_FindCheckedFeatures_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_NonContiguousNeighborListArrayPathKey,
                                                                                                                   k_NonContiguousNeighborListArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_ParentDensitiesArrayNameKey, k_GroupingDensitiesName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_ParentIdsArrayPathKey, k_ParentIdsPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_ParentVolumesArrayPathKey, k_ParentVolumesPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseNonContiguousNeighborsKey, k_UseNonContiguousNeighbors_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_VolumesArrayPathKey, k_FeatureVolumesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
