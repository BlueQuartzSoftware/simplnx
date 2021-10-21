#include "ArrayCalculator.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/CalculatorFilterParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/ScalarTypeFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ArrayCalculator::name() const
{
  return FilterTraits<ArrayCalculator>::name.str();
}

//------------------------------------------------------------------------------
std::string ArrayCalculator::className() const
{
  return FilterTraits<ArrayCalculator>::className;
}

//------------------------------------------------------------------------------
Uuid ArrayCalculator::uuid() const
{
  return FilterTraits<ArrayCalculator>::uuid;
}

//------------------------------------------------------------------------------
std::string ArrayCalculator::humanName() const
{
  return "Attribute Array Calculator";
}

//------------------------------------------------------------------------------
std::vector<std::string> ArrayCalculator::defaultTags() const
{
  return {"#Core", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters ArrayCalculator::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedAttributeMatrix_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<CalculatorFilterParameter>(k_InfixEquation_Key, "Infix Expression", "", {}));
  params.insert(std::make_unique<ScalarTypeFilterParameter>(k_ScalarType_Key, "Scalar Type", "", {}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CalculatedArray_Key, "Calculated Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ArrayCalculator::clone() const
{
  return std::make_unique<ArrayCalculator>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ArrayCalculator::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);
  auto pInfixEquationValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InfixEquation_Key);
  auto pScalarTypeValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ScalarType_Key);
  auto pCalculatedArrayValue = filterArgs.value<DataPath>(k_CalculatedArray_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ArrayCalculatorAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ArrayCalculator::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);
  auto pInfixEquationValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InfixEquation_Key);
  auto pScalarTypeValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ScalarType_Key);
  auto pCalculatedArrayValue = filterArgs.value<DataPath>(k_CalculatedArray_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
