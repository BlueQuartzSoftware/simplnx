#include "ReadDREAM3DFilter.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Readers/FileReader.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <nlohmann/json.hpp>

namespace
{
constexpr nx::core::StringLiteral k_ImportedPipeline = "Imported Pipeline";
constexpr nx::core::int32 k_NoImportPathError = -1;
constexpr nx::core::int32 k_FailedOpenFileReaderError = -25;
} // namespace

namespace nx::core
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
  return "Read DREAM3D-NX File";
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
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
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
  nx::core::HDF5::FileReader fileReader(importData.FilePath);
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
  nx::core::HDF5::FileReader d3dReader(importData.FilePath);
  if(d3dReader.isValid())
  {
    std::string fileVersion = DREAM3D::GetFileVersion(d3dReader);
    // File version checking should be more robust
    if(fileVersion == DREAM3D::k_CurrentFileVersion)
    {
      Result<Pipeline> pipelineResult = DREAM3D::ImportPipelineFromFile(d3dReader);
      if(pipelineResult.valid())
      {
        json[k_ImportedPipeline] = pipelineResult.value().toJson();
      }
    }
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

Result<Arguments> ReadDREAM3DFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadDREAM3DFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertTopParameters<SIMPLConversion::DataContainerReaderFilterParameterConverter>(args, json, k_ImportFileData));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
