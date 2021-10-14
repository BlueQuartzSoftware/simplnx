#include "FindNorm.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindNorm::name() const
{
  return FilterTraits<FindNorm>::name.str();
}

std::string FindNorm::className() const
{
  return FilterTraits<FindNorm>::className;
}

Uuid FindNorm::uuid() const
{
  return FilterTraits<FindNorm>::uuid;
}

std::string FindNorm::humanName() const
{
  return "Find Norm";
}

Parameters FindNorm::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_PSpace_Key, "p-Space Value", "", 1.23345f));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Input Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NormArrayPath_Key, "Norm", "", DataPath{}));

  return params;
}

IFilter::UniquePointer FindNorm::clone() const
{
  return std::make_unique<FindNorm>();
}

Result<OutputActions> FindNorm::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPSpaceValue = filterArgs.value<float32>(k_PSpace_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNormArrayPathValue = filterArgs.value<DataPath>(k_NormArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindNormAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindNorm::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPSpaceValue = filterArgs.value<float32>(k_PSpace_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNormArrayPathValue = filterArgs.value<DataPath>(k_NormArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
