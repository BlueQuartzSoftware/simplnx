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
const DataPath k_ThrowawayCheckedFeatures = DataPath({"HiddenTempCheckedFeatures"});
const DataPath k_ThrowawayNonContiguous = DataPath({"HiddenNonContiguousNL"});
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
                                                                 DataPath{}, NeighborListSelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ParentIdsPath_Key, "Feature Parent Ids", "Input Feature level ParentIds", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureVolumesArrayPath_Key, "Feature Volumes", "The Feature Volumes Data Array", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<NeighborListSelectionParameter>(k_ContiguousNeighborListArrayPath_Key, "Contiguous Neighbor List", "List of contiguous neighbors for each Feature.", DataPath{},
                                                                 NeighborListSelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Input Parent Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ParentVolumesPath_Key, "Parent Volumes", "Input Parent feature level volumes data array", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

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
IFilter::UniquePointer ComputeGroupingDensityFilter::clone() const
{
  return std::make_unique<ComputeGroupingDensityFilter>();
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeGroupingDensityFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeGroupingDensityFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
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

  auto* pFeatureAM = dataStructure.getDataAs<AttributeMatrix>(pFeatureVolumesPath.getParent());
  if(pFeatureAM == nullptr)
  {
    return MakePreflightErrorResult(-15673, fmt::format("Feature Volumes [{}] must be stored in an Attribute Matrix.", pFeatureVolumesPath.toString()));
  }

  if(pFindCheckedFeatures)
  {
    {
      DataPath checkedFeaturesPath = pFeatureVolumesPath.replaceName(pCheckedFeaturesName);
      auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, pFeatureAM->getShape(), ShapeType{1}, checkedFeaturesPath);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
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

  auto* pParentAM = dataStructure.getDataAs<AttributeMatrix>(pParentVolumesPath.getParent());
  if(pParentAM == nullptr)
  {
    return MakePreflightErrorResult(-15670, fmt::format("Parent Volumes [{}] must be stored in an Attribute Matrix.", pParentVolumesPath.toString()));
  }

  {
    DataPath groupingDataPath = pParentVolumesPath.replaceName(pGroupingDensitiesName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, pParentAM->getShape(), std::vector<usize>{1}, groupingDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeGroupingDensityFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeGroupingDensityInputValues inputValues;

  inputValues.ParentIdsPath = filterArgs.value<DataPath>(k_ParentIdsPath_Key);
  inputValues.ParentVolumesPath = filterArgs.value<DataPath>(k_ParentVolumesPath_Key);
  inputValues.ContiguousNLPath = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  inputValues.VolumesPath = filterArgs.value<DataPath>(k_FeatureVolumesArrayPath_Key);
  inputValues.GroupingDensitiesPath = inputValues.ParentVolumesPath.replaceName(filterArgs.value<std::string>(k_GroupingDensitiesName_Key));

  inputValues.UseNonContiguousNeighbors = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  if(inputValues.UseNonContiguousNeighbors)
  {
    inputValues.NonContiguousNLPath = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  }
  else
  {
    inputValues.NonContiguousNLPath = k_ThrowawayNonContiguous;
  }

  inputValues.FindCheckedFeatures = filterArgs.value<bool>(k_FindCheckedFeatures_Key);
  if(inputValues.FindCheckedFeatures)
  {
    inputValues.CheckedFeaturesPath = inputValues.VolumesPath.replaceName(filterArgs.value<std::string>(k_CheckedFeaturesName_Key));
  }
  else
  {
    inputValues.CheckedFeaturesPath = k_ThrowawayCheckedFeatures;
  }

  return ComputeGroupingDensity(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_CheckedFeaturesArrayNameKey = "CheckedFeaturesArrayName";
constexpr StringLiteral k_ContiguousNeighborListArrayPathKey = "ContiguousNeighborListArrayPath";
constexpr StringLiteral k_FindCheckedFeaturesKey = "FindCheckedFeatures";
constexpr StringLiteral k_NonContiguousNeighborListArrayPathKey = "NonContiguousNeighborListArrayPath";
constexpr StringLiteral k_ParentDensitiesArrayNameKey = "ParentDensitiesArrayName";
constexpr StringLiteral k_ParentIdsArrayPathKey = "ParentIdsArrayPath";
constexpr StringLiteral k_ParentVolumesArrayPathKey = "ParentVolumesArrayPath";
constexpr StringLiteral k_UseNonContiguousNeighborsKey = "UseNonContiguousNeighbors";
constexpr StringLiteral k_VolumesArrayPathKey = "VolumesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeGroupingDensityFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeGroupingDensityFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CheckedFeaturesArrayNameKey, k_CheckedFeaturesName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ContiguousNeighborListArrayPathKey, k_ContiguousNeighborListArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindCheckedFeaturesKey, k_FindCheckedFeatures_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NonContiguousNeighborListArrayPathKey,
                                                                                                                   k_NonContiguousNeighborListArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_ParentDensitiesArrayNameKey, k_GroupingDensitiesName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ParentIdsArrayPathKey, k_ParentIdsPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ParentVolumesArrayPathKey, k_ParentVolumesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseNonContiguousNeighborsKey, k_UseNonContiguousNeighbors_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_VolumesArrayPathKey, k_FeatureVolumesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
