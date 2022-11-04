#include "FindEuclideanDistMapFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include "Core/Filters/Algorithms/FindEuclideanDistMap.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindEuclideanDistMapFilter::name() const
{
  return FilterTraits<FindEuclideanDistMapFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindEuclideanDistMapFilter::className() const
{
  return FilterTraits<FindEuclideanDistMapFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindEuclideanDistMapFilter::uuid() const
{
  return FilterTraits<FindEuclideanDistMapFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindEuclideanDistMapFilter::humanName() const
{
  return "Find Euclidean Distance Map";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindEuclideanDistMapFilter::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindEuclideanDistMapFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<BoolParameter>(k_CalcManhattanDist_Key, "Output arrays are Manhattan distance (int32)",
                                                "If Manhattan distance is used then results are stored as int32 otherwise results are stored as float32", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DoBoundaries_Key, "Calculate Distance to Boundaries", "", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DoTripleLines_Key, "Calculate Distance to Triple Lines", "", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DoQuadPoints_Key, "Calculate Distance to Quadruple Points", "", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveNearestNeighbors_Key, "Store the Nearest Boundary Cells", "", false));

  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath({"CellData", "FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::ComponentTypes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_GBDistancesArrayName_Key, "Boundary Distances", "", "GBManhattanDistances"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_TJDistancesArrayName_Key, "Triple Line Distances", "", "TJManhattanDistances"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_QPDistancesArrayName_Key, "Quadruple Point Distances", "", "QPManhattanDistances"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NearestNeighborsArrayName_Key, "Nearest Boundary Cells", "", "NearestNeighbors"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_DoBoundaries_Key, k_GBDistancesArrayName_Key, std::make_any<bool>(true));
  params.linkParameters(k_DoTripleLines_Key, k_TJDistancesArrayName_Key, std::make_any<bool>(true));
  params.linkParameters(k_DoQuadPoints_Key, k_QPDistancesArrayName_Key, std::make_any<bool>(true));
  params.linkParameters(k_SaveNearestNeighbors_Key, k_NearestNeighborsArrayName_Key, std::make_any<bool>(true));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindEuclideanDistMapFilter::clone() const
{
  return std::make_unique<FindEuclideanDistMapFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindEuclideanDistMapFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pCalcManhattanDistValue = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  auto pDoBoundariesValue = filterArgs.value<bool>(k_DoBoundaries_Key);
  auto pDoTripleLinesValue = filterArgs.value<bool>(k_DoTripleLines_Key);
  auto pDoQuadPointsValue = filterArgs.value<bool>(k_DoQuadPoints_Key);
  auto pSaveNearestNeighborsValue = filterArgs.value<bool>(k_SaveNearestNeighbors_Key);

  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  DataPath parentGroup = pFeatureIdsArrayPathValue.getParent();

  // Interesting thing about this parameter: The Default VALUE must be at the root level of the Data Structure. This is because the
  // user may not actually want to keep that created array in which case we then try to delete the array. The DeleteDataAction
  // will fail in preflight because the Array was never actually created at its default location and so if fails. If the user
  // does in fact want to keep this array, then the user would have actually set the DataPaths to something that will actually get created.
  auto pNearestNeighborsArrayNameValue = filterArgs.value<std::string>(k_NearestNeighborsArrayName_Key);
  DataPath pNearestNeighborsArrayPath = pSaveNearestNeighborsValue ? parentGroup.createChildPath(pNearestNeighborsArrayNameValue) : DataPath::FromString(pNearestNeighborsArrayNameValue).value();

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // Get the Cell Data Array so we get the tuple shape correct
  const auto* cellDataArray = dataStructure.getDataAs<Int32Array>(pFeatureIdsArrayPathValue);
  if(nullptr == cellDataArray)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12801, fmt::format("{} Data Array is not of the correct type. Select a DataArray object.", pFeatureIdsArrayPathValue.toString())}})};
  }

  IDataStore::ShapeType tupleShape = cellDataArray->getIDataStore()->getTupleShape();
  DataType outputDataType = DataType::int32;
  if(!pCalcManhattanDistValue)
  {
    outputDataType = DataType::float32;
  }

  // Create the GBDistancesArray
  {
    auto arrayPath = parentGroup.createChildPath(filterArgs.value<std::string>(k_GBDistancesArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(outputDataType, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().actions.push_back(std::move(action));
  }
  // Create the TJDistancesArray
  {
    auto arrayPath = parentGroup.createChildPath(filterArgs.value<std::string>(k_TJDistancesArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(outputDataType, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().actions.push_back(std::move(action));
  }
  // Create the QPDistancesArray
  {
    auto arrayPath = parentGroup.createChildPath(filterArgs.value<std::string>(k_QPDistancesArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(outputDataType, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().actions.push_back(std::move(action));
  }
  // Create the NearestNeighborsArray
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, tupleShape, std::vector<usize>{3ULL}, pNearestNeighborsArrayPath);
    resultOutputActions.value().actions.push_back(std::move(action));
  }

  // If we are NOT saving the nearest neighbors then we need to delete this array that gets created.
  if(!pSaveNearestNeighborsValue)
  {
    auto action = std::make_unique<DeleteDataAction>(pNearestNeighborsArrayPath);
    resultOutputActions.value().deferredActions.push_back(std::move(action));
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
Result<> FindEuclideanDistMapFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  FindEuclideanDistMapInputValues inputValues;

  inputValues.CalcManhattanDist = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  inputValues.DoBoundaries = filterArgs.value<bool>(k_DoBoundaries_Key);
  inputValues.DoTripleLines = filterArgs.value<bool>(k_DoTripleLines_Key);
  inputValues.DoQuadPoints = filterArgs.value<bool>(k_DoQuadPoints_Key);
  inputValues.SaveNearestNeighbors = filterArgs.value<bool>(k_SaveNearestNeighbors_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  DataPath parentGroupPath = inputValues.FeatureIdsArrayPath.getParent();
  inputValues.GBDistancesArrayName = parentGroupPath.createChildPath(filterArgs.value<std::string>(k_GBDistancesArrayName_Key));
  inputValues.TJDistancesArrayName = parentGroupPath.createChildPath(filterArgs.value<std::string>(k_TJDistancesArrayName_Key));
  inputValues.QPDistancesArrayName = parentGroupPath.createChildPath(filterArgs.value<std::string>(k_QPDistancesArrayName_Key));
  auto nearestNeighborName = filterArgs.value<std::string>(k_NearestNeighborsArrayName_Key);
  inputValues.NearestNeighborsArrayName = inputValues.SaveNearestNeighbors ? parentGroupPath.createChildPath(nearestNeighborName) : DataPath::FromString(nearestNeighborName).value();
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return FindEuclideanDistMap(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
