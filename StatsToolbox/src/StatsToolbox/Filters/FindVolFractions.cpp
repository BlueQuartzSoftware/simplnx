#include "FindVolFractions.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindVolFractions::name() const
{
  return FilterTraits<FindVolFractions>::name.str();
}

std::string FindVolFractions::className() const
{
  return FilterTraits<FindVolFractions>::className;
}

Uuid FindVolFractions::uuid() const
{
  return FilterTraits<FindVolFractions>::uuid;
}

std::string FindVolFractions::humanName() const
{
  return "Find Volume Fractions of Ensembles";
}

Parameters FindVolFractions::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_VolFractionsArrayPath_Key, "Volume Fractions", "", DataPath{}));

  return params;
}

IFilter::UniquePointer FindVolFractions::clone() const
{
  return std::make_unique<FindVolFractions>();
}

Result<OutputActions> FindVolFractions::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pVolFractionsArrayPathValue = filterArgs.value<DataPath>(k_VolFractionsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindVolFractionsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindVolFractions::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pVolFractionsArrayPathValue = filterArgs.value<DataPath>(k_VolFractionsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
