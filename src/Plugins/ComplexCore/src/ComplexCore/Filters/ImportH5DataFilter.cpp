#include "ImportH5DataFilter.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/Actions/ImportObjectAction.hpp"
#include "complex/Parameters/H5DataStructureImportParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

namespace complex
{
std::string ImportH5DataFilter::name() const
{
  return FilterTraits<ImportH5DataFilter>::name;
}

std::string ImportH5DataFilter::className() const
{
  return FilterTraits<ImportH5DataFilter>::className;
}

Uuid ImportH5DataFilter::uuid() const
{
  return FilterTraits<ImportH5DataFilter>::uuid;
}

std::string ImportH5DataFilter::humanName() const
{
  return "Export HDF5 Data Filter";
}

Parameters ImportH5DataFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<H5DataStructureImportParameter>(k_ImportFileData, "Import File Path", "The HDF5 file path the DataStructure should be imported from.",
                                                                 H5DataStructureImportParameter::ImportData()));
  return params;
}

IFilter::UniquePointer ImportH5DataFilter::clone() const
{
  return std::make_unique<ImportH5DataFilter>();
}

void createDataAction(const DataStructure& dataStructure, const DataPath& targetPath, OutputActions& actions)
{
  auto dataObject = dataStructure.getSharedData(targetPath);
  if(dataObject == nullptr)
  {
    return;
  }

  auto action = std::make_unique<ImportObjectAction>(dataObject, targetPath);
  actions.actions.push_back(std::move(action));
}

Result<OutputActions> getDataCreationResults(const DataStructure& importDataStructure, const nonstd::span<DataPath>& importPaths)
{
  OutputActions actions;
  for(const auto dataPath : importPaths)
  {
    createDataAction(importDataStructure, dataPath, actions);
  }
  return {std::move(actions)};
}

Result<OutputActions> ImportH5DataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto h5ImportData = args.value<H5DataStructureImportParameter::ImportData>(k_ImportFileData);
  if(h5ImportData.FilePath.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-1, "Import file path not provided."}})};
  }
  H5::FileReader fileReader(h5ImportData.FilePath);
  if(!fileReader.isValid())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-64, "Failed to open the HDF5 file at the specified path."}})};
  }

  // Import DataStructure
  H5::DataStructureReader dataReader;
  H5::ErrorType errorCode;
  auto importDataStructure = dataReader.readH5Group(fileReader, errorCode);
  if(errorCode < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{errorCode, "Failed to import a DataStructure from the target HDF5 file."}})};
  }

  // Create target DataPaths for the output DataStructure
  return getDataCreationResults(importDataStructure, h5ImportData.DataPaths);
}

Result<> ImportH5DataFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto h5ImportData = args.value<H5DataStructureImportParameter::ImportData>(k_ImportFileData);
  H5::FileReader fileReader(h5ImportData.FilePath);
  if(!fileReader.isValid())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-64, "Failed to open the HDF5 file at the specified path."}})};
  }

  // Import DataStructure
  H5::DataStructureReader dataReader;
  H5::ErrorType errorCode;
  auto importDataStructure = dataReader.readH5Group(fileReader, errorCode);
  if(errorCode < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{errorCode, "Failed to import a DataStructure from the target HDF5 file."}})};
  }

  // Add target DataObjects to the output DataStructure

  return {};
}
} // namespace complex
