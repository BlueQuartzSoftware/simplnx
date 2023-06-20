#include "FindCAxisLocationsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindCAxisLocations.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindCAxisLocationsFilter::name() const
{
  return FilterTraits<FindCAxisLocationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindCAxisLocationsFilter::className() const
{
  return FilterTraits<FindCAxisLocationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindCAxisLocationsFilter::uuid() const
{
  return FilterTraits<FindCAxisLocationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindCAxisLocationsFilter::humanName() const
{
  return "Find C-Axis Locations";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindCAxisLocationsFilter::defaultTags() const
{
  return {"Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindCAxisLocationsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CAxisLocationsArrayName_Key, "C-Axis Locations", "", "CAxisLocation"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindCAxisLocationsFilter::clone() const
{
  return std::make_unique<FindCAxisLocationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindCAxisLocationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> dataPaths = {pQuatsArrayPathValue, pCellPhasesArrayPathValue};
  if(!dataStructure.validateNumberOfTuples(dataPaths))
  {
    return MakePreflightErrorResult(
        -3520, fmt::format("The quaternions cell data array '{}' and the phases cell data array '{}' have mismatching number of tuples. Make sure these arrays are both located in the "
                           "cell data attribute matrix for the selected geometry.",
                           pQuatsArrayPathValue.toString(), pCellPhasesArrayPathValue.toString()));
  }

  {
    const DataPath pCAxisLocationsPathValue = pQuatsArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_CAxisLocationsArrayName_Key));
    const std::vector<usize> tupleShape = dataStructure.getDataRefAs<Float32Array>(pQuatsArrayPathValue).getTupleShape();
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3}, pCAxisLocationsPathValue);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-3521, "Finding the c-axis locations requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Make sure your data is of one of these two types."});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindCAxisLocationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  FindCAxisLocationsInputValues inputValues;

  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.CAxisLocationsArrayName = inputValues.QuatsArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_CAxisLocationsArrayName_Key));

  return FindCAxisLocations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
