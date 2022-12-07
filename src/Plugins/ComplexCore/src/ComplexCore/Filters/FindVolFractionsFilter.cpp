#include "FindVolFractionsFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindVolFractionsFilter::name() const
{
  return FilterTraits<FindVolFractionsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindVolFractionsFilter::className() const
{
  return FilterTraits<FindVolFractionsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindVolFractionsFilter::uuid() const
{
  return FilterTraits<FindVolFractionsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindVolFractionsFilter::humanName() const
{
  return "Find Volume Fractions of Ensembles";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindVolFractionsFilter::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindVolFractionsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Objects: Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Array specifying which Ensemble each Cell belong",
                                                          DataPath({"DataContainer", "CellData", "Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Objects: Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_VolFractionsArrayPath_Key, "Volume Fractions", "Fraction of volume that belongs to each Ensemble", DataPath({"Volume Fractions"}),
                                                         ArrayCreationParameter::AllowedParentGroupType{BaseGroup::GroupType::BaseGroup}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindVolFractionsFilter::clone() const
{
  return std::make_unique<FindVolFractionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindVolFractionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pVolFractionsArrayPathValue = filterArgs.value<DataPath>(k_VolFractionsArrayPath_Key);

  const auto& cellPhasesArray = dataStructure.getDataRefAs<IDataArray>(pCellPhasesArrayPathValue);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto* cellEnsembleData = dataStructure.getDataAs<AttributeMatrix>(pVolFractionsArrayPathValue.getParent());
  if(cellEnsembleData == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-47630, fmt::format("Could not find selected feature Attribute Matrix at path '{}'", pVolFractionsArrayPathValue.getParent().toString()))};
  }

  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, cellEnsembleData->getShape(), std::vector<usize>{1}, pVolFractionsArrayPathValue);
  resultOutputActions.value().actions.push_back(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindVolFractionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{
  auto& cellPhasesArrayRef = dataStructure.getDataRefAs<Int32Array>(filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key));
  auto& volFractionsArrayRef = dataStructure.getDataRefAs<Float32Array>(filterArgs.value<DataPath>(k_VolFractionsArrayPath_Key));

  usize totalPoints = cellPhasesArrayRef.getNumberOfTuples();
  usize totalEnsembles = volFractionsArrayRef.getNumberOfTuples();

  std::vector<usize> ensembleElements(totalEnsembles, 0);
  // Calculate the total number of elements in each Ensemble
  for(usize index = 0; index < totalPoints; index++)
  {
    ensembleElements[cellPhasesArrayRef[index]]++;
  }
  // Calculate the Volume Fraction
  for(usize index = 0; index < totalEnsembles; index++)
  {
    volFractionsArrayRef[index] = static_cast<float32>(ensembleElements[index]) / static_cast<float32>(totalPoints);
  }

  return {};
}
} // namespace complex
