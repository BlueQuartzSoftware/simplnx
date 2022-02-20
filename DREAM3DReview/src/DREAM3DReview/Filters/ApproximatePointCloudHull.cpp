#include "ApproximatePointCloudHull.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ApproximatePointCloudHull::name() const
{
  return FilterTraits<ApproximatePointCloudHull>::name.str();
}

//------------------------------------------------------------------------------
std::string ApproximatePointCloudHull::className() const
{
  return FilterTraits<ApproximatePointCloudHull>::className;
}

//------------------------------------------------------------------------------
Uuid ApproximatePointCloudHull::uuid() const
{
  return FilterTraits<ApproximatePointCloudHull>::uuid;
}

//------------------------------------------------------------------------------
std::string ApproximatePointCloudHull::humanName() const
{
  return "Approximate Point Cloud Hull";
}

//------------------------------------------------------------------------------
std::vector<std::string> ApproximatePointCloudHull::defaultTags() const
{
  return {"#PointCloudFilters", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters ApproximatePointCloudHull::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_GridResolution_Key, "Grid Resolution", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfEmptyNeighbors_Key, "Minimum Number of Emtpy Neighbors", "", 1234356));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_VertexDataContainerName_Key, "Vertex Geometry", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_HullDataContainerName_Key, "Hull Vertex Geoemetry", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ApproximatePointCloudHull::clone() const
{
  return std::make_unique<ApproximatePointCloudHull>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ApproximatePointCloudHull::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  auto pGridResolutionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_GridResolution_Key);
  auto pNumberOfEmptyNeighborsValue = filterArgs.value<int32>(k_NumberOfEmptyNeighbors_Key);
  auto pVertexDataContainerNameValue = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  auto pHullDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_HullDataContainerName_Key);

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
Result<> ApproximatePointCloudHull::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pGridResolutionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_GridResolution_Key);
  auto pNumberOfEmptyNeighborsValue = filterArgs.value<int32>(k_NumberOfEmptyNeighbors_Key);
  auto pVertexDataContainerNameValue = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  auto pHullDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_HullDataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
