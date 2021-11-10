#include "InputCrystalCompliances.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/Symmetric6x6FilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string InputCrystalCompliances::name() const
{
  return FilterTraits<InputCrystalCompliances>::name.str();
}

//------------------------------------------------------------------------------
std::string InputCrystalCompliances::className() const
{
  return FilterTraits<InputCrystalCompliances>::className;
}

//------------------------------------------------------------------------------
Uuid InputCrystalCompliances::uuid() const
{
  return FilterTraits<InputCrystalCompliances>::uuid;
}

//------------------------------------------------------------------------------
std::string InputCrystalCompliances::humanName() const
{
  return "Input Crystal Compliances";
}

//------------------------------------------------------------------------------
std::vector<std::string> InputCrystalCompliances::defaultTags() const
{
  return {"#Generic", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters InputCrystalCompliances::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<Symmetric6x6FilterParameter>(k_Compliances_Key, "Compliance Values (10^-11 Pa^-1)", "", {}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CrystalCompliancesArrayPath_Key, "Crystal Compliances", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InputCrystalCompliances::clone() const
{
  return std::make_unique<InputCrystalCompliances>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InputCrystalCompliances::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCompliancesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Compliances_Key);
  auto pCrystalCompliancesArrayPathValue = filterArgs.value<DataPath>(k_CrystalCompliancesArrayPath_Key);

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
  auto action = std::make_unique<InputCrystalCompliancesAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> InputCrystalCompliances::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
