#include "MergeTwinsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/MergeTwins.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/NeighborListSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MergeTwinsFilter::name() const
{
  return FilterTraits<MergeTwinsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string MergeTwinsFilter::className() const
{
  return FilterTraits<MergeTwinsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid MergeTwinsFilter::uuid() const
{
  return FilterTraits<MergeTwinsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string MergeTwinsFilter::humanName() const
{
  return "Merge Twins";
}

//------------------------------------------------------------------------------
std::vector<std::string> MergeTwinsFilter::defaultTags() const
{
  return {"#Reconstruction", "#Grouping"};
}

//------------------------------------------------------------------------------
Parameters MergeTwinsFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Parameters"});

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNonContiguousNeighbors_Key, "Use Non-Contiguous Neighbors", "", false));

  params.insert(std::make_unique<Float32Parameter>(k_AxisTolerance_Key, "Axis Tolerance (Degrees)", "", 3.0F));
  params.insert(std::make_unique<Float32Parameter>(k_AngleTolerance_Key, "Angle Tolerance (Degrees)", "", 2.0F));

  params.insertSeparator(Parameters::Separator{"Required Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath({"FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Required Feature Data"});
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_ContiguousNeighborListArrayPath_Key, "Contiguous Neighbor List", "", DataPath({"NeighborList2"}),
                                                                 NeighborListSelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_NonContiguousNeighborListArrayPath_Key, "Non-Contiguous Neighbor List", "", DataPath{},
                                                                 NeighborListSelectionParameter::AllowedTypes{DataType::int32}));
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_ContiguousNeighborListArrayPath_Key, false);
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_NonContiguousNeighborListArrayPath_Key, true);

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath({"Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath({"Quats"}), ArraySelectionParameter::AllowedTypes{DataType::float32}));

  params.insertSeparator(Parameters::Separator{"Required Ensemble Data"});
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath({"CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32}));

  params.insertSeparator(Parameters::Separator{"Created Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellParentIdsArrayName_Key, "Parent Ids", "", DataPath({"ParentIds"})));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewCellFeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "", DataPath({"NewGrain Data"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureParentIdsArrayName_Key, "Parent Ids", "", DataPath({"ParentIds"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ActiveArrayName_Key, "Active", "", DataPath({"Active"})));
  // Associate the Linkable Parameter(s) to the children parameters that they control

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MergeTwinsFilter::clone() const
{
  return std::make_unique<MergeTwinsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MergeTwinsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto pUseNonContiguousNeighborsValue = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  auto pNonContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  auto pContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellParentIdsArrayNameValue = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  auto pNewCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pFeatureParentIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<size_t> cDims(1, 1);
  const NeighborList<int32>* contiguousNeighborList = dataStructure.getDataAs<NeighborList<int32>>(pContiguousNeighborListArrayPathValue);
  if(contiguousNeighborList == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-6874600, fmt::format("Could not find contiguous neighbor list of type Int32 at path '{}' ", pContiguousNeighborListArrayPathValue.toString())), {}};
  }
  if(pUseNonContiguousNeighborsValue)
  {
    const NeighborList<int32>* nonContiguousNeighborList = dataStructure.getDataAs<NeighborList<int32>>(pNonContiguousNeighborListArrayPathValue);
    if(nonContiguousNeighborList == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-6874601, fmt::format("Could not find non contiguous neighbor list of type Int32 at path '{}' ", pNonContiguousNeighborListArrayPathValue.toString())),
              {}};
    }
  }

  std::vector<size_t> tDims(1, 0);
  auto newCellFeatureAction =
      std::make_unique<CreateDataGroupAction>(pNewCellFeatureAttributeMatrixNameValue); // TODO: this should probably eventually be an AttributeMatrix with tDims as the tuplse shape
  resultOutputActions.value().actions.push_back(std::move(newCellFeatureAction));

  std::vector<DataPath> dataArrayPaths;

  // Cell Data
  const Int32Array* featureIds = dataStructure.getDataAs<Int32Array>(pFeatureIdsArrayPathValue);
  if(nullptr == featureIds)
  {
    return {MakeErrorResult<OutputActions>(-6874602, fmt::format("Could not find feature ids array of type Int32 at path '{}' ", pFeatureIdsArrayPathValue.toString())), {}};
  }
  auto cellParentIdsAction = std::make_unique<CreateArrayAction>(DataType::int32, featureIds->getIDataStore()->getTupleShape(), cDims, pCellParentIdsArrayNameValue);
  resultOutputActions.value().actions.push_back(std::move(cellParentIdsAction));

  // Feature Data
  const Int32Array* phases = dataStructure.getDataAs<Int32Array>(pFeaturePhasesArrayPathValue);
  if(nullptr == phases)
  {
    return {MakeErrorResult<OutputActions>(-6874603, fmt::format("Could not find phases array of type Int32 at path '{}' ", pFeaturePhasesArrayPathValue.toString())), {}};
  }
  dataArrayPaths.push_back(pFeaturePhasesArrayPathValue);

  auto featureParentIdsAction = std::make_unique<CreateArrayAction>(DataType::int32, phases->getIDataStore()->getTupleShape(), cDims, pFeatureParentIdsArrayNameValue);
  resultOutputActions.value().actions.push_back(std::move(featureParentIdsAction));

  cDims[0] = 4;
  const Float32Array* avgQuats = dataStructure.getDataAs<Float32Array>(pAvgQuatsArrayPathValue);
  if(nullptr == avgQuats)
  {
    return {MakeErrorResult<OutputActions>(-6874602, fmt::format("Could not find average quaternions array of type Float32 at path '{}' ", pAvgQuatsArrayPathValue.toString())), {}};
  }
  dataArrayPaths.push_back(pAvgQuatsArrayPathValue);

  // New Feature Data
  cDims[0] = 1;
  auto activeAction = std::make_unique<CreateArrayAction>(DataType::boolean, tDims, cDims, pActiveArrayNameValue);
  resultOutputActions.value().actions.push_back(std::move(activeAction));

  // Ensemble Data
  const UInt32Array* crystalStructures = dataStructure.getDataAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  if(nullptr == crystalStructures)
  {
    return {MakeErrorResult<OutputActions>(-6874602, fmt::format("Could not find crystal structures array of type UInt32 at path '{}' ", pCrystalStructuresArrayPathValue.toString())), {}};
  }

  dataStructure.validateNumberOfTuples(dataArrayPaths);

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> MergeTwinsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  MergeTwinsInputValues inputValues;
  GroupFeaturesInputValues groupInputValues;

  groupInputValues.UseNonContiguousNeighbors = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  groupInputValues.NonContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  groupInputValues.ContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  inputValues.AxisTolerance = filterArgs.value<float32>(k_AxisTolerance_Key);
  inputValues.AngleTolerance = filterArgs.value<float32>(k_AngleTolerance_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.CellParentIdsArrayName = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  inputValues.NewCellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  inputValues.FeatureParentIdsArrayName = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  inputValues.ActiveArrayName = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  return MergeTwins(dataStructure, messageHandler, shouldCancel, &inputValues, &groupInputValues)();
}
} // namespace complex
