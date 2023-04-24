#include "FindAvgCAxesFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindAvgCAxes.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindAvgCAxesFilter::name() const
{
  return FilterTraits<FindAvgCAxesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindAvgCAxesFilter::className() const
{
  return FilterTraits<FindAvgCAxesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindAvgCAxesFilter::uuid() const
{
  return FilterTraits<FindAvgCAxesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindAvgCAxesFilter::humanName() const
{
  return "Find Average C-Axis Orientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindAvgCAxesFilter::defaultTags() const
{
  return {"Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindAvgCAxesFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrix_Key, "Cell Feature Attribute Matrix", "The path to the cell feature attribute matrix", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Required Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_AvgCAxesArrayPath_Key, "Average C-Axes", "", "AvgCAxes"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindAvgCAxesFilter::clone() const
{
  return std::make_unique<FindAvgCAxesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindAvgCAxesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrix_Key);
  auto pAvgCAxesArrayNameValue = filterArgs.value<std::string>(k_AvgCAxesArrayPath_Key);

  const DataPath avgCAxesPath = pCellFeatureAttributeMatrixPathValue.createChildPath(pAvgCAxesArrayNameValue);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> dataPaths = {pQuatsArrayPathValue, pFeatureIdsArrayPathValue, pCellPhasesArrayPathValue};
  if(!dataStructure.validateNumberOfTuples(dataPaths))
  {
    return MakePreflightErrorResult(
        -6400,
        fmt::format(
            "The quaternions cell data array '{}', feature ids cell data array '{}', and phases cell data array '{}' have mismatching number of tuples. Make sure these arrays are all located in the "
            "cell data attribute matrix for the selected geometry.",
            pQuatsArrayPathValue.toString(), pFeatureIdsArrayPathValue.toString(), pCellPhasesArrayPathValue.toString()));
  }

  {
    std::vector<usize> tupleShape = dataStructure.getDataRefAs<AttributeMatrix>(pCellFeatureAttributeMatrixPathValue).getShape();
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3}, avgCAxesPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-6401, "Finding the average c-axes requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Make sure your data is of one of these two types."});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindAvgCAxesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  FindAvgCAxesInputValues inputValues;

  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CellFeatureDataPath = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrix_Key);
  inputValues.AvgCAxesArrayPath = inputValues.CellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_AvgCAxesArrayPath_Key));
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return FindAvgCAxes(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
