#pragma once

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <fstream>
#include <string>
namespace fs = std::filesystem;

namespace nx::core
{
inline const std::string k_FilterIncludeInsertToken = "# FILTER_INCLUDE_INSERT";
inline const std::string k_FilterNameInsertToken = "# FILTER_NAME_INSERT";
inline const std::string k_PythonFilterTemplate = R"(
@PYTHON_FILTER_TEMPLATE@
)";

/**
 *
 * @param filterName
 * @param humanName
 * @param uuidString
 * @return
 */
inline std::string GeneratePythonFilter(const std::string& filterName, const std::string& humanName, const std::string& uuidString)
{
  std::string content = k_PythonFilterTemplate;

  content = StringUtilities::replace(content, "#PYTHON_FILTER_NAME#", filterName);
  content = StringUtilities::replace(content, "#PYTHON_FILTER_HUMAN_NAME#", humanName);
  content = StringUtilities::replace(content, "#PYTHON_FILTER_UUID#", uuidString);

  return content;
}

/**
 *
 * @param outputPath
 * @param filterName
 * @return
 */
inline Result<> InsertFilterNameInPluginFiles(const std::filesystem::path& pluginPath, const std::string& filterName)
{
  Result<> result;
  std::string pluginName = pluginPath.stem().string();

  fs::path pluginPyPath = pluginPath / "Plugin.py";
  if(!fs::exists(pluginPyPath))
  {
    return MakeErrorResult(-2000, fmt::format("Non-existent plugin file at path: {}", pluginPyPath.string()));
  }

  fs::path initPyPath = pluginPath / "__init__.py";
  if(!fs::exists(initPyPath))
  {
    return MakeErrorResult(-2001, fmt::format("Non-existent plugin file at path: {}", initPyPath.string()));
  }

  std::vector<fs::path> pluginFilePaths = {pluginPyPath, initPyPath};
  for(const auto& pluginFilePath : pluginFilePaths)
  {
    std::ifstream file(pluginFilePath.string());
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // Insert filter import statement
    {
      std::size_t filterInsertPos = content.find(k_FilterIncludeInsertToken);
      if(filterInsertPos == std::string::npos)
      {
        result.warnings().push_back(
            {-2002, fmt::format("Plugin file ('{0}') does not contain the filter insert token ('{1}'), so the filter import statement could not be automatically inserted into the plugin "
                                "file.  This plugin file must be manually updated to include the filter import statement.",
                                pluginFilePath.string(), k_FilterNameInsertToken)});
      }
      filterInsertPos--; // Go back one character to insert before the newline

      std::string filterImportToken = fmt::format("from {0}.{1} import {1}\n", pluginName, filterName);
      content.insert(filterInsertPos, filterImportToken);
    }

    // Insert filter name
    {
      std::size_t filterInsertPos = content.find(k_FilterNameInsertToken);
      if(filterInsertPos == std::string::npos)
      {
        result.warnings().push_back(
            {-2002, fmt::format("Plugin file ('{0}') does not contain the filter insert token ('{1}'), so the filter name ('{2}') could not be automatically inserted into the plugin "
                                "file.  This plugin file must be manually updated to include the filter name ('{2}').",
                                pluginFilePath.string(), k_FilterNameInsertToken, filterName)});
      }
      filterInsertPos -= 2; // Go back two characters to insert before the closing brace

      std::string filterNameToken = filterName;
      if(pluginFilePath.filename() == "__init__.py")
      {
        filterNameToken = fmt::format("'{}'", filterNameToken);
      }
      content.insert(filterInsertPos, "," + filterNameToken);
    }

    std::ofstream out_file(pluginFilePath.string());
    out_file << content;
    out_file.close();
  }

  return result;
}

/**
 *
 * @param outputPath
 * @param filterName
 * @param humanName
 * @param uuidString
 * @return
 */
inline Result<> WritePythonFilterToPlugin(const std::filesystem::path& pluginPath, const std::string& filterName)
{
  std::string content = GeneratePythonFilter(filterName, filterName, Uuid::GenerateV4().str());
  fs::path outputPath = pluginPath / fmt::format("{}.py", filterName);
  AtomicFile tempFile(outputPath.string(), true);
  {
    // Scope this so that the file closes first before we then 'commit' with the atomic file
    std::ofstream fout(tempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
    if(!fout.is_open())
    {
      return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", tempFile.tempFilePath().string()));
    }

    fout << content;
  }

  return InsertFilterNameInPluginFiles(pluginPath, filterName);
}

/**
 *
 * @param outputPath
 * @param filterName
 * @param humanName
 * @param uuidString
 * @return
 */
inline Result<> WritePythonFiltersToPlugin(const std::filesystem::path& pluginPath, const std::string& filterNames)
{
  auto filterNamesSplit = StringUtilities::split(filterNames, ',');
  for(const auto& filterName : filterNamesSplit)
  {
    auto result = nx::core::WritePythonFilterToPlugin(pluginPath, filterName);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

/**
 *
 * @param outputPath
 * @param filterName
 * @param humanName
 * @param uuidString
 * @return
 */
inline Result<> WritePythonFilterToFile(const std::filesystem::path& outputPath, const std::string& filterName, const std::string& humanName, const std::string& uuidString)
{

  std::string content = GeneratePythonFilter(filterName, humanName, uuidString);
  auto outputFilePath = fmt::format("{}{}{}.py", outputPath.string(), std::string{std::filesystem::path::preferred_separator}, filterName);
  AtomicFile tempFile(outputFilePath, true);
  {
    // Scope this so that the file closes first before we then 'commit' with the atomic file
    std::ofstream fout(tempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
    if(!fout.is_open())
    {
      return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", tempFile.tempFilePath().string()));
    }

    fout << content;
  }

  return {};
}

inline const std::string k_PluginPythonFile = R"(
@PYTHON_PLUGIN_TEMPLATE@
)";

inline const std::string k_PluginInitPythonFile = R"(
@PYTHON_PLUGIN_INIT_TEMPLATE@
)";

/**
 *
 * @param pluginName
 * @param pluginUUID
 * @param pluginShortName
 * @param pluginDescription
 * @param pluginFilterList
 * @return
 */
inline std::string GeneratePythonPlugin(const std::string& pluginName, const std::string& pluginShortName, const std::string& pluginDescription, const std::string& pluginFilterList)
{
  std::string content = k_PluginPythonFile;

  content = StringUtilities::replace(content, "#PLUGIN_NAME#", pluginName);
  content = StringUtilities::replace(content, "#PLUGIN_UUID#", Uuid::GenerateV4().str());
  content = StringUtilities::replace(content, "#PLUGIN_SHORT_NAME#", pluginShortName);
  content = StringUtilities::replace(content, "#PLUGIN_DESCRIPTION#", pluginDescription);

  auto filterList = StringUtilities::split(pluginFilterList, ',');
  content = StringUtilities::replace(content, "#PLUGIN_FILTER_LIST#", fmt::format("{}", fmt::join(filterList, ",")));

  std::string importStatements;
  for(const auto& name : filterList)
  {
    importStatements.append(fmt::format("from {}.{} import {}\n", pluginName, name, name));
  }

  content = StringUtilities::replace(content, "#PLUGIN_IMPORT_CODE#", importStatements);

  return content;
}

/**
 *
 * @param outputDirectory
 * @param pluginName
 * @param pluginUUID
 * @param pluginShortName
 * @param pluginDescription
 * @param pluginFilterList
 * @return
 */
inline Result<> WritePythonPluginFiles(const std::filesystem::path& outputDirectory, const std::string& pluginName, const std::string& pluginShortName, const std::string& pluginDescription,
                                       const std::string& pluginFilterList)
{

  auto pluginRootPath = outputDirectory / pluginName;
  auto result = nx::core::CreateOutputDirectories(pluginRootPath);
  if(result.invalid())
  {
    return result;
  }
  auto outputPath = pluginRootPath / "Plugin.py";
  {
    AtomicFile tempFile(outputPath.string(), true);
    {
      // Scope this so that the file closes first before we then 'commit' with the atomic file
      std::ofstream fout(tempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
      if(!fout.is_open())
      {
        return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", tempFile.tempFilePath().string()));
      }

      std::string content = GeneratePythonPlugin(pluginName, pluginShortName, pluginDescription, pluginFilterList);

      fout << content;
    }
  }

  // Write the __init__.py file
  outputPath = pluginRootPath / "__init__.py";
  {
    AtomicFile initTempFile(outputPath.string(), true);
    {
      // Scope this so that the file closes first before we then 'commit' with the atomic file
      std::ofstream fout(initTempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
      if(!fout.is_open())
      {
        return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", initTempFile.tempFilePath().string()));
      }
      std::string content = k_PluginInitPythonFile;

      content = StringUtilities::replace(content, "#PLUGIN_NAME#", pluginName);
      content = StringUtilities::replace(content, "#PLUGIN_UUID#", Uuid::GenerateV4().str());
      content = StringUtilities::replace(content, "#PLUGIN_SHORT_NAME#", pluginShortName);
      content = StringUtilities::replace(content, "#PLUGIN_DESCRIPTION#", pluginDescription);

      auto filterList = StringUtilities::split(pluginFilterList, ',');

      std::string aList(fmt::format("'{}',", pluginName));
      for(const auto& name : filterList)
      {
        aList.append(fmt::format("'{}', ", name));
      }
      aList.append("'get_plugin'");
      content = StringUtilities::replace(content, "#PLUGIN_FILTER_LIST#", aList);

      std::string importStatements;
      for(const auto& name : filterList)
      {
        importStatements.append(fmt::format("from {}.{} import {}\n", pluginName, name, name));
      }

      content = StringUtilities::replace(content, "#PLUGIN_IMPORT_CODE#", importStatements);

      fout << content;
    }
  }
  // Now loop over each Filter and generate the skeleton files
  auto filterList = StringUtilities::split(pluginFilterList, ',');
  for(const auto& name : filterList)
  {
    WritePythonFilterToFile(pluginRootPath, name, name, Uuid::GenerateV4().str());
  }

  return {};
}

} // namespace nx::core
