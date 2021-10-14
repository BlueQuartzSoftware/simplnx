#include "ExportMultiOnScaleTableFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/PreflightUpdatedValueFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string ExportMultiOnScaleTableFile::name() const
{
  return FilterTraits<ExportMultiOnScaleTableFile>::name.str();
}

std::string ExportMultiOnScaleTableFile::className() const
{
  return FilterTraits<ExportMultiOnScaleTableFile>::className;
}

Uuid ExportMultiOnScaleTableFile::uuid() const
{
  return FilterTraits<ExportMultiOnScaleTableFile>::uuid;
}

std::string ExportMultiOnScaleTableFile::humanName() const
{
  return "Create Multi OnScale Table File";
}

Parameters ExportMultiOnScaleTableFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path ", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_DataContainerPrefix_Key, "Data Container Prefix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MatrixName_Key, "Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_ArrayName_Key, "Array Name", "", "SomeString"));
  params.insert(std::make_unique<VectorInt32Parameter>(k_NumKeypoints_Key, "Number of Keypoints", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_SelectedArrays_Key, "Selected Arrays", "", {}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PhaseNamesArrayPath_Key, "Phase Names", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ExportMultiOnScaleTableFile::clone() const
{
  return std::make_unique<ExportMultiOnScaleTableFile>();
}

Result<OutputActions> ExportMultiOnScaleTableFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pDataContainerPrefixValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  auto pMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_MatrixName_Key);
  auto pArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ArrayName_Key);
  auto pNumKeypointsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumKeypoints_Key);
  auto pSelectedArraysValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedArrays_Key);
  auto pPhaseNamesArrayPathValue = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ExportMultiOnScaleTableFileAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ExportMultiOnScaleTableFile::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pDataContainerPrefixValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  auto pMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_MatrixName_Key);
  auto pArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ArrayName_Key);
  auto pNumKeypointsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumKeypoints_Key);
  auto pSelectedArraysValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedArrays_Key);
  auto pPhaseNamesArrayPathValue = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
