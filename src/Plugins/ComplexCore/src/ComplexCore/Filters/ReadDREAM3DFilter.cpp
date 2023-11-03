#include "ReadDREAM3DFilter.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/Readers/FileReader.hpp"

#include "complex/Utilities/SIMPLConversion.hpp"

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
std::string ReadDREAM3DFilter::name() const
{
  return FilterTraits<ReadDREAM3DFilter>::name;
}

//------------------------------------------------------------------------------
std::string ReadDREAM3DFilter::className() const
{
  return FilterTraits<ReadDREAM3DFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadDREAM3DFilter::uuid() const
{
  return FilterTraits<ReadDREAM3DFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadDREAM3DFilter::humanName() const
{
  return "Read DREAM.3D File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadDREAM3DFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadDREAM3DFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Dream3dImportParameter>(k_ImportFileData, "Import File Path", "The HDF5 file path the DataStructure should be imported from.", Dream3dImportParameter::ImportData()));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadDREAM3DFilter::clone() const
{
  return std::make_unique<ReadDREAM3DFilter>();
}

IFilter::PreflightResult ReadDREAM3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
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

Result<> ReadDREAM3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  return {};
}

nlohmann::json ReadDREAM3DFilter::toJson(const Arguments& args) const
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

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OverwriteExistingDataContainersKey = "OverwriteExistingDataContainers";
constexpr StringLiteral k_InputFileDataContainerArrayProxyKey = "InputFileDataContainerArrayProxy";
} // namespace SIMPL
} // namespace

Result<Arguments> ImportDREAM3DFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ImportDREAM3DFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertTopParameters<SIMPLConversion::DataContainerReaderFilterParameterConverter>(args, json, k_ImportFileData));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace complex
