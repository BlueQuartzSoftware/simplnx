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
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path ", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_GeometryFileName_Key, "Geometry File Name", "", "SomeString"));
  params.insert(std::make_unique<Int32Parameter>(k_HomogenizationIndex_Key, "Homogenization Index", "", 1234356));
  params.insert(std::make_unique<BoolParameter>(k_CompressGeomFile_Key, "Compress Geom File", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Element belongs", DataPath({"CellData", "FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath({"Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
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
IFilter::PreflightResult ExportDAMASKFiles::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pDataFormatValue = filterArgs.value<ChoicesParameter::ValueType>(k_DataFormat_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pGeometryFileNameValue = filterArgs.value<StringParameter::ValueType>(k_GeometryFileName_Key);
  auto pHomogenizationIndexValue = filterArgs.value<int32>(k_HomogenizationIndex_Key);
  auto pCompressGeomFileValue = filterArgs.value<bool>(k_CompressGeomFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
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
Result<> ExportDAMASKFiles::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDataFormatValue = filterArgs.value<ChoicesParameter::ValueType>(k_DataFormat_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pGeometryFileNameValue = filterArgs.value<StringParameter::ValueType>(k_GeometryFileName_Key);
  auto pHomogenizationIndexValue = filterArgs.value<int32>(k_HomogenizationIndex_Key);
  auto pCompressGeomFileValue = filterArgs.value<bool>(k_CompressGeomFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
