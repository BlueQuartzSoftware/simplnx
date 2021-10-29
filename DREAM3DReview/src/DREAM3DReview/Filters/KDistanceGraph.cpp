#include "KDistanceGraph.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string KDistanceGraph::name() const
{
  return FilterTraits<KDistanceGraph>::name.str();
}

//------------------------------------------------------------------------------
std::string KDistanceGraph::className() const
{
  return FilterTraits<KDistanceGraph>::className;
}

//------------------------------------------------------------------------------
Uuid KDistanceGraph::uuid() const
{
  return FilterTraits<KDistanceGraph>::uuid;
}

//------------------------------------------------------------------------------
std::string KDistanceGraph::humanName() const
{
  return "K Distance Graph";
}

//------------------------------------------------------------------------------
std::vector<std::string> KDistanceGraph::defaultTags() const
{
  return {"#DREAM3D Review", "#Clustering"};
}

//------------------------------------------------------------------------------
Parameters KDistanceGraph::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_MinDist_Key, "K<sup>th</sup> Nearest Neighbor", "", 1234356));
  params.insert(std::make_unique<ChoicesParameter>(k_DistanceMetric_Key, "Distance Metric", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Input Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_KDistanceArrayPath_Key, "K Distance", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer KDistanceGraph::clone() const
{
  return std::make_unique<KDistanceGraph>();
}

//------------------------------------------------------------------------------
Result<OutputActions> KDistanceGraph::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMinDistValue = filterArgs.value<int32>(k_MinDist_Key);
  auto pDistanceMetricValue = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pKDistanceArrayPathValue = filterArgs.value<DataPath>(k_KDistanceArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<KDistanceGraphAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> KDistanceGraph::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMinDistValue = filterArgs.value<int32>(k_MinDist_Key);
  auto pDistanceMetricValue = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pKDistanceArrayPathValue = filterArgs.value<DataPath>(k_KDistanceArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
