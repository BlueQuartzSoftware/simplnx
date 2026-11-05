#pragma once

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/FileReplaceGuard.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/utils/PythonPluginSourceTemplate.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <fstream>
#include <string>
namespace fs = std::filesystem;

namespace nx::core
{
inline std::string CamelCaseToSnakeCase(const std::string& input)
{
  std::string result;
  for(const char letter : input)
  {
    if(std::isupper(letter) != 0)
    {
      if(!result.empty())
      {
        result += '_';
      }
      result += static_cast<char>(std::tolower(letter));
    }
    else
    {
      result += letter;
    }
  }
  return result;
}

// inline const std::string k_FilterIncludeInsertToken = "# FILTER_INCLUDE_INSERT";
// inline const std::string k_FilterNameInsertToken = "# FILTER_NAME_INSERT";

/**
 *
 * @param filterName
 * @param humanName
 * @param uuidString
 * @return
 */
inline std::string GeneratePythonFilter(const std::string& filterName, const std::string& humanName, const std::string& uuidString)
{
  std::string content = PythonFilterTemplate();

  content = StringUtilities::replace(content, "#PYTHON_FILTER_NAME#", filterName);
  content = StringUtilities::replace(content, "#PYTHON_FILTER_HUMAN_NAME#", humanName);
  content = StringUtilities::replace(content, "#PYTHON_FILTER_UUID#", uuidString);

  return content;
}

/**
 * @brief Adds the import lines of a filter to the __init__.py and Plugin.py files of an existing Python plugin.
 * @param pluginPath Source folder of the plugin that holds both files.
 * @param filterName Name of the filter module.
 * @return Success with the warnings of the file-replace hooks, -2000 or -2001 when a file is missing, or the first hook error.
 *
 * Both files are rewritten in place. BeforeFileReplace() runs on both files before either one is truncated.
 */
inline Result<> InsertFilterNameInPluginFiles(const std::filesystem::path& pluginPath, const std::string& filterName)
{
  Result<> result;
  std::string pluginName = pluginPath.string();
  if(StringUtilities::ends_with(pluginName, "/"))
  {
    pluginName.pop_back();
  }
  // const std::filesystem::path plugPath(pluginName);
  // pluginName = plugPath.stem().string();

  const fs::path pluginPyPath = pluginPath / "Plugin.py";
  if(!fs::exists(pluginPyPath))
  {
    return MakeErrorResult(-2000, fmt::format("Non-existent plugin file at path: {}", pluginPyPath.string()));
  }

  const fs::path initPyPath = pluginPath / "__init__.py";
  if(!fs::exists(initPyPath))
  {
    return MakeErrorResult(-2001, fmt::format("Non-existent plugin file at path: {}", initPyPath.string()));
  }

  // Both files are rewritten in place, so the file-replace hook runs on both before either one is truncated.
  for(const fs::path& rewrittenPath : {initPyPath, pluginPyPath})
  {
    Result<> hookResult = BeforeFileReplace(rewrittenPath);
    if(hookResult.invalid())
    {
      hookResult.warnings().insert(hookResult.warnings().begin(), result.warnings().begin(), result.warnings().end());
      return hookResult;
    }
    result.warnings().insert(result.warnings().end(), hookResult.warnings().begin(), hookResult.warnings().end());
  }

  // Update the __init__.py file
  {
    std::ifstream file(initPyPath.string());
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();
    file.close();
    std::vector<std::string> lines = nx::core::StringUtilities::split(content, "\n", true);
    if(lines.back().empty())
    {
      lines.pop_back();
    }
    // Create the output file by opening the same file for OVERWRITE.
    std::ofstream outFile = std::ofstream(initPyPath.string(), std::ios_base::binary | std::ios_base::trunc);

    const std::string filterMarkerLine = fmt::format("# FILTER_START: {}", filterName);
    const std::string lastMarkerLine = "def get_plugin():";

    // const std::string filterImportToken = fmt::format("from {0}.{1} import {1}", pluginName, filterName);
    bool insertToken = true;
    for(auto& line : lines)
    {

      if(line == filterMarkerLine)
      {
        insertToken = false;
      }
      if(line == lastMarkerLine && insertToken)
      {
        outFile << "# FILTER_START: " << filterName << "\n"
                << "from ." << filterName << " import " << filterName << "\n"
                << "__all__.append('" << filterName << "')\n"
                << "# FILTER_END: " << filterName << "\n\n";
      }
      outFile << line << '\n'; // Write the line out to the file
    }
    outFile.close();
  }

  // Update the Plugin.py file
  {
    std::ifstream file(pluginPyPath.string());
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();
    file.close();
    std::vector<std::string> lines = nx::core::StringUtilities::split(content, "\n", true);
    if(lines.back().empty())
    {
      lines.pop_back();
    }
    // Create the output file by opening the same file for OVERWRITE.
    std::ofstream outFile = std::ofstream(pluginPyPath.string(), std::ios_base::binary | std::ios_base::trunc);

    const std::string filterMarkerLine = fmt::format("# FILTER_START: {}", filterName);
    const std::string lastMarkerLine = "import simplnx as nx";

    // const std::string filterImportToken = fmt::format("from {0}.{1} import {1}", pluginName, filterName);
    // const std::string filterInsertToken = fmt::format("'{}'", filterName);

    bool insertToken = true;
    for(auto& line : lines)
    {
      // If the line is the exact same as the generated import statement mark false
      if(line == filterMarkerLine)
      {
        insertToken = false;
      }
      if(line == lastMarkerLine && insertToken)
      {
        outFile << "# FILTER_START: " << filterName << "\n"
                << "from ." << filterName << " import " << filterName << "\n"
                << "_filters.append(" << filterName << ")\n"
                << "# FILTER_END: " << filterName << "\n\n";
      }
      outFile << line << '\n'; // Write the line out to the file
    }
    outFile.close();
  }

  return result;
}

/**
 * @brief Writes text to a file through AtomicFile.
 * @param content Text to write.
 * @param outputFilePath Final path of the file.
 * @return Success with the warnings of the file-replace hook. Otherwise the AtomicFile creation error, -74100 when the
 * temporary file cannot open, or the commit error, each with the hook warnings.
 */
inline Result<> AtomicallyWriteFile(const std::string& content, const std::filesystem::path& outputFilePath)
{
  auto atomicFileResult = AtomicFile::Create(outputFilePath);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }

  AtomicFile tempFile = std::move(atomicFileResult.value());
  Result<> result;
  {
    // Scope this so that the file closes first before we then 'commit' with the atomic file
    std::ofstream fileOut(tempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
    if(fileOut.is_open())
    {
      fileOut << content;
    }
    else
    {
      result = MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", tempFile.tempFilePath().string()));
    }
  }
  if(result.valid())
  {
    result = tempFile.commit();
  }
  // The file-replace hook in AtomicFile::Create() can report warnings; they come first.
  result.warnings().insert(result.warnings().begin(), atomicFileResult.warnings().begin(), atomicFileResult.warnings().end());
  return result;
}

/**
 * @brief Adds the warnings of a valid result to the warnings collected so far, or returns the failing result.
 * @param collected Result that collects the warnings of each step. It stays valid.
 * @param stepResult Result of the next step.
 * @return True when stepResult is valid. Otherwise false, and stepResult holds the warnings of collected in front of its own.
 *
 * MergeResults() is not used, because it would turn a step result that is invalid with no errors into a success.
 */
inline bool CollectStepResult(Result<>& collected, Result<>& stepResult)
{
  if(stepResult.invalid())
  {
    stepResult.warnings().insert(stepResult.warnings().begin(), collected.warnings().begin(), collected.warnings().end());
    return false;
  }
  collected.warnings().insert(collected.warnings().end(), stepResult.warnings().begin(), stepResult.warnings().end());
  return true;
}

/**
 * @brief Writes the Python source file of a filter and its documentation stub, and optionally registers the filter.
 * @param outputPath Source folder of the plugin.
 * @param filterName Name of the filter module.
 * @param humanName Human-readable filter name.
 * @param updatePluginFiles True to add the filter to the __init__.py and Plugin.py files.
 * @return Success with the warnings of every step, or the first error with the earlier warnings. The first failure stops
 * before a later file changes.
 */
inline Result<> WritePythonFilterToPlugin(const std::filesystem::path& outputPath, const std::string& filterName, const std::string& humanName, bool updatePluginFiles)
{
  Result<> result;

  // **************************************************************************
  // Write Python Filter Skeleton code
  // **************************************************************************
  {
    const std::string content = GeneratePythonFilter(filterName, humanName, Uuid::GenerateV4().str());
    const fs::path outputFilePath = outputPath / fmt::format("{}.py", filterName);
    Result<> writeResult = AtomicallyWriteFile(content, outputFilePath);
    if(!CollectStepResult(result, writeResult))
    {
      return writeResult;
    }
  }

  // **************************************************************************
  // Write Matching Documentation Stub File
  // **************************************************************************
  {
    std::stringstream docOut;
    docOut << "# " << filterName << "\n";
    docOut << "\n\n";
    docOut << "## Group (Subgroup)\n";
    docOut << "\n\n";
    docOut << "## Description\n";
    docOut << "\nThis filter....\n";
    docOut << "## Parameters\n\n";
    docOut << "List each parameter, its type and brief description\n";

    docOut << "## Example Pipelines\n";
    docOut << "  \n";
    docOut << "  \n";
    docOut << "\n";
    docOut << "## License & Copyright\n";
    docOut << "\n";
    docOut << "Please see the description file distributed with this **Plugin**\n";

    const fs::path outputFilePath = outputPath / ".." / ".." / "docs" / fmt::format("{}.md", filterName);

    Result<> writeResult = AtomicallyWriteFile(docOut.str(), outputFilePath);
    if(!CollectStepResult(result, writeResult))
    {
      return writeResult;
    }
  }

  if(updatePluginFiles)
  {
    Result<> insertResult = InsertFilterNameInPluginFiles(outputPath, filterName);
    if(!CollectStepResult(result, insertResult))
    {
      return insertResult;
    }
  }
  return result;
}

/**
 * @brief Writes and registers each filter of a comma-separated list in an existing Python plugin.
 * @param pluginPath Source folder of the plugin.
 * @param filterNames Comma-separated filter names.
 * @return Success with the warnings of every filter, or the first error with the earlier warnings.
 */
inline Result<> WritePythonFiltersToPlugin(const std::filesystem::path& pluginPath, const std::string& filterNames)
{
  Result<> result;
  auto filterNamesSplit = StringUtilities::split(filterNames, ',');
  for(const auto& filterName : filterNamesSplit)
  {
    auto filterWriteResult = nx::core::WritePythonFilterToPlugin(pluginPath, filterName, filterName, true);
    if(!CollectStepResult(result, filterWriteResult))
    {
      return filterWriteResult;
    }
  }

  return result;
}

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
  std::string content = PluginPythonFile();

  content = StringUtilities::replace(content, "#PLUGIN_NAME#", pluginName);
  content = StringUtilities::replace(content, "#PLUGIN_UUID#", Uuid::GenerateV4().str());
  content = StringUtilities::replace(content, "#PLUGIN_SHORT_NAME#", pluginShortName);
  content = StringUtilities::replace(content, "#PLUGIN_DESCRIPTION#", pluginDescription);

  auto filterList = StringUtilities::split(pluginFilterList, ',');
  content = StringUtilities::replace(content, "#PLUGIN_FILTER_LIST#", fmt::format("{}", fmt::join(filterList, ", ")));

  std::stringstream codeStream;

  for(const auto& name : filterList)
  {
    codeStream << "# FILTER_START: " << name << "\n"
               << "from ." << name << " import " << name << "\n"
               << "_filters.append(" << name << ")\n"
               << "# FILTER_END: " << name << "\n\n";
  }
  content = StringUtilities::replace(content, "#PLUGIN_IMPORT_CODE#", codeStream.str());

  return content;
}

/**
 *
 * @param outputDirectory
 * @param pluginName
 * @param pluginShortName
 * @param pluginDescription
 * @param pluginFilterList
 * @return
 */
inline Result<> WritePythonPluginFiles(const std::filesystem::path& outputDirectory, const std::string& pluginName, const std::string& pluginShortName, const std::string& pluginDescription,
                                       const std::string& pluginFilterList)
{
  auto pluginRootPath = outputDirectory / pluginName;
  auto pluginSrcPath = pluginRootPath / "src" / pluginName;
  auto result = nx::core::CreateOutputDirectories(pluginRootPath);
  if(result.invalid())
  {
    return result;
  }
  auto pluginCondaPath = pluginRootPath / "conda";
  result = nx::core::CreateOutputDirectories(pluginCondaPath);
  if(result.invalid())
  {
    return result;
  }
  auto pluginDocsPath = pluginRootPath / "docs";
  result = nx::core::CreateOutputDirectories(pluginDocsPath);
  if(result.invalid())
  {
    return result;
  }
  // Each file is written through AtomicFile. The first failure stops before a later file is prepared or changed.
  {
    Result<> writeResult = AtomicallyWriteFile(GeneratePythonPlugin(pluginName, pluginShortName, pluginDescription, pluginFilterList), pluginSrcPath / "Plugin.py");
    if(!CollectStepResult(result, writeResult))
    {
      return writeResult;
    }
  }

  // Write the __init__.py file
  {
    std::string content = PluginInitPythonFile();

    content = StringUtilities::replace(content, "#PLUGIN_NAME#", pluginName);
    content = StringUtilities::replace(content, "#PLUGIN_UUID#", Uuid::GenerateV4().str());
    content = StringUtilities::replace(content, "#PLUGIN_SHORT_NAME#", pluginShortName);
    content = StringUtilities::replace(content, "#PLUGIN_DESCRIPTION#", pluginDescription);

    auto filterList = StringUtilities::split(pluginFilterList, ',');

    std::string aList(fmt::format("'{}', ", pluginName));
    for(const auto& name : filterList)
    {
      aList.append(fmt::format("'{}', ", name));
    }
    aList.append("'get_plugin'");
    content = StringUtilities::replace(content, "#PLUGIN_FILTER_LIST#", aList);

    std::stringstream codeStream;
    for(const auto& name : filterList)
    {
      codeStream << "# FILTER_START: " << name << "\n"
                 << "from ." << name << " import " << name << "\n"
                 << "__all__.append('" << name << "')\n"
                 << "# FILTER_END: " << name << "\n\n";
    }
    content = StringUtilities::replace(content, "#PLUGIN_IMPORT_CODE#", codeStream.str());

    Result<> writeResult = AtomicallyWriteFile(content, pluginSrcPath / "__init__.py");
    if(!CollectStepResult(result, writeResult))
    {
      return writeResult;
    }
  }

  // Write the environment.yml file
  {
    std::string content = PluginEnvironmentPythonFile();
    content = StringUtilities::replace(content, "#PLUGIN_ENV_NAME#", CamelCaseToSnakeCase(pluginName));
    Result<> writeResult = AtomicallyWriteFile(content, pluginRootPath / "environment.yml");
    if(!CollectStepResult(result, writeResult))
    {
      return writeResult;
    }
  }

  // Write the pyproject.toml file
  {
    std::string content = PluginPyProjectPythonFile();
    content = StringUtilities::replace(content, "#PLUGIN_NAME#", pluginName);
    Result<> writeResult = AtomicallyWriteFile(content, pluginRootPath / "pyproject.toml");
    if(!CollectStepResult(result, writeResult))
    {
      return writeResult;
    }
  }

  // Write the meta.yaml file
  {
    std::string content = PluginMetaPythonFile();
    content = StringUtilities::replace(content, "#PLUGIN_NAME#", pluginName);
    Result<> writeResult = AtomicallyWriteFile(content, pluginCondaPath / "meta.yaml");
    if(!CollectStepResult(result, writeResult))
    {
      return writeResult;
    }
  }

  // Generate the skeleton files of each filter.
  auto filterNamesSplit = StringUtilities::split(pluginFilterList, ',');
  for(const auto& filterName : filterNamesSplit)
  {
    auto filterWriteResult = nx::core::WritePythonFilterToPlugin(pluginSrcPath, filterName, filterName, false);
    if(!CollectStepResult(result, filterWriteResult))
    {
      return filterWriteResult;
    }
  }

  return result;
}
} // namespace nx::core
