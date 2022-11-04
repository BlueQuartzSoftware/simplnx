#include "FindNeighborhoodsFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "Core/Filters/Algorithms/FindNeighborhoods.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindNeighborhoodsFilter::name() const
{
  return FilterTraits<FindNeighborhoodsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindNeighborhoodsFilter::className() const
{
  return FilterTraits<FindNeighborhoodsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindNeighborhoodsFilter::uuid() const
{
  return FilterTraits<FindNeighborhoodsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindNeighborhoodsFilter::humanName() const
{
  return "Find Feature Neighborhoods";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindNeighborhoodsFilter::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindNeighborhoodsFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<Float32Parameter>(k_MultiplesOfAverage_Key, "Multiples of Average Diameter", "", 1.0F));

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "", DataPath({"Data Container"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Required Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_EquivalentDiametersArrayPath_Key, "Equivalent Diameters", "", DataPath({"CellFeatureData", "EquivalentDiameters"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::ComponentTypes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath({"CellFeatureData", "Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::ComponentTypes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "", DataPath({"CellFeatureData", "Centroids"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::ComponentTypes{{3}}));
  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NeighborhoodsArrayName_Key, "Neighborhoods", "", DataPath({"CellFeatureData", "Neighborhoods"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NeighborhoodListArrayName_Key, "NeighborhoodList", "", DataPath({"CellFeatureData", "NeighborhoodList"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindNeighborhoodsFilter::clone() const
{
  return std::make_unique<FindNeighborhoodsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindNeighborhoodsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pMultiplesOfAverageValue = filterArgs.value<float32>(k_MultiplesOfAverage_Key);
  auto pEquivalentDiametersArrayPathValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pNeighborhoodsArrayNameValue = filterArgs.value<DataPath>(k_NeighborhoodsArrayName_Key);
  auto pNeighborhoodListArrayNameValue = filterArgs.value<DataPath>(k_NeighborhoodListArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // Get the Feature Phases Array and get its TupleShape
  const auto* featurePhases = dataStructure.getDataAs<Int32Array>(pFeaturePhasesArrayPathValue);
  if(nullptr == featurePhases)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12801, "Feature Phases Data Array is not of the correct type"}})};
  }

  IDataStore::ShapeType tupleShape = featurePhases->getIDataStore()->getTupleShape();
  // Create the Neighborhoods Array in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, tupleShape, std::vector<usize>{1ULL}, pNeighborhoodsArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(action));
  }
  // Create the NeighborList Output NeighborList in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::int32, featurePhases->getNumberOfTuples(), pNeighborhoodListArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(action));
  }

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindNeighborhoodsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  FindNeighborhoodsInputValues inputValues;

  inputValues.MultiplesOfAverage = filterArgs.value<float32>(k_MultiplesOfAverage_Key);
  inputValues.EquivalentDiametersArrayPath = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.NeighborhoodsArrayName = filterArgs.value<DataPath>(k_NeighborhoodsArrayName_Key);
  inputValues.NeighborhoodListArrayName = filterArgs.value<DataPath>(k_NeighborhoodListArrayName_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return FindNeighborhoods(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
