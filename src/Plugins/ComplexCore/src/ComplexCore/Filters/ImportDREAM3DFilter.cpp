#include "ImportDREAM3DFilter.hpp"

#include "nlohmann/json.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

namespace
{
constexpr complex::StringLiteral k_ImportedPipeline = "Imported Pipeline";
constexpr complex::int32 k_NoImportPathError = -1;
constexpr complex::int32 k_FailedOpenFileReaderError = -25;
constexpr complex::int32 k_NoSelectedPaths = -26;
} // namespace

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
  return "Read DREAM.3D File (v8)";
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

IFilter::PreflightResult ImportDREAM3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto importData = args.value<Dream3dImportParameter::ImportData>(k_ImportFileData);
  if(importData.FilePath.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_NoImportPathError, "Import file path not provided."}})};
  }
  H5::FileReader fileReader(importData.FilePath);
  if(!fileReader.isValid())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_FailedOpenFileReaderError, "Failed to open the HDF5 file at the specified path."}})};
  }

  // Require at least one DataPath to import
  // NX does not allow the use of this value as intended.
  if(importData.DataPaths.has_value() && importData.DataPaths->empty())
  {
    importData.DataPaths = std::nullopt;
  }

  OutputActions actions;
  auto action = std::make_unique<ImportH5ObjectPathsAction>(fileReader, importData.DataPaths);
  actions.actions.push_back(std::move(action));
  return {std::move(actions)};
}

Result<> ImportDREAM3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
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
