#include "PipelineAnnotation.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ParagraphFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string PipelineAnnotation::name() const
{
  return FilterTraits<PipelineAnnotation>::name.str();
}

//------------------------------------------------------------------------------
std::string PipelineAnnotation::className() const
{
  return FilterTraits<PipelineAnnotation>::className;
}

//------------------------------------------------------------------------------
Uuid PipelineAnnotation::uuid() const
{
  return FilterTraits<PipelineAnnotation>::uuid;
}

//------------------------------------------------------------------------------
std::string PipelineAnnotation::humanName() const
{
  return "Pipeline Annotation";
}

//------------------------------------------------------------------------------
std::vector<std::string> PipelineAnnotation::defaultTags() const
{
  return {"#Core", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters PipelineAnnotation::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<ParagraphFilterParameter>(k_Summary_Key, "", "", {}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PipelineAnnotation::clone() const
{
  return std::make_unique<PipelineAnnotation>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PipelineAnnotation::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSummaryValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Summary_Key);

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
  auto action = std::make_unique<PipelineAnnotationAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> PipelineAnnotation::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSummaryValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Summary_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
