#include "ApproximatePointCloudHull.hpp"

#include "complex/DataStructure/DataPath.hpp"
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
IFilter::PreflightResult ApproximatePointCloudHull::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pGridResolutionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_GridResolution_Key);
  auto pNumberOfEmptyNeighborsValue = filterArgs.value<int32>(k_NumberOfEmptyNeighbors_Key);
  auto pVertexDataContainerNameValue = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  auto pHullDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_HullDataContainerName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ApproximatePointCloudHullAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ApproximatePointCloudHull::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
