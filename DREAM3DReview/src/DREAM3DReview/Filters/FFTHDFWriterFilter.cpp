#include "FFTHDFWriterFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FFTHDFWriterFilter::name() const
{
  return FilterTraits<FFTHDFWriterFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FFTHDFWriterFilter::className() const
{
  return FilterTraits<FFTHDFWriterFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FFTHDFWriterFilter::uuid() const
{
  return FilterTraits<FFTHDFWriterFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FFTHDFWriterFilter::humanName() const
{
  return "Export MASSIF Data (HDF5)";
}

//------------------------------------------------------------------------------
std::vector<std::string> FFTHDFWriterFilter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters FFTHDFWriterFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteEigenstrains_Key, "Write Eigenstrains", "", false));
  params.insert(std::make_unique<FileSystemPathParameter>(k_EigenstrainsOutputFile_Key, "Eigenstrain Output File", "", fs::path("<default file to read goes here>"),
                                                          FileSystemPathParameter::PathType::OutputFile));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEigenstrainsArrayPath_Key, "Eigenstrains", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_WriteEigenstrains_Key, k_EigenstrainsOutputFile_Key, true);
  params.linkParameters(k_WriteEigenstrains_Key, k_CellEigenstrainsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FFTHDFWriterFilter::clone() const
{
  return std::make_unique<FFTHDFWriterFilter>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FFTHDFWriterFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pWriteEigenstrainsValue = filterArgs.value<bool>(k_WriteEigenstrains_Key);
  auto pEigenstrainsOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_EigenstrainsOutputFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCellEigenstrainsArrayPathValue = filterArgs.value<DataPath>(k_CellEigenstrainsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FFTHDFWriterFilterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FFTHDFWriterFilter::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pWriteEigenstrainsValue = filterArgs.value<bool>(k_WriteEigenstrains_Key);
  auto pEigenstrainsOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_EigenstrainsOutputFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCellEigenstrainsArrayPathValue = filterArgs.value<DataPath>(k_CellEigenstrainsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
