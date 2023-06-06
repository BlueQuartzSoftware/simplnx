#include "LosAlamosFFTWriterFilter.hpp"

#include "ComplexCore/Filters/Algorithms/LosAlamosFFTWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string LosAlamosFFTWriterFilter::name() const
{
  return FilterTraits<LosAlamosFFTWriterFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string LosAlamosFFTWriterFilter::className() const
{
  return FilterTraits<LosAlamosFFTWriterFilter>::className;
}

//------------------------------------------------------------------------------
Uuid LosAlamosFFTWriterFilter::uuid() const
{
  return FilterTraits<LosAlamosFFTWriterFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string LosAlamosFFTWriterFilter::humanName() const
{
  return "Export Los Alamos FFT File";
}

//------------------------------------------------------------------------------
std::vector<std::string> LosAlamosFFTWriterFilter::defaultTags() const
{
  return {"IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters LosAlamosFFTWriterFilter::parameters() const
{
  Parameters params;

  /**
   * Please separate the parameters into groups generally of the following:
   *
   * params.insertSeparator(Parameters::Separator{"Input Parameters"});
   * params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Required Input Feature Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
   *
   * .. or create appropriate separators as needed. The UI in COMPLEX no longer
   * does this for the developer by using catgories as in SIMPL
   */

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer LosAlamosFFTWriterFilter::clone() const
{
  return std::make_unique<LosAlamosFFTWriterFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult LosAlamosFFTWriterFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);



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

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> LosAlamosFFTWriterFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{

  LosAlamosFFTWriterInputValues inputValues;

    inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);


  return LosAlamosFFTWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
