#include "ReadDREAM3DFilter.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/IO/HDF5/RectGridGeomIO.hpp"
#include "simplnx/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <utility>

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
  return Dream3dImportParameter::k_ReadFilterHumanName.str();
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
  params.insert(std::make_unique<BoolParameter>(k_UseDefaultGeometryValues_Key, Dream3dImportParameter::k_UseDefaultGeometryValuesLabel,
                                                "Use 0 for invalid stored origin components and 1 for invalid stored spacing components in the loaded image geometry. "
                                                "The file is not changed. Invalid rectilinear grid bounds still produce an error.",
                                                false));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadDREAM3DFilter::parametersVersion() const
{
  // Version 2: Added Use Default Origin/Spacing for Invalid Values.
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadDREAM3DFilter::clone() const
{
  return std::make_unique<ReadDREAM3DFilter>();
}

IFilter::PreflightResult ReadDREAM3DFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                          const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto importData = filterArgs.value<Dream3dImportParameter::ImportData>(k_ImportFileData);
  if(importData.FilePath.empty())
  {
    return {MakeErrorResult<OutputActions>(k_NoImportPathError, "Import file path not provided.")};
  }

  const bool useDefaultGeometryValues = filterArgs.value<bool>(k_UseDefaultGeometryValues_Key);

  // Neutral metadata avoids source-policy decisions before the destination action can plan selected arrays.
  auto dataStructureResult = DREAM3D::Dream3dPreflightCache::Instance().fetchSelectableMetadata(importData.FilePath);

  Result<OutputActions> result;

  if(dataStructureResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(dataStructureResult)), {})};
  }
  result.warnings() = std::move(dataStructureResult.warnings());
  const auto& importedDataStructure = dataStructureResult.value().dataStructure;

  std::vector<DataPath> selectedPaths;
  if(importData.ImportPolicy == Dream3dImportParameter::PathImportPolicy::IncludeList)
  {
    for(const auto& dataPath : importData.DataPaths)
    {
      if(!importedDataStructure.containsData(dataPath))
      {
        result.warnings().push_back(Warning{-12, fmt::format("The Include List path '{}' does not exist in the input file '{}'.", dataPath.toString(), importData.FilePath.string())});
      }
    }
    auto allDataPaths = importedDataStructure.getAllDataPaths();
    selectedPaths = DREAM3D::ExpandSelectedPathsToDescendants(importData.DataPaths, allDataPaths);
    selectedPaths = DREAM3D::ExpandSelectedPathsToAncestors(selectedPaths);

    if(selectedPaths.empty())
    {
      result.warnings().push_back(
          Warning{-10, "The import policy is set to 'Include List' and the file paths list is empty.  This will result in no data being imported.  Is this what you meant to do?"});
    }
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
        selectedPaths = std::move(allDataPaths);
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

        selectedPaths = std::move(finalDataPaths);
      }
    }
    else
    {
      selectedPaths = std::move(allDataPaths);
    }
  }
  else
  {
    auto failure = MakeErrorResult<OutputActions>(k_UnsupportedPathImportPolicyError, "The chosen PathImportPolicy is not supported by this filter.  Please contact the developers.");
    failure.warnings() = std::move(result.warnings());
    return {std::move(failure)};
  }

  auto validation = dataStructureResult.value().applyImageMetadata(importData.FilePath, selectedPaths, useDefaultGeometryValues);
  if(validation.invalid())
  {
    for(auto& error : validation.errors())
    {
      if(error.code == ImageGeom::k_InvalidOrigin || error.code == ImageGeom::k_InvalidSpacing)
      {
        error.message += fmt::format(" Enable '{}' to use defaults for invalid components in the loaded geometry.", Dream3dImportParameter::k_UseDefaultGeometryValuesLabel.view());
      }
    }
    auto failure = ConvertInvalidResult<OutputActions>(std::move(validation));
    failure.warnings() = std::move(result.warnings());
    return {std::move(failure)};
  }

  auto boundsValidation = HDF5::RectGridGeomIO::ValidateImportedBounds(importedDataStructure, importData.FilePath, selectedPaths);
  if(boundsValidation.invalid())
  {
    auto failure = ConvertInvalidResult<OutputActions>(std::move(boundsValidation));
    failure.warnings() = std::move(result.warnings());
    return {std::move(failure)};
  }

  // The action reports each selected geometry's defaulted values when applied in either mode.
  result.value().appendAction(std::make_unique<ImportH5ObjectPathsAction>(importData.FilePath, selectedPaths, useDefaultGeometryValues));
  return {std::move(result)};
}

Result<> ReadDREAM3DFilter::executeImpl(DataStructure& /*dataStructure*/, const Arguments& /*filterArgs*/, const PipelineFilter* /*pipelineNode*/, const MessageHandler& /*messageHandler*/,
                                        const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
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
