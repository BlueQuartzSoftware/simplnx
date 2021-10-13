#include "CreateGridMontage.hpp"

#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
std::string CreateGridMontage::name() const
{
  return FilterTraits<CreateGridMontage>::name.str();
}

Uuid CreateGridMontage::uuid() const
{
  return FilterTraits<CreateGridMontage>::uuid;
}

std::string CreateGridMontage::humanName() const
{
  return "Create Grid Montage";
}

Parameters CreateGridMontage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  // Associate the Linkable Parameter(s) to the children parameters that they control

  return params;
}

IFilter::UniquePointer CreateGridMontage::clone() const
{
  return std::make_unique<CreateGridMontage>();
}

Result<OutputActions> CreateGridMontage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateGridMontageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CreateGridMontage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
