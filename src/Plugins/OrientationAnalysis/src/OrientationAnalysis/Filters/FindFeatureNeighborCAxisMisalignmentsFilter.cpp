#include "FindFeatureNeighborCAxisMisalignmentsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindFeatureNeighborCAxisMisalignments.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/NeighborListSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeatureNeighborCAxisMisalignmentsFilter::name() const
{
  return FilterTraits<FindFeatureNeighborCAxisMisalignmentsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureNeighborCAxisMisalignmentsFilter::className() const
{
  return FilterTraits<FindFeatureNeighborCAxisMisalignmentsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureNeighborCAxisMisalignmentsFilter::uuid() const
{
  return FilterTraits<FindFeatureNeighborCAxisMisalignmentsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureNeighborCAxisMisalignmentsFilter::humanName() const
{
  return "Find Feature Neighbor C-Axis Misalignments";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureNeighborCAxisMisalignmentsFilter::defaultTags() const
{
  return {"Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureNeighborCAxisMisalignmentsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindAvgMisals_Key, "Find Average Misalignment Per Feature",
                                                                 "Whether the average of the C-axis misalignments with the neighboring Features should be stored for each Feature", false));
  params.insertSeparator(Parameters::Separator{"Required Cell Feature Data"});
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_NeighborListArrayPath_Key, "Neighbor List", "List of the contiguous neighboring Features for a given Feature", DataPath{},
                                                                 NeighborListSelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Defines the average orientation of the Feature in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each **Feature** belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CAxisMisalignmentListArrayName_Key, "C-Axis Misalignment List",
                                                          "List of the C-axis misalignment angles (in degrees) with the contiguous neighboring Features for a given Feature", "CAxisMisalignmentList"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AvgCAxisMisalignmentsArrayName_Key, "Average C-Axis Misalignments",
                                                          "Number weighted average of neighbor C-axis misalignments. Only created if Find Average Misalignment Per Feature is checked",
                                                          "AvgCAxisMisalignments"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_FindAvgMisals_Key, k_AvgCAxisMisalignmentsArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureNeighborCAxisMisalignmentsFilter::clone() const
{
  return std::make_unique<FindFeatureNeighborCAxisMisalignmentsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeatureNeighborCAxisMisalignmentsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pFindAvgMisalsValue = filterArgs.value<bool>(k_FindAvgMisals_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCAxisMisalignmentListArrayNameValue = filterArgs.value<std::string>(k_CAxisMisalignmentListArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(!dataStructure.validateNumberOfTuples({pNeighborListArrayPathValue, pAvgQuatsArrayPathValue, pFeaturePhasesArrayPathValue}))
  {
    return MakePreflightErrorResult(
        -1560, fmt::format("The average quaternions feature data array '{}', neighbor list feature data array '{}' and the feature phases data array '{}' have mismatching number of tuples. Make "
                           "sure these arrays are all located in the cell data attribute matrix for the selected geometry.",
                           pAvgQuatsArrayPathValue.toString(), pNeighborListArrayPathValue.toString(), pFeaturePhasesArrayPathValue.toString()));
  }

  const auto& featurePhases = dataStructure.getDataRefAs<Int32Array>(pFeaturePhasesArrayPathValue);
  {
    auto createArrayAction = std::make_unique<CreateNeighborListAction>(DataType::float32, featurePhases.getNumberOfTuples(),
                                                                        pFeaturePhasesArrayPathValue.getParent().createChildPath(pCAxisMisalignmentListArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  if(pFindAvgMisalsValue)
  {
    auto pAvgCAxisMisalignmentsArrayNameValue = filterArgs.value<std::string>(k_AvgCAxisMisalignmentsArrayName_Key);
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, featurePhases.getTupleShape(), std::vector<usize>{1},
                                                                 pFeaturePhasesArrayPathValue.getParent().createChildPath(pAvgCAxisMisalignmentsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-1561, "Finding the feature neighbor c-axis mis orientation requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Make sure your data is of one of these types."});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureNeighborCAxisMisalignmentsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  FindFeatureNeighborCAxisMisalignmentsInputValues inputValues;

  inputValues.FindAvgMisals = filterArgs.value<bool>(k_FindAvgMisals_Key);
  inputValues.NeighborListArrayPath = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.CAxisMisalignmentListArrayName = inputValues.FeaturePhasesArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_CAxisMisalignmentListArrayName_Key));
  inputValues.AvgCAxisMisalignmentsArrayName = inputValues.FeaturePhasesArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_AvgCAxisMisalignmentsArrayName_Key));

  return FindFeatureNeighborCAxisMisalignments(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
