#include "MaskCountDecision.hpp"

#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MaskCountDecision::name() const
{
  return FilterTraits<MaskCountDecision>::name.str();
}

//------------------------------------------------------------------------------
std::string MaskCountDecision::className() const
{
  return FilterTraits<MaskCountDecision>::className;
}

//------------------------------------------------------------------------------
Uuid MaskCountDecision::uuid() const
{
  return FilterTraits<MaskCountDecision>::uuid;
}

//------------------------------------------------------------------------------
std::string MaskCountDecision::humanName() const
{
  return "Mask Count Decision";
}

//------------------------------------------------------------------------------
std::vector<std::string> MaskCountDecision::defaultTags() const
{
  return {"#Core", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters MaskCountDecision::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MaskCountDecision::clone() const
{
  return std::make_unique<MaskCountDecision>();
}

//------------------------------------------------------------------------------
Result<OutputActions> MaskCountDecision::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<MaskCountDecisionAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> MaskCountDecision::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
