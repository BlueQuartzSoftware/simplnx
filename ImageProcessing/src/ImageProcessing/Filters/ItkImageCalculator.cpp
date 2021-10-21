#include "ItkImageCalculator.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkImageCalculator::name() const
{
  return FilterTraits<ItkImageCalculator>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkImageCalculator::className() const
{
  return FilterTraits<ItkImageCalculator>::className;
}

//------------------------------------------------------------------------------
Uuid ItkImageCalculator::uuid() const
{
  return FilterTraits<ItkImageCalculator>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkImageCalculator::humanName() const
{
  return "Image Calculator (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkImageCalculator::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkImageCalculator::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath1_Key, "First Attribute Array to Process", "", DataPath{}));
  params.insert(std::make_unique<ChoicesParameter>(k_Operator_Key, "Operator", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath2_Key, "Second Array to Process", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Output Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkImageCalculator::clone() const
{
  return std::make_unique<ItkImageCalculator>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ItkImageCalculator::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedCellArrayPath1Value = filterArgs.value<DataPath>(k_SelectedCellArrayPath1_Key);
  auto pOperatorValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operator_Key);
  auto pSelectedCellArrayPath2Value = filterArgs.value<DataPath>(k_SelectedCellArrayPath2_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkImageCalculatorAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ItkImageCalculator::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedCellArrayPath1Value = filterArgs.value<DataPath>(k_SelectedCellArrayPath1_Key);
  auto pOperatorValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operator_Key);
  auto pSelectedCellArrayPath2Value = filterArgs.value<DataPath>(k_SelectedCellArrayPath2_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
