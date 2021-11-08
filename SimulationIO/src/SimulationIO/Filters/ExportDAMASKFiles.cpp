#include "ExportDAMASKFiles.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExportDAMASKFiles::name() const
{
  return FilterTraits<ExportDAMASKFiles>::name.str();
}

//------------------------------------------------------------------------------
std::string ExportDAMASKFiles::className() const
{
  return FilterTraits<ExportDAMASKFiles>::className;
}

//------------------------------------------------------------------------------
Uuid ExportDAMASKFiles::uuid() const
{
  return FilterTraits<ExportDAMASKFiles>::uuid;
}

//------------------------------------------------------------------------------
std::string ExportDAMASKFiles::humanName() const
{
  return "Export DAMASK Files";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExportDAMASKFiles::defaultTags() const
{
  return {"#Unsupported", "#SimulationIO"};
}

//------------------------------------------------------------------------------
Parameters ExportDAMASKFiles::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_DataFormat_Key, "Data Format", "", 0, ChoicesParameter::Choices{"pointwise", "grainwise"}));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path ", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_GeometryFileName_Key, "Geometry File Name", "", "SomeString"));
  params.insert(std::make_unique<Int32Parameter>(k_HomogenizationIndex_Key, "Homogenization Index", "", 1234356));
  params.insert(std::make_unique<BoolParameter>(k_CompressGeomFile_Key, "Compress Geom File", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_DataFormat_Key, k_CompressGeomFile_Key, 0);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExportDAMASKFiles::clone() const
{
  return std::make_unique<ExportDAMASKFiles>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExportDAMASKFiles::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pDataFormatValue = filterArgs.value<ChoicesParameter::ValueType>(k_DataFormat_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pGeometryFileNameValue = filterArgs.value<StringParameter::ValueType>(k_GeometryFileName_Key);
  auto pHomogenizationIndexValue = filterArgs.value<int32>(k_HomogenizationIndex_Key);
  auto pCompressGeomFileValue = filterArgs.value<bool>(k_CompressGeomFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ExportDAMASKFilesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ExportDAMASKFiles::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDataFormatValue = filterArgs.value<ChoicesParameter::ValueType>(k_DataFormat_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pGeometryFileNameValue = filterArgs.value<StringParameter::ValueType>(k_GeometryFileName_Key);
  auto pHomogenizationIndexValue = filterArgs.value<int32>(k_HomogenizationIndex_Key);
  auto pCompressGeomFileValue = filterArgs.value<bool>(k_CompressGeomFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
