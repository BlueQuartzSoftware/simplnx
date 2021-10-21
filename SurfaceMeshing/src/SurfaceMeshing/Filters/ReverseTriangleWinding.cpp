#include "ReverseTriangleWinding.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ReverseTriangleWinding::name() const
{
  return FilterTraits<ReverseTriangleWinding>::name.str();
}

//------------------------------------------------------------------------------
std::string ReverseTriangleWinding::className() const
{
  return FilterTraits<ReverseTriangleWinding>::className;
}

//------------------------------------------------------------------------------
Uuid ReverseTriangleWinding::uuid() const
{
  return FilterTraits<ReverseTriangleWinding>::uuid;
}

//------------------------------------------------------------------------------
std::string ReverseTriangleWinding::humanName() const
{
  return "Reverse Triangle Winding";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReverseTriangleWinding::defaultTags() const
{
  return {"#Surface Meshing", "#Connectivity Arrangement"};
}

//------------------------------------------------------------------------------
Parameters ReverseTriangleWinding::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SurfaceDataContainerName_Key, "Data Container", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReverseTriangleWinding::clone() const
{
  return std::make_unique<ReverseTriangleWinding>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ReverseTriangleWinding::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSurfaceDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ReverseTriangleWindingAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ReverseTriangleWinding::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSurfaceDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
