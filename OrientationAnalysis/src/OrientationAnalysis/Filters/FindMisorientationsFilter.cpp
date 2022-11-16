#include "FindMisorientationsFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/NeighborListSelectionParameter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindMisorientations.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindMisorientationsFilter::name() const
{
  return FilterTraits<FindMisorientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindMisorientationsFilter::className() const
{
  return FilterTraits<FindMisorientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindMisorientationsFilter::uuid() const
{
  return FilterTraits<FindMisorientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindMisorientationsFilter::humanName() const
{
  return "Find Feature Neighbor Misorientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindMisorientationsFilter::defaultTags() const
{
  return {"#Statistics", "#Crystallography", "#Misorientation"};
}

//------------------------------------------------------------------------------
Parameters FindMisorientationsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindAvgMisors_Key, "Find Average Misorientation Per Feature",
                                                                 "Specifies if the average of the misorienations with the neighboring Features should be stored for each Feature", false));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_NeighborListArrayPath_Key, "Feature Neighbor List", "List of the contiguous neighboring Features for a given Feature",
                                                                 DataPath({"DataContainer", "FeatureData", "NeighborList"}), NeighborListSelectionParameter::AllowedTypes{complex::DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Feature Average Quaternions", "Defines the average orientation of the Feature in quaternion representation",
                                                          DataPath({"DataContainer", "FeatureData", "AvgQuats"}), ArraySelectionParameter::AllowedTypes{complex::DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "Specifies to which Ensemble each Feature belongs",
                                                          DataPath({"DataContainer", "FeatureData", "Phases"}), ArraySelectionParameter::AllowedTypes{complex::DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"DataContainer", "CellEnsembleData", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_MisorientationListArrayName_Key, "Misorientation List",
                                                          "The name of the data object containing the list of the misorientation angles with the contiguous neighboring Features for a given Feature",
                                                          "MisorientationList"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AvgMisorientationsArrayName_Key, "Average Misorientations",
                                                          "The name of the array containing the number weighted average of neighbor misorientations.", "AvgMisorientations"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_FindAvgMisors_Key, k_AvgMisorientationsArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindMisorientationsFilter::clone() const
{
  return std::make_unique<FindMisorientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindMisorientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFindAvgMisorsValue = filterArgs.value<bool>(k_FindAvgMisors_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto cellFeatDataPath = pAvgQuatsArrayPathValue.getParent();
  auto pMisorientationListArrayPath = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_MisorientationListArrayName_Key));
  auto pAvgMisorientationsArrayPath = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_AvgMisorientationsArrayName_Key));

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  std::vector<DataPath> dataArrayPaths;

  // Validate the Quats is a 4 component array
  const auto* avgQuats = dataStructure.getDataAs<Float32Array>(pAvgQuatsArrayPathValue);
  if(avgQuats->getNumberOfComponents() != 4)
  {
    return {MakeErrorResult<OutputActions>(-34500, "Input Average Quaternions does not have 4 components.")};
  }

  const auto* featurePhases = dataStructure.getDataAs<Int32Array>(pFeaturePhasesArrayPathValue);

  dataArrayPaths.push_back(pAvgQuatsArrayPathValue);
  dataArrayPaths.push_back(pFeaturePhasesArrayPathValue);
  dataArrayPaths.push_back(pNeighborListArrayPathValue);

  if(!dataStructure.validateNumberOfTuples(dataArrayPaths))
  {
    return {MakeErrorResult<OutputActions>(-34501, "Input DataArrays do not have matching tuple count")};
  }

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  if(pFindAvgMisorsValue)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float32, avgQuats->getIDataStore()->getTupleShape(), std::vector<usize>{1}, pAvgMisorientationsArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Create the NeighborList array
  auto createArrayAction = std::make_unique<CreateNeighborListAction>(complex::DataType::float32, avgQuats->getNumberOfTuples(), pMisorientationListArrayPath);
  resultOutputActions.value().actions.push_back(std::move(createArrayAction));

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindMisorientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  FindMisorientationsInputValues inputValues;

  inputValues.FindAvgMisors = filterArgs.value<bool>(k_FindAvgMisors_Key);
  inputValues.NeighborListArrayPath = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto cellFeatDataPath = inputValues.AvgQuatsArrayPath.getParent();
  inputValues.MisorientationListArrayName = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_MisorientationListArrayName_Key));
  inputValues.AvgMisorientationsArrayName = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_AvgMisorientationsArrayName_Key));

  return FindMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
