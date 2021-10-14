#include "InputCrystalCompliances.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/Symmetric6x6FilterParameter.hpp"

using namespace complex;

namespace complex
{
std::string InputCrystalCompliances::name() const
{
  return FilterTraits<InputCrystalCompliances>::name.str();
}

Uuid InputCrystalCompliances::uuid() const
{
  return FilterTraits<InputCrystalCompliances>::uuid;
}

std::string InputCrystalCompliances::humanName() const
{
  return "Input Crystal Compliances";
}

Parameters InputCrystalCompliances::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Symmetric6x6FilterParameter>(k_Compliances_Key, "Compliance Values (10^-11 Pa^-1)", "", {}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CrystalCompliancesArrayPath_Key, "Crystal Compliances", "", DataPath{}));

  return params;
}

IFilter::UniquePointer InputCrystalCompliances::clone() const
{
  return std::make_unique<InputCrystalCompliances>();
}

Result<OutputActions> InputCrystalCompliances::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pCompliancesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Compliances_Key);
  auto pCrystalCompliancesArrayPathValue = filterArgs.value<DataPath>(k_CrystalCompliancesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<InputCrystalCompliancesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> InputCrystalCompliances::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCompliancesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Compliances_Key);
  auto pCrystalCompliancesArrayPathValue = filterArgs.value<DataPath>(k_CrystalCompliancesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
