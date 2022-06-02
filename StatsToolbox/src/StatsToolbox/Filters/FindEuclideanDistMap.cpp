#include "FindEuclideanDistMap.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindEuclideanDistMap::name() const
{
  return FilterTraits<FindEuclideanDistMap>::name.str();
}

//------------------------------------------------------------------------------
std::string FindEuclideanDistMap::className() const
{
  return FilterTraits<FindEuclideanDistMap>::className;
}

//------------------------------------------------------------------------------
Uuid FindEuclideanDistMap::uuid() const
{
  return FilterTraits<FindEuclideanDistMap>::uuid;
}

//------------------------------------------------------------------------------
std::string FindEuclideanDistMap::humanName() const
{
  return "Find Euclidean Distance Map";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindEuclideanDistMap::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindEuclideanDistMap::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_CalcManhattanDist_Key, "Calculate Manhattan Distance", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DoBoundaries_Key, "Calculate Distance to Boundaries", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DoTripleLines_Key, "Calculate Distance to Triple Lines", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DoQuadPoints_Key, "Calculate Distance to Quadruple Points", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveNearestNeighbors_Key, "Store the Nearest Boundary Cells", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath({"FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_GBDistancesArrayName_Key, "Boundary Distances", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_TJDistancesArrayName_Key, "Triple Line Distances", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_QPDistancesArrayName_Key, "Quadruple Point Distances", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NearestNeighborsArrayName_Key, "Nearest Neighbors", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_DoBoundaries_Key, k_GBDistancesArrayName_Key, true);
  params.linkParameters(k_DoTripleLines_Key, k_TJDistancesArrayName_Key, true);
  params.linkParameters(k_DoQuadPoints_Key, k_QPDistancesArrayName_Key, true);
  params.linkParameters(k_SaveNearestNeighbors_Key, k_NearestNeighborsArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindEuclideanDistMap::clone() const
{
  return std::make_unique<FindEuclideanDistMap>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindEuclideanDistMap::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  auto pCalcManhattanDistValue = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  auto pDoBoundariesValue = filterArgs.value<bool>(k_DoBoundaries_Key);
  auto pDoTripleLinesValue = filterArgs.value<bool>(k_DoTripleLines_Key);
  auto pDoQuadPointsValue = filterArgs.value<bool>(k_DoQuadPoints_Key);
  auto pSaveNearestNeighborsValue = filterArgs.value<bool>(k_SaveNearestNeighbors_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pGBDistancesArrayNameValue = filterArgs.value<DataPath>(k_GBDistancesArrayName_Key);
  auto pTJDistancesArrayNameValue = filterArgs.value<DataPath>(k_TJDistancesArrayName_Key);
  auto pQPDistancesArrayNameValue = filterArgs.value<DataPath>(k_QPDistancesArrayName_Key);
  auto pNearestNeighborsArrayNameValue = filterArgs.value<DataPath>(k_NearestNeighborsArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindEuclideanDistMap::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCalcManhattanDistValue = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  auto pDoBoundariesValue = filterArgs.value<bool>(k_DoBoundaries_Key);
  auto pDoTripleLinesValue = filterArgs.value<bool>(k_DoTripleLines_Key);
  auto pDoQuadPointsValue = filterArgs.value<bool>(k_DoQuadPoints_Key);
  auto pSaveNearestNeighborsValue = filterArgs.value<bool>(k_SaveNearestNeighbors_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pGBDistancesArrayNameValue = filterArgs.value<DataPath>(k_GBDistancesArrayName_Key);
  auto pTJDistancesArrayNameValue = filterArgs.value<DataPath>(k_TJDistancesArrayName_Key);
  auto pQPDistancesArrayNameValue = filterArgs.value<DataPath>(k_QPDistancesArrayName_Key);
  auto pNearestNeighborsArrayNameValue = filterArgs.value<DataPath>(k_NearestNeighborsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
