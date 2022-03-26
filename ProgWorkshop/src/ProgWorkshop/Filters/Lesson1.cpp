#include "Lesson1.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string Lesson1::name() const
{
  return FilterTraits<Lesson1>::name.str();
}

//------------------------------------------------------------------------------
std::string Lesson1::className() const
{
  return FilterTraits<Lesson1>::className;
}

//------------------------------------------------------------------------------
Uuid Lesson1::uuid() const
{
  return FilterTraits<Lesson1>::uuid;
}

//------------------------------------------------------------------------------
std::string Lesson1::humanName() const
{
  return "Lesson1";
}

//------------------------------------------------------------------------------
std::vector<std::string> Lesson1::defaultTags() const
{
  return {"#Unsupported", "#ProgWorkshop"};
}

//------------------------------------------------------------------------------
Parameters Lesson1::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputDataArrayPath_Key, "InputDataArrayPath", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "OutputDataArrayPath", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_Value_Key, "Value", "", 1.23345f));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer Lesson1::clone() const
{
  return std::make_unique<Lesson1>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult Lesson1::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pValue = filterArgs.value<float32>(k_Value_Key);

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
    // auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::FILL_ME_IN, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS, pOutputDataArrayPathValue);
    // resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> Lesson1::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                              const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pValue = filterArgs.value<float32>(k_Value_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
