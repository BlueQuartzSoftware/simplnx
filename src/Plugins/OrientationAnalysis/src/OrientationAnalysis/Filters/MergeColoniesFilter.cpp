#include "MergeColoniesFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/MergeColonies.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/NeighborListSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <random>

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MergeColoniesFilter::name() const
{
  return FilterTraits<MergeColoniesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string MergeColoniesFilter::className() const
{
  return FilterTraits<MergeColoniesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid MergeColoniesFilter::uuid() const
{
  return FilterTraits<MergeColoniesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string MergeColoniesFilter::humanName() const
{
  return "Merge Colonies";
}

//------------------------------------------------------------------------------
std::vector<std::string> MergeColoniesFilter::defaultTags() const
{
  return {"Reconstruction", "Grouping"};
}

//------------------------------------------------------------------------------
Parameters MergeColoniesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Optional Variables"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed", "The seed fed into the random generator", std::mt19937::default_seed));

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNonContiguousNeighbors_Key, "Use Non-Contiguous Neighbors", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_AxisTolerance_Key, "Axis Tolerance (Degrees)", "", 0.0f));
  params.insert(std::make_unique<Float32Parameter>(k_AngleTolerance_Key, "Angle Tolerance (Degrees)", "", 0.0f));

  params.insertSeparator(Parameters::Separator{"Required Feature Data"});
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_NonContiguousNeighborListArrayPath_Key, "Non-Contiguous Neighbor List", "", DataPath{},
                                                                 NeighborListSelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_ContiguousNeighborListArrayPath_Key, "Contiguous Neighbor List", "", DataPath{},
                                                                 NeighborListSelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));

  params.insertSeparator(Parameters::Separator{"Required Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Element Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellParentIdsArrayName_Key, "Cell Parent Ids", "", "Cell Parent Ids"));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewCellFeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureParentIdsArrayName_Key, "Feature Parent Ids", "", "Feature Parent Ids"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ActiveArrayName_Key, "Active", "", "Active Features"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_NonContiguousNeighborListArrayPath_Key, true);
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MergeColoniesFilter::clone() const
{
  return std::make_unique<MergeColoniesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MergeColoniesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto pFeaturePhasesPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeatureIdsPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellParentIdsNameValue = filterArgs.value<std::string>(k_CellParentIdsArrayName_Key);
  auto pCellFeatureAMPathValue = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pFeatureParentIdsNameValue = filterArgs.value<std::string>(k_FeatureParentIdsArrayName_Key);
  auto pActiveNameValue = filterArgs.value<std::string>(k_ActiveArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Create the CreateArray action and add it to the resultOutputActions object
  {
    auto action = std::make_unique<CreateAttributeMatrixAction>(pCellFeatureAMPathValue, std::vector<usize>{0});
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Create the CreateArray action and add it to the resultOutputActions object
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::boolean, std::vector<usize>{0}, std::vector<usize>{1}, pCellFeatureAMPathValue.createChildPath(pActiveNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Create the CreateArray action and add it to the resultOutputActions object
  {
    std::vector<usize> tupDims = dataStructure.getDataAs<IArray>(pFeaturePhasesPathValue)->getTupleShape();
    DataPath featureParentIdsPath = pFeaturePhasesPathValue.getParent().createChildPath(pFeatureParentIdsNameValue);
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, tupDims, std::vector<usize>{1}, featureParentIdsPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Create the CreateArray action and add it to the resultOutputActions object
  {
    std::vector<usize> tupDims = dataStructure.getDataAs<IArray>(pFeatureIdsPathValue)->getTupleShape();
    DataPath cellParentIdsPath = pFeatureIdsPathValue.getParent().createChildPath(pCellParentIdsNameValue);
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, tupDims, std::vector<usize>{1}, cellParentIdsPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> MergeColoniesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  MergeColoniesInputValues inputValues;

  auto seed = filterArgs.value<uint64>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  inputValues.SeedValue = seed;
  inputValues.AxisTolerance = filterArgs.value<float32>(k_AxisTolerance_Key);
  inputValues.AngleTolerance = filterArgs.value<float32>(k_AngleTolerance_Key);
  inputValues.FeaturePhasesPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.AvgQuatsPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeatureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CrystalStructuresPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.CellParentIdsPath = inputValues.FeatureIdsPath.getParent().createChildPath(filterArgs.value<std::string>(k_CellParentIdsArrayName_Key));
  inputValues.CellFeatureAMPath = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  inputValues.FeatureParentIdsPath = inputValues.FeaturePhasesPath.getParent().createChildPath(filterArgs.value<std::string>(k_FeatureParentIdsArrayName_Key));
  inputValues.ActivePath = inputValues.CellFeatureAMPath.createChildPath(filterArgs.value<std::string>(k_ActiveArrayName_Key));

  GroupFeaturesInputValues gpInputValues;

  gpInputValues.UseNonContiguousNeighbors = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  gpInputValues.NonContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  gpInputValues.ContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);

  return MergeColonies(dataStructure, messageHandler, shouldCancel, &inputValues, &gpInputValues)();
}
} // namespace complex
