#include "ReadDREAM3DFilter.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace
{
constexpr nx::core::int32 k_NoImportPathError = -1;
constexpr nx::core::int32 k_UnsupportedPathImportPolicyError = -51;
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
  params.insert(
      std::make_unique<Dream3dImportParameter>(k_ImportFileData, "Import File Path", "The HDF5 file path the DataStructure should be imported from.", Dream3dImportParameter::ImportData({})));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadDREAM3DFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadDREAM3DFilter::clone() const
{
  return std::make_unique<ReadDREAM3DFilter>();
}

IFilter::PreflightResult ReadDREAM3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                          const ExecutionContext& executionContext) const
{
  auto importData = filterArgs.value<Dream3dImportParameter::ImportData>(k_ImportFileData);
  if(importData.FilePath.empty())
  {
    return {MakeErrorResult<OutputActions>(k_NoImportPathError, "Import file path not provided.")};
  }

  // Preflight metadata is served from Dream3dPreflightCache instead of being
  // read from the file on every pass: pipelines re-preflight on every
  // parameter edit, and a full HDF5 metadata traversal per edit freezes the
  // UI for seconds on high-latency storage (network mounts). After the first
  // import, each preflight costs a single stat() (see Dream3dPreflightCache).
  Result<DataStructure> dataStructureResult = DREAM3D::Dream3dPreflightCache::Instance().fetch(importData.FilePath);

  Result<OutputActions> result;
  OutputActions& actions = result.value();

  if(dataStructureResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(dataStructureResult)), {})};
  }
  auto importedDataStructure = dataStructureResult.value();

  if(importData.ImportPolicy == Dream3dImportParameter::PathImportPolicy::IncludeList)
  {
    auto allDataPaths = importedDataStructure.getAllDataPaths();
    std::vector<DataPath> selectedDataPaths = DREAM3D::ExpandSelectedPathsToDescendants(importData.DataPaths, allDataPaths);
    selectedDataPaths = DREAM3D::ExpandSelectedPathsToAncestors(selectedDataPaths);

    if(selectedDataPaths.empty())
    {
      result.warnings().push_back(
          Warning{-10, "The import policy is set to 'Include List' and the file paths list is empty.  This will result in no data being imported.  Is this what you meant to do?"});
    }

    actions.appendAction(std::make_unique<ImportH5ObjectPathsAction>(importData.FilePath, selectedDataPaths));
  }
  else if(importData.ImportPolicy == Dream3dImportParameter::PathImportPolicy::ExcludeList || importData.ImportPolicy == Dream3dImportParameter::PathImportPolicy::All)
  {
    auto allDataPaths = importedDataStructure.getAllDataPaths();
    if(importData.ImportPolicy == Dream3dImportParameter::PathImportPolicy::ExcludeList)
    {
      if(importData.DataPaths.empty())
      {
        result.warnings().push_back(Warning{-11,
                                            "The import policy is set to 'Exclude List' and the file paths list is empty.  This will result in all data being imported.  You can accomplish the same "
                                            "result by setting the import policy to 'All'."});
        actions.appendAction(std::make_unique<ImportH5ObjectPathsAction>(importData.FilePath, allDataPaths));
      }
      else
      {
        auto expandedDataPaths = DREAM3D::ExpandSelectedPathsToDescendants(importData.DataPaths, allDataPaths);

        // Erase the expandedDataPaths from allPaths to create finalDataPaths
        std::vector<nx::core::DataPath> finalDataPaths = allDataPaths;
        for(const auto& dataPath : expandedDataPaths)
        {
          auto iter = std::find(finalDataPaths.begin(), finalDataPaths.end(), dataPath);
          if(iter != finalDataPaths.end())
          {
            finalDataPaths.erase(iter);
          }
        }

        actions.appendAction(std::make_unique<ImportH5ObjectPathsAction>(importData.FilePath, finalDataPaths));
      }
    }
    else
    {
      actions.appendAction(std::make_unique<ImportH5ObjectPathsAction>(importData.FilePath, allDataPaths));
    }
  }
  else
  {
    return {MakeErrorResult<OutputActions>(k_UnsupportedPathImportPolicyError, "The chosen PathImportPolicy is not supported by this filter.  Please contact the developers.")};
  }

  return {result};
}

Result<> ReadDREAM3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  return {};
}

nlohmann::json ReadDREAM3DFilter::toJson(const Arguments& args) const
{
  auto json = IFilter::toJson(args);
  // Disabled writing pipeline json from DREAM3D file to prevent infinite recursive loop
  // when the pipeline reads and writes to the same file
  // See https://github.com/BlueQuartzSoftware/simplnx/pull/1033

  // auto importData = filterArgs.value<Dream3dImportParameter::ImportData>(k_ImportFileData);
  // if(!importData.FilePath.empty())
  // {
  //   auto d3dReader = nx::core::HDF5::FileIO::ReadFile(importData.FilePath);
  //   if(d3dReader.isValid())
  //   {
  //     std::string fileVersion = DREAM3D::GetFileVersion(d3dReader);
  //     // File version checking should be more robust
  //     if(fileVersion == DREAM3D::k_CurrentFileVersion)
  //     {
  //       Result<Pipeline> pipelineResult = DREAM3D::ImportPipelineFromFile(d3dReader);
  //       if(pipelineResult.valid())
  //       {
  //         json[k_ImportFileData] = pipelineResult.value().toJson();
  //       }
  //     }
  //   }
  // }
  return json;
}

Result<Arguments> ReadDREAM3DFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadDREAM3DFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertTopParameters<SIMPLConversion::DataContainerReaderFilterParameterConverter>(args, json, k_ImportFileData));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
