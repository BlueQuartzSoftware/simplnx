#include "ImportDREAM3DFilter.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/Readers/FileReader.hpp"

#include <nlohmann/json.hpp>

namespace
{
constexpr complex::StringLiteral k_ImportedPipeline = "Imported Pipeline";
constexpr complex::int32 k_NoImportPathError = -1;
constexpr complex::int32 k_FailedOpenFileReaderError = -25;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportDREAM3DFilter::name() const
{
  return FilterTraits<ImportDREAM3DFilter>::name;
}

//------------------------------------------------------------------------------
std::string ImportDREAM3DFilter::className() const
{
  return FilterTraits<ImportDREAM3DFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ImportDREAM3DFilter::uuid() const
{
  return FilterTraits<ImportDREAM3DFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportDREAM3DFilter::humanName() const
{
  return "Read DREAM.3D File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportDREAM3DFilter::defaultTags() const
{
  return {"IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ImportDREAM3DFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Dream3dImportParameter>(k_ImportFileData, "Import File Path", "The HDF5 file path the DataStructure should be imported from.", Dream3dImportParameter::ImportData()));
  return params;
}

//------------------------------------------------------------------------------
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
  complex::HDF5::FileReader fileReader(importData.FilePath);
  if(!fileReader.isValid())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_FailedOpenFileReaderError, "Failed to open the HDF5 file at the specified path."}})};
  }

  OutputActions actions;
  auto action = std::make_unique<ImportH5ObjectPathsAction>(importData.FilePath, importData.DataPaths);
  actions.appendAction(std::move(action));
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
  Result<Pipeline> pipelineResult = DREAM3D::ImportPipelineFromFile(importData.FilePath);
  if(pipelineResult.valid())
  {
    json[k_ImportedPipeline] = pipelineResult.value().toJson();
  }
  return json;
}
} // namespace complex
