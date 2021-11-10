#include "DiscretizeDDDomain.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string DiscretizeDDDomain::name() const
{
  return FilterTraits<DiscretizeDDDomain>::name.str();
}

//------------------------------------------------------------------------------
std::string DiscretizeDDDomain::className() const
{
  return FilterTraits<DiscretizeDDDomain>::className;
}

//------------------------------------------------------------------------------
Uuid DiscretizeDDDomain::uuid() const
{
  return FilterTraits<DiscretizeDDDomain>::uuid;
}

//------------------------------------------------------------------------------
std::string DiscretizeDDDomain::humanName() const
{
  return "Discretize DDD Domain";
}

//------------------------------------------------------------------------------
std::vector<std::string> DiscretizeDDDomain::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters DiscretizeDDDomain::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_CellSize_Key, "Cell Size (Microns)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_EdgeDataContainerName_Key, "Edge Data Container", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_OutputDataContainerName_Key, "Volume Data Container", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputArrayName_Key, "Dislocation Line Density Array Name", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer DiscretizeDDDomain::clone() const
{
  return std::make_unique<DiscretizeDDDomain>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult DiscretizeDDDomain::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCellSizeValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellSize_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  auto pOutputDataContainerNameValue = filterArgs.value<DataPath>(k_OutputDataContainerName_Key);
  auto pOutputAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputAttributeMatrixName_Key);
  auto pOutputArrayNameValue = filterArgs.value<DataPath>(k_OutputArrayName_Key);

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
  auto action = std::make_unique<DiscretizeDDDomainAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> DiscretizeDDDomain::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCellSizeValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellSize_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  auto pOutputDataContainerNameValue = filterArgs.value<DataPath>(k_OutputDataContainerName_Key);
  auto pOutputAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputAttributeMatrixName_Key);
  auto pOutputArrayNameValue = filterArgs.value<DataPath>(k_OutputArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
