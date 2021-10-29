#include "TriangleDihedralAngleFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string TriangleDihedralAngleFilter::name() const
{
  return FilterTraits<TriangleDihedralAngleFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string TriangleDihedralAngleFilter::className() const
{
  return FilterTraits<TriangleDihedralAngleFilter>::className;
}

//------------------------------------------------------------------------------
Uuid TriangleDihedralAngleFilter::uuid() const
{
  return FilterTraits<TriangleDihedralAngleFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string TriangleDihedralAngleFilter::humanName() const
{
  return "Find Minimum Triangle Dihedral Angle";
}

//------------------------------------------------------------------------------
std::vector<std::string> TriangleDihedralAngleFilter::defaultTags() const
{
  return {"#Surface Meshing", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters TriangleDihedralAngleFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshTriangleDihedralAnglesArrayPath_Key, "Face Dihedral Angles", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer TriangleDihedralAngleFilter::clone() const
{
  return std::make_unique<TriangleDihedralAngleFilter>();
}

//------------------------------------------------------------------------------
Result<OutputActions> TriangleDihedralAngleFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSurfaceMeshTriangleDihedralAnglesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleDihedralAnglesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<TriangleDihedralAngleFilterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> TriangleDihedralAngleFilter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSurfaceMeshTriangleDihedralAnglesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleDihedralAnglesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
