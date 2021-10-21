#include "TriangleCentroidFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string TriangleCentroidFilter::name() const
{
  return FilterTraits<TriangleCentroidFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string TriangleCentroidFilter::className() const
{
  return FilterTraits<TriangleCentroidFilter>::className;
}

//------------------------------------------------------------------------------
Uuid TriangleCentroidFilter::uuid() const
{
  return FilterTraits<TriangleCentroidFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string TriangleCentroidFilter::humanName() const
{
  return "Generate Triangle Centroids";
}

//------------------------------------------------------------------------------
std::vector<std::string> TriangleCentroidFilter::defaultTags() const
{
  return {"#Surface Meshing", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters TriangleCentroidFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshTriangleCentroidsArrayPath_Key, "Face Centroids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer TriangleCentroidFilter::clone() const
{
  return std::make_unique<TriangleCentroidFilter>();
}

//------------------------------------------------------------------------------
Result<OutputActions> TriangleCentroidFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSurfaceMeshTriangleCentroidsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleCentroidsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<TriangleCentroidFilterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> TriangleCentroidFilter::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSurfaceMeshTriangleCentroidsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleCentroidsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
