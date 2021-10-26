#include "ReadASCIIData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ReadASCIIDataFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ReadASCIIData::name() const
{
  return FilterTraits<ReadASCIIData>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadASCIIData::className() const
{
  return FilterTraits<ReadASCIIData>::className;
}

//------------------------------------------------------------------------------
Uuid ReadASCIIData::uuid() const
{
  return FilterTraits<ReadASCIIData>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadASCIIData::humanName() const
{
  return "Import ASCII Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadASCIIData::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ReadASCIIData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<ReadASCIIDataFilterParameter>(k_WizardData_Key, "ASCII Wizard Data", "", {}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadASCIIData::clone() const
{
  return std::make_unique<ReadASCIIData>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ReadASCIIData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pWizardDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_WizardData_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ReadASCIIDataAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ReadASCIIData::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pWizardDataValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_WizardData_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
