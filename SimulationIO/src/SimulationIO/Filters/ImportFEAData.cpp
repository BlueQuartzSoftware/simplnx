#include "ImportFEAData.hpp"

#include "complex/DataStructure/DataPath.hpp"
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
std::string ImportFEAData::name() const
{
  return FilterTraits<ImportFEAData>::name.str();
}

std::string ImportFEAData::className() const
{
  return FilterTraits<ImportFEAData>::className;
}

Uuid ImportFEAData::uuid() const
{
  return FilterTraits<ImportFEAData>::uuid;
}

std::string ImportFEAData::humanName() const
{
  return "Import FEA Data";
}

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
  params.linkParameters(k_FEAPackage_Key, k_odbFilePath_Key, 1);
  params.linkParameters(k_FEAPackage_Key, k_ABQPythonCommand_Key, 2);
  params.linkParameters(k_FEAPackage_Key, k_InstanceName_Key, 3);
  params.linkParameters(k_FEAPackage_Key, k_Step_Key, 4);
  params.linkParameters(k_FEAPackage_Key, k_FrameNumber_Key, 5);
  params.linkParameters(k_FEAPackage_Key, k_DEFORMInputFile_Key, 6);
  params.linkParameters(k_FEAPackage_Key, k_BSAMInputFile_Key, 7);
  params.linkParameters(k_FEAPackage_Key, k_DEFORMPointTrackInputFile_Key, 8);
  params.linkParameters(k_FEAPackage_Key, k_ImportSingleTimeStep_Key, 9);
  params.linkParameters(k_FEAPackage_Key, k_SingleTimeStepValue_Key, 10);
  params.linkParameters(k_FEAPackage_Key, k_TimeSeriesBundleName_Key, 11);
  params.linkParameters(k_ImportSingleTimeStep_Key, k_SingleTimeStepValue_Key, true);

  return params;
}

IFilter::UniquePointer ImportFEAData::clone() const
{
  return std::make_unique<ImportFEAData>();
}

Result<OutputActions> ImportFEAData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
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
  auto pSingleTimeStepValueValue = filterArgs.value<int32>(k_SingleTimeStepValue_Key);
  auto pTimeSeriesBundleNameValue = filterArgs.value<StringParameter::ValueType>(k_TimeSeriesBundleName_Key);
  auto pDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportFEADataAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ImportFEAData::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pSingleTimeStepValueValue = filterArgs.value<int32>(k_SingleTimeStepValue_Key);
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
