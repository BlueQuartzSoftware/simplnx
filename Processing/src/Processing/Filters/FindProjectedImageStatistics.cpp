#include "FindProjectedImageStatistics.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindProjectedImageStatistics::name() const
{
  return FilterTraits<FindProjectedImageStatistics>::name.str();
}

//------------------------------------------------------------------------------
std::string FindProjectedImageStatistics::className() const
{
  return FilterTraits<FindProjectedImageStatistics>::className;
}

//------------------------------------------------------------------------------
Uuid FindProjectedImageStatistics::uuid() const
{
  return FilterTraits<FindProjectedImageStatistics>::uuid;
}

//------------------------------------------------------------------------------
std::string FindProjectedImageStatistics::humanName() const
{
  return "Find Projected Image Statistics";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindProjectedImageStatistics::defaultTags() const
{
  return {"#Processing", "#Image"};
}

//------------------------------------------------------------------------------
Parameters FindProjectedImageStatistics::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Plane_Key, "Plane of Interest", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Quantify", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_ProjectedImageMinArrayName_Key, "Projected Image Min", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ProjectedImageMaxArrayName_Key, "Projected Image Max", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ProjectedImageAvgArrayName_Key, "Projected Image Avg", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ProjectedImageStdArrayName_Key, "Projected Image Std", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ProjectedImageVarArrayName_Key, "Projected Image Var", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindProjectedImageStatistics::clone() const
{
  return std::make_unique<FindProjectedImageStatistics>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindProjectedImageStatistics::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pProjectedImageMinArrayNameValue = filterArgs.value<DataPath>(k_ProjectedImageMinArrayName_Key);
  auto pProjectedImageMaxArrayNameValue = filterArgs.value<DataPath>(k_ProjectedImageMaxArrayName_Key);
  auto pProjectedImageAvgArrayNameValue = filterArgs.value<DataPath>(k_ProjectedImageAvgArrayName_Key);
  auto pProjectedImageStdArrayNameValue = filterArgs.value<DataPath>(k_ProjectedImageStdArrayName_Key);
  auto pProjectedImageVarArrayNameValue = filterArgs.value<DataPath>(k_ProjectedImageVarArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindProjectedImageStatisticsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindProjectedImageStatistics::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pProjectedImageMinArrayNameValue = filterArgs.value<DataPath>(k_ProjectedImageMinArrayName_Key);
  auto pProjectedImageMaxArrayNameValue = filterArgs.value<DataPath>(k_ProjectedImageMaxArrayName_Key);
  auto pProjectedImageAvgArrayNameValue = filterArgs.value<DataPath>(k_ProjectedImageAvgArrayName_Key);
  auto pProjectedImageStdArrayNameValue = filterArgs.value<DataPath>(k_ProjectedImageStdArrayName_Key);
  auto pProjectedImageVarArrayNameValue = filterArgs.value<DataPath>(k_ProjectedImageVarArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
