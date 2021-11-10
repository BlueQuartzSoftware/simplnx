#include "NearestPointFuseRegularGrids.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string NearestPointFuseRegularGrids::name() const
{
  return FilterTraits<NearestPointFuseRegularGrids>::name.str();
}

//------------------------------------------------------------------------------
std::string NearestPointFuseRegularGrids::className() const
{
  return FilterTraits<NearestPointFuseRegularGrids>::className;
}

//------------------------------------------------------------------------------
Uuid NearestPointFuseRegularGrids::uuid() const
{
  return FilterTraits<NearestPointFuseRegularGrids>::uuid;
}

//------------------------------------------------------------------------------
std::string NearestPointFuseRegularGrids::humanName() const
{
  return "Fuse Regular Grids (Nearest Point)";
}

//------------------------------------------------------------------------------
std::vector<std::string> NearestPointFuseRegularGrids::defaultTags() const
{
  return {"#Sampling", "#Spacing"};
}

//------------------------------------------------------------------------------
Parameters NearestPointFuseRegularGrids::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_ReferenceCellAttributeMatrixPath_Key, "Reference Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SamplingCellAttributeMatrixPath_Key, "Sampling Cell Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer NearestPointFuseRegularGrids::clone() const
{
  return std::make_unique<NearestPointFuseRegularGrids>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult NearestPointFuseRegularGrids::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pReferenceCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_ReferenceCellAttributeMatrixPath_Key);
  auto pSamplingCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SamplingCellAttributeMatrixPath_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<NearestPointFuseRegularGridsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> NearestPointFuseRegularGrids::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pReferenceCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_ReferenceCellAttributeMatrixPath_Key);
  auto pSamplingCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SamplingCellAttributeMatrixPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
