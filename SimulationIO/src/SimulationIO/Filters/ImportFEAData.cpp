#include "ImportFEAData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
std::string ImportFEAData::name() const
{
  return FilterTraits<ImportFEAData>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportFEAData::className() const
{
  return FilterTraits<ImportFEAData>::className;
}

//------------------------------------------------------------------------------
Uuid ImportFEAData::uuid() const
{
  return FilterTraits<ImportFEAData>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportFEAData::humanName() const
{
  return "Import FEA Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportFEAData::defaultTags() const
{
  return {"#Unsupported", "#SimulationIO"};
}

//------------------------------------------------------------------------------
Parameters ImportFEAData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_FEAPackage_Key, "FEA package", "", 0, ChoicesParameter::Choices{"ABAQUS", "BSAM", "DEFORM", "DEFORM_POINT_TRACK"}));
  params.insert(std::make_unique<StringParameter>(k_odbName_Key, "odb Name", "", "SomeString"));
  params.insert(std::make_unique<FileSystemPathParameter>(k_odbFilePath_Key, "odb File Path", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_ABQPythonCommand_Key, "ABAQUS Python Command", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_InstanceName_Key, "Instance Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_Step_Key, "Step", "", "SomeString"));
  params.insert(std::make_unique<Int32Parameter>(k_FrameNumber_Key, "Frame Number", "", 1234356));
  params.insert(std::make_unique<FileSystemPathParameter>(k_BSAMInputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<FileSystemPathParameter>(k_DEFORMInputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_DEFORMPointTrackInputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ImportSingleTimeStep_Key, "Read Single Time Step", "", false));
  params.insert(std::make_unique<Int32Parameter>(k_SingleTimeStepValue_Key, "Time Step", "", 1234356));
  params.insertSeparator(Parameters::Separator{""});
  params.insert(std::make_unique<StringParameter>(k_TimeSeriesBundleName_Key, "Time Series Bundle Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_DataContainerName_Key, "Data Container Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_FEAPackage_Key, k_odbName_Key, 0);
  params.linkParameters(k_FEAPackage_Key, k_odbFilePath_Key, 0);
  params.linkParameters(k_FEAPackage_Key, k_ABQPythonCommand_Key, 0);
  params.linkParameters(k_FEAPackage_Key, k_InstanceName_Key, 0);
  params.linkParameters(k_FEAPackage_Key, k_Step_Key, 0);
  params.linkParameters(k_FEAPackage_Key, k_FrameNumber_Key, 0);
  params.linkParameters(k_FEAPackage_Key, k_DEFORMInputFile_Key, 2);
  params.linkParameters(k_FEAPackage_Key, k_BSAMInputFile_Key, 1);
  params.linkParameters(k_FEAPackage_Key, k_DEFORMPointTrackInputFile_Key, 3);
  params.linkParameters(k_FEAPackage_Key, k_ImportSingleTimeStep_Key, 3);
  params.linkParameters(k_FEAPackage_Key, k_SingleTimeStepValue_Key, 3);
  params.linkParameters(k_FEAPackage_Key, k_TimeSeriesBundleName_Key, 3);
  params.linkParameters(k_ImportSingleTimeStep_Key, k_SingleTimeStepValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportFEAData::clone() const
{
  return std::make_unique<ImportFEAData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportFEAData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFEAPackageValue = filterArgs.value<ChoicesParameter::ValueType>(k_FEAPackage_Key);
  auto podbNameValue = filterArgs.value<StringParameter::ValueType>(k_odbName_Key);
  auto podbFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_odbFilePath_Key);
  auto pABQPythonCommandValue = filterArgs.value<StringParameter::ValueType>(k_ABQPythonCommand_Key);
  auto pInstanceNameValue = filterArgs.value<StringParameter::ValueType>(k_InstanceName_Key);
  auto pStepValue = filterArgs.value<StringParameter::ValueType>(k_Step_Key);
  auto pFrameNumberValue = filterArgs.value<int32>(k_FrameNumber_Key);
  auto pBSAMInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_BSAMInputFile_Key);
  auto pDEFORMInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_DEFORMInputFile_Key);
  auto pDEFORMPointTrackInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_DEFORMPointTrackInputFile_Key);
  auto pImportSingleTimeStepValue = filterArgs.value<bool>(k_ImportSingleTimeStep_Key);
  auto pSingleTimeStepValue = filterArgs.value<int32>(k_SingleTimeStepValue_Key);
  auto pTimeSeriesBundleNameValue = filterArgs.value<StringParameter::ValueType>(k_TimeSeriesBundleName_Key);
  auto pDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);

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

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportFEAData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFEAPackageValue = filterArgs.value<ChoicesParameter::ValueType>(k_FEAPackage_Key);
  auto podbNameValue = filterArgs.value<StringParameter::ValueType>(k_odbName_Key);
  auto podbFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_odbFilePath_Key);
  auto pABQPythonCommandValue = filterArgs.value<StringParameter::ValueType>(k_ABQPythonCommand_Key);
  auto pInstanceNameValue = filterArgs.value<StringParameter::ValueType>(k_InstanceName_Key);
  auto pStepValue = filterArgs.value<StringParameter::ValueType>(k_Step_Key);
  auto pFrameNumberValue = filterArgs.value<int32>(k_FrameNumber_Key);
  auto pBSAMInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_BSAMInputFile_Key);
  auto pDEFORMInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_DEFORMInputFile_Key);
  auto pDEFORMPointTrackInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_DEFORMPointTrackInputFile_Key);
  auto pImportSingleTimeStepValue = filterArgs.value<bool>(k_ImportSingleTimeStep_Key);
  auto pSingleTimeStepValue = filterArgs.value<int32>(k_SingleTimeStepValue_Key);
  auto pTimeSeriesBundleNameValue = filterArgs.value<StringParameter::ValueType>(k_TimeSeriesBundleName_Key);
  auto pDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
