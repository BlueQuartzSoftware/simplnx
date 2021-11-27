#include "ArrayCalculator.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
  /*[x]*/ params.insert(std::make_unique<CalculatorFilterParameter>(k_InfixEquation_Key, "Infix Expression", "", {}));
  /*[x]*/ params.insert(std::make_unique<ScalarTypeFilterParameter>(k_ScalarType_Key, "Scalar Type", "", {}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CalculatedArray_Key, "Calculated Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ArrayCalculator::clone() const
{
  return std::make_unique<ArrayCalculator>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ArrayCalculator::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSelectedAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);
  auto pInfixEquationValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InfixEquation_Key);
  auto pScalarTypeValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ScalarType_Key);
  auto pCalculatedArrayValue = filterArgs.value<DataPath>(k_CalculatedArray_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  // This block is commented out because it needs some variables to be filled in.
  {
    // auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::FILL_ME_IN, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS, pCalculatedArrayValue);
    // resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ArrayCalculator::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
