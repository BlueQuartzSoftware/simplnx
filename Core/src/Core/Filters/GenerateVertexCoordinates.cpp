#include "GenerateVertexCoordinates.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateVertexCoordinates::name() const
{
  return FilterTraits<GenerateVertexCoordinates>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateVertexCoordinates::className() const
{
  return FilterTraits<GenerateVertexCoordinates>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateVertexCoordinates::uuid() const
{
  return FilterTraits<GenerateVertexCoordinates>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateVertexCoordinates::humanName() const
{
  return "Generate Cell Center Coords";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateVertexCoordinates::defaultTags() const
{
  return {"#Core", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters GenerateVertexCoordinates::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedDataContainerName_Key, "Data Container with Input Geometry", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CoordinateArrayPath_Key, "Created Vertex Coordinates", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateVertexCoordinates::clone() const
{
  return std::make_unique<GenerateVertexCoordinates>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateVertexCoordinates::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedDataContainerNameValue = filterArgs.value<DataPath>(k_SelectedDataContainerName_Key);
  auto pCoordinateArrayPathValue = filterArgs.value<DataPath>(k_CoordinateArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GenerateVertexCoordinatesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> GenerateVertexCoordinates::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedDataContainerNameValue = filterArgs.value<DataPath>(k_SelectedDataContainerName_Key);
  auto pCoordinateArrayPathValue = filterArgs.value<DataPath>(k_CoordinateArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
