#include "ClearDataMask.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ClearDataMask::name() const
{
  return FilterTraits<ClearDataMask>::name.str();
}

//------------------------------------------------------------------------------
std::string ClearDataMask::className() const
{
  return FilterTraits<ClearDataMask>::className;
}

//------------------------------------------------------------------------------
Uuid ClearDataMask::uuid() const
{
  return FilterTraits<ClearDataMask>::uuid;
}

//------------------------------------------------------------------------------
std::string ClearDataMask::humanName() const
{
  return "Clear Data (Mask)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ClearDataMask::defaultTags() const
{
  return {"#Processing", "#Cleanup"};
}

//------------------------------------------------------------------------------
Parameters ClearDataMask::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ClearDataMask::clone() const
{
  return std::make_unique<ClearDataMask>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ClearDataMask::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ClearDataMaskAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ClearDataMask::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
