#include "CreateAbaqusFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DynamicTableFilterParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string CreateAbaqusFile::name() const
{
  return FilterTraits<CreateAbaqusFile>::name.str();
}

std::string CreateAbaqusFile::className() const
{
  return FilterTraits<CreateAbaqusFile>::className;
}

Uuid CreateAbaqusFile::uuid() const
{
  return FilterTraits<CreateAbaqusFile>::uuid;
}

std::string CreateAbaqusFile::humanName() const
{
  return "Create Abaqus File";
}

Parameters CreateAbaqusFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path ", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_OutputFilePrefix_Key, "Output File Prefix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_JobName_Key, "Job Name", "", "SomeString"));
  params.insert(std::make_unique<Int32Parameter>(k_NumDepvar_Key, "Number of Solution Dependent State Variables", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_NumUserOutVar_Key, "Number of User Output Variables", "", 1234356));
  params.insert(std::make_unique<DynamicTableFilterParameter>(k_MatConst_Key, "Material Constants", "", {}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AbqFeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));

  return params;
}

IFilter::UniquePointer CreateAbaqusFile::clone() const
{
  return std::make_unique<CreateAbaqusFile>();
}

Result<OutputActions> CreateAbaqusFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pOutputFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);
  auto pJobNameValue = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  auto pNumDepvarValue = filterArgs.value<int32>(k_NumDepvar_Key);
  auto pNumUserOutVarValue = filterArgs.value<int32>(k_NumUserOutVar_Key);
  auto pMatConstValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MatConst_Key);
  auto pAbqFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_AbqFeatureIdsArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateAbaqusFileAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CreateAbaqusFile::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pOutputFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);
  auto pJobNameValue = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  auto pNumDepvarValue = filterArgs.value<int32>(k_NumDepvar_Key);
  auto pNumUserOutVarValue = filterArgs.value<int32>(k_NumUserOutVar_Key);
  auto pMatConstValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MatConst_Key);
  auto pAbqFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_AbqFeatureIdsArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
