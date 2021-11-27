#include "ImportAsciDataArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportAsciDataArray::name() const
{
  return FilterTraits<ImportAsciDataArray>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportAsciDataArray::className() const
{
  return FilterTraits<ImportAsciDataArray>::className;
}

//------------------------------------------------------------------------------
Uuid ImportAsciDataArray::uuid() const
{
  return FilterTraits<ImportAsciDataArray>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportAsciDataArray::humanName() const
{
  return "Import ASCII Attribute Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportAsciDataArray::defaultTags() const
{
  return {"#Core", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportAsciDataArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Scalar Type", "", NumericType::int8));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfComponents_Key, "Number of Components", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_SkipHeaderLines_Key, "Skip Header Lines", "", 1234356));
  params.insert(std::make_unique<ChoicesParameter>(k_Delimiter_Key, "Delimiter", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedAttributeArrayPath_Key, "Output Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportAsciDataArray::clone() const
{
  return std::make_unique<ImportAsciDataArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportAsciDataArray::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pNumberOfComponentsValue = filterArgs.value<int32>(k_NumberOfComponents_Key);
  auto pSkipHeaderLinesValue = filterArgs.value<int32>(k_SkipHeaderLines_Key);
  auto pDelimiterValue = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  auto pCreatedAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);

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
  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string firstLine;

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
    // auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::FILL_ME_IN, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS, pCreatedAttributeArrayPathValue);
    // resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"FirstLine", firstLine});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportAsciDataArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pNumberOfComponentsValue = filterArgs.value<int32>(k_NumberOfComponents_Key);
  auto pSkipHeaderLinesValue = filterArgs.value<int32>(k_SkipHeaderLines_Key);
  auto pDelimiterValue = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  auto pCreatedAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
