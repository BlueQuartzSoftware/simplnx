#include "FindVertexToTriangleDistances.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindVertexToTriangleDistances::name() const
{
  return FilterTraits<FindVertexToTriangleDistances>::name.str();
}

Uuid FindVertexToTriangleDistances::uuid() const
{
  return FilterTraits<FindVertexToTriangleDistances>::uuid;
}

std::string FindVertexToTriangleDistances::humanName() const
{
  return "Find Vertex to Triangle Distances";
}

Parameters FindVertexToTriangleDistances::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_VertexDataContainer_Key, "Source Vertex Geometry", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleDataContainer_Key, "Target Triangle Geometry", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_TriangleNormalsArrayPath_Key, "Triangle Normals", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DistancesArrayPath_Key, "Distances", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ClosestTriangleIdArrayPath_Key, "Closest Triangle Ids", "", DataPath{}));

  return params;
}

IFilter::UniquePointer FindVertexToTriangleDistances::clone() const
{
  return std::make_unique<FindVertexToTriangleDistances>();
}

Result<OutputActions> FindVertexToTriangleDistances::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pVertexDataContainerValue = filterArgs.value<DataPath>(k_VertexDataContainer_Key);
  auto pTriangleDataContainerValue = filterArgs.value<DataPath>(k_TriangleDataContainer_Key);
  auto pTriangleNormalsArrayPathValue = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);
  auto pDistancesArrayPathValue = filterArgs.value<DataPath>(k_DistancesArrayPath_Key);
  auto pClosestTriangleIdArrayPathValue = filterArgs.value<DataPath>(k_ClosestTriangleIdArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindVertexToTriangleDistancesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindVertexToTriangleDistances::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pVertexDataContainerValue = filterArgs.value<DataPath>(k_VertexDataContainer_Key);
  auto pTriangleDataContainerValue = filterArgs.value<DataPath>(k_TriangleDataContainer_Key);
  auto pTriangleNormalsArrayPathValue = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);
  auto pDistancesArrayPathValue = filterArgs.value<DataPath>(k_DistancesArrayPath_Key);
  auto pClosestTriangleIdArrayPathValue = filterArgs.value<DataPath>(k_ClosestTriangleIdArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
