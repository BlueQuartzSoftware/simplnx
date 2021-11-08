#include "ExportLAMMPSFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExportLAMMPSFile::name() const
{
  return FilterTraits<ExportLAMMPSFile>::name.str();
}

//------------------------------------------------------------------------------
std::string ExportLAMMPSFile::className() const
{
  return FilterTraits<ExportLAMMPSFile>::className;
}

//------------------------------------------------------------------------------
Uuid ExportLAMMPSFile::uuid() const
{
  return FilterTraits<ExportLAMMPSFile>::uuid;
}

//------------------------------------------------------------------------------
std::string ExportLAMMPSFile::humanName() const
{
  return "Export LAMMPS Data File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExportLAMMPSFile::defaultTags() const
{
  return {"#Unsupported", "#SimulationIO"};
}

//------------------------------------------------------------------------------
Parameters ExportLAMMPSFile::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_LammpsFile_Key, "LAMMPS File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AtomFeatureLabelsPath_Key, "Atom Feature Labels", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExportLAMMPSFile::clone() const
{
  return std::make_unique<ExportLAMMPSFile>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExportLAMMPSFile::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pLammpsFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_LammpsFile_Key);
  auto pAtomFeatureLabelsPathValue = filterArgs.value<DataPath>(k_AtomFeatureLabelsPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ExportLAMMPSFileAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ExportLAMMPSFile::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pLammpsFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_LammpsFile_Key);
  auto pAtomFeatureLabelsPathValue = filterArgs.value<DataPath>(k_AtomFeatureLabelsPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
