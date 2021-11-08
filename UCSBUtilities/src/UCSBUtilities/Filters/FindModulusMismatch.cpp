#include "FindModulusMismatch.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindModulusMismatch::name() const
{
  return FilterTraits<FindModulusMismatch>::name.str();
}

//------------------------------------------------------------------------------
std::string FindModulusMismatch::className() const
{
  return FilterTraits<FindModulusMismatch>::className;
}

//------------------------------------------------------------------------------
Uuid FindModulusMismatch::uuid() const
{
  return FilterTraits<FindModulusMismatch>::uuid;
}

//------------------------------------------------------------------------------
std::string FindModulusMismatch::humanName() const
{
  return "Find Elastic Modulus Mismatch";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindModulusMismatch::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindModulusMismatch::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_ModuliArrayPath_Key, "Moduli", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "SurfaceMeshFaceLabels", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshDeltaModulusArrayName_Key, "SurfaceMeshDeltaModulus", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindModulusMismatch::clone() const
{
  return std::make_unique<FindModulusMismatch>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindModulusMismatch::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pModuliArrayPathValue = filterArgs.value<DataPath>(k_ModuliArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshDeltaModulusArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshDeltaModulusArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindModulusMismatchAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindModulusMismatch::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pModuliArrayPathValue = filterArgs.value<DataPath>(k_ModuliArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshDeltaModulusArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshDeltaModulusArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
