#include "ImportDREAM3DFilter.hpp"

#include "nlohmann/json.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/Actions/ImportObjectAction.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

namespace
{
constexpr complex::StringLiteral k_ImportedPipeline = "Imported Pipeline";
}

namespace complex
{
std::string ImportDREAM3DFilter::name() const
{
  return FilterTraits<ImportDREAM3DFilter>::name;
}

std::string ImportDREAM3DFilter::className() const
{
  return FilterTraits<ImportDREAM3DFilter>::className;
}

Uuid ImportDREAM3DFilter::uuid() const
{
  return FilterTraits<ImportDREAM3DFilter>::uuid;
}

std::string ImportDREAM3DFilter::humanName() const
{
  return "Export HDF5 Data Filter";
}

Parameters ImportDREAM3DFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<Dream3dImportParameter>(k_ImportFileData, "Import File Path", "The HDF5 file path the DataStructure should be imported from.", Dream3dImportParameter::ImportData()));
  return params;
}

IFilter::UniquePointer ImportDREAM3DFilter::clone() const
{
  return std::make_unique<ImportDREAM3DFilter>();
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
  for(const auto& dataPath : importPaths)
  {
    createDataAction(importDataStructure, dataPath, actions);
  }
  return {std::move(actions)};
}

Result<OutputActions> ImportDREAM3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto importData = args.value<Dream3dImportParameter::ImportData>(k_ImportFileData);
  if(importData.FilePath.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-1, "Import file path not provided."}})};
  }
  H5::FileReader fileReader(importData.FilePath);
  if(!fileReader.isValid())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-64, "Failed to open the HDF5 file at the specified path."}})};
  }

  // Import DataStructure
  H5::ErrorType errorCode;
  auto [pipeline, importDataStructure] = DREAM3D::ReadFile(fileReader, errorCode);
  if(errorCode < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{errorCode, "Failed to import a DataStructure from the target HDF5 file."}})};
  }

  // Create target DataPaths for the output DataStructure
  auto importDataPaths = importData.DataPaths;
  // Import shortest paths first
  std::sort(importDataPaths.begin(), importDataPaths.end(), [](const DataPath& first, const DataPath& second) { return first.getLength() < second.getLength(); });
  return getDataCreationResults(importDataStructure, importDataPaths);
}

Result<> ImportDREAM3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto importData = args.value<Dream3dImportParameter::ImportData>(k_ImportFileData);
  H5::FileReader fileReader(importData.FilePath);
  if(!fileReader.isValid())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-64, "Failed to open the HDF5 file at the specified path."}})};
  }

  // Import DataStructure
  H5::ErrorType errorCode;
  auto [pipeline, importedDataStructure] = DREAM3D::ReadFile(fileReader, errorCode);
  if(errorCode < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{errorCode, "Failed to import .dream3d file data."}})};
  }

  // Add target DataObjects to the output DataStructure

  return {};
}

nlohmann::json ImportDREAM3DFilter::toJson(const Arguments& args) const
{
  auto json = IFilter::toJson(args);

  auto importData = args.value<Dream3dImportParameter::ImportData>(k_ImportFileData);
  H5::FileReader fileReader(importData.FilePath);
  if(fileReader.isValid())
  {
    H5::ErrorType errorCode;
    auto [importedPipeline, importedDataStructure] = DREAM3D::ReadFile(fileReader, errorCode);
    json[k_ImportedPipeline] = std::move(importedPipeline.toJson());
  }
  return json;
}
} // namespace complex
