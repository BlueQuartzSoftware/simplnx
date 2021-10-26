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
Result<OutputActions> PipelineAnnotation::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSummaryValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Summary_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<PipelineAnnotationAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> PipelineAnnotation::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
