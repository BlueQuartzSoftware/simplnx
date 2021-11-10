#include "ErodeDilateCoordinationNumber.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumber::name() const
{
  return FilterTraits<ErodeDilateCoordinationNumber>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumber::className() const
{
  return FilterTraits<ErodeDilateCoordinationNumber>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateCoordinationNumber::uuid() const
{
  return FilterTraits<ErodeDilateCoordinationNumber>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumber::humanName() const
{
  return "Smooth Bad Data (Coordination Number)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateCoordinationNumber::defaultTags() const
{
  return {"#Processing", "#Cleanup"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateCoordinationNumber::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_CoordinationNumber_Key, "Coordination Number to Consider", "", 1234356));
  params.insert(std::make_unique<BoolParameter>(k_Loop_Key, "Loop Until Gone", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ErodeDilateCoordinationNumber::clone() const
{
  return std::make_unique<ErodeDilateCoordinationNumber>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ErodeDilateCoordinationNumber::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCoordinationNumberValue = filterArgs.value<int32>(k_CoordinationNumber_Key);
  auto pLoopValue = filterArgs.value<bool>(k_Loop_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

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
  auto action = std::make_unique<ErodeDilateCoordinationNumberAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ErodeDilateCoordinationNumber::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCoordinationNumberValue = filterArgs.value<int32>(k_CoordinationNumber_Key);
  auto pLoopValue = filterArgs.value<bool>(k_Loop_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
