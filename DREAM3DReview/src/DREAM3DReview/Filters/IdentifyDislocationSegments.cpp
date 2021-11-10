#include "IdentifyDislocationSegments.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string IdentifyDislocationSegments::name() const
{
  return FilterTraits<IdentifyDislocationSegments>::name.str();
}

//------------------------------------------------------------------------------
std::string IdentifyDislocationSegments::className() const
{
  return FilterTraits<IdentifyDislocationSegments>::className;
}

//------------------------------------------------------------------------------
Uuid IdentifyDislocationSegments::uuid() const
{
  return FilterTraits<IdentifyDislocationSegments>::uuid;
}

//------------------------------------------------------------------------------
std::string IdentifyDislocationSegments::humanName() const
{
  return "Identify Dislocation Segments";
}

//------------------------------------------------------------------------------
std::vector<std::string> IdentifyDislocationSegments::defaultTags() const
{
  return {"#Unsupported", "#FeatureIdentification"};
}

//------------------------------------------------------------------------------
Parameters IdentifyDislocationSegments::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Edge Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_BurgersVectorsArrayPath_Key, "Burgers Vectors", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SlipPlaneNormalsArrayPath_Key, "Slip Plane Normals", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Edge Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_DislocationIdsArrayName_Key, "Dislocation Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Edge Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_EdgeFeatureAttributeMatrixName_Key, "Edge Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ActiveArrayName_Key, "Active", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer IdentifyDislocationSegments::clone() const
{
  return std::make_unique<IdentifyDislocationSegments>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult IdentifyDislocationSegments::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pBurgersVectorsArrayPathValue = filterArgs.value<DataPath>(k_BurgersVectorsArrayPath_Key);
  auto pSlipPlaneNormalsArrayPathValue = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayPath_Key);
  auto pDislocationIdsArrayNameValue = filterArgs.value<DataPath>(k_DislocationIdsArrayName_Key);
  auto pEdgeFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeFeatureAttributeMatrixName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

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
  auto action = std::make_unique<IdentifyDislocationSegmentsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> IdentifyDislocationSegments::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pBurgersVectorsArrayPathValue = filterArgs.value<DataPath>(k_BurgersVectorsArrayPath_Key);
  auto pSlipPlaneNormalsArrayPathValue = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayPath_Key);
  auto pDislocationIdsArrayNameValue = filterArgs.value<DataPath>(k_DislocationIdsArrayName_Key);
  auto pEdgeFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeFeatureAttributeMatrixName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
