#pragma once

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/utils/PythonPluginSourceTemplate.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
namespace fs = std::filesystem;

namespace nx::core
{
/**
 * @brief Splits the comma-separated filter names, trims white space from each name and checks
 * that each name can be used as a Python module and class name.
 * @param filterNames The comma-separated filter names. An empty value means no filters.
 * @return The trimmed names, or error -105150 that names the first entry that is empty, is a
 * reserved Python name (a keyword or __debug__) or is not a valid ASCII Python identifier.
 */
inline Result<std::vector<std::string>> ParsePythonFilterNames(const std::string& filterNames)
{
  static constexpr std::array<std::string_view, 36> k_ReservedNames = {"False",  "None",     "True", "and",    "as",      "assert", "async",  "await",  "break", "class",  "continue", "def",
                                                                       "del",    "elif",     "else", "except", "finally", "for",    "from",   "global", "if",    "import", "in",       "is",
                                                                       "lambda", "nonlocal", "not",  "or",     "pass",    "raise",  "return", "try",    "while", "with",   "yield",    "__debug__"};
  const auto isIdentifierStart = [](char value) { return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || value == '_'; };
  const auto isIdentifierChar = [&isIdentifierStart](char value) { return isIdentifierStart(value) || (value >= '0' && value <= '9'); };

  if(StringUtilities::trimmed(filterNames).empty())
  {
    return {std::vector<std::string>{}};
  }
  constexpr std::array<char, 1> k_NameDelimiters = {','};
  auto names = StringUtilities::split(filterNames, k_NameDelimiters, true);
  for(auto& name : names)
  {
    const std::string original = name;
    name = StringUtilities::trimmed(name);
    if(name.empty() || !isIdentifierStart(name.front()) || !std::all_of(name.begin(), name.end(), isIdentifierChar) ||
       std::find(k_ReservedNames.begin(), k_ReservedNames.end(), name) != k_ReservedNames.end())
    {
      return MakeErrorResult<std::vector<std::string>>(-105150, fmt::format("Filter Names entry '{}' is empty, is a reserved Python name or is not a valid Python identifier. Use a letter or '_' "
                                                                            "first, followed by letters, digits or '_'.",
                                                                            original));
    }
  }
  return {std::move(names)};
}

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
 *
 * @param outputPath
 * @param filterName
 * @return
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
 *
 * @return
 */
inline Result<> AtomicallyWriteFile(const std::string& content, const std::filesystem::path& outputFilePath)
{
  auto atomicFileResult = AtomicFile::Create(outputFilePath);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }

  AtomicFile tempFile = std::move(atomicFileResult.value());
  {
    // Scope this so that the file closes first before we then 'commit' with the atomic file
    std::ofstream fileOut(tempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
    if(!fileOut.is_open())
    {
      return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", tempFile.tempFilePath().string()));
    }

    fileOut << content;
  }

  return tempFile.commit();
}

/**
 *
 * @param outputPath
 * @param filterName
 * @param humanName
 * @param uuidString
 * @return
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
    result = MergeResults(result, AtomicallyWriteFile(content, outputFilePath));
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

    result = MergeResults(result, AtomicallyWriteFile(docOut.str(), outputFilePath));
  }

  if(updatePluginFiles)
  {
    result = MergeResults(result, InsertFilterNameInPluginFiles(outputPath, filterName));
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
inline Result<> WritePythonFiltersToPlugin(const std::filesystem::path& pluginPath, const std::string& filterNames)
{
  Result<> result;
  auto namesResult = ParsePythonFilterNames(filterNames);
  if(namesResult.invalid())
  {
    return ConvertResult(std::move(namesResult));
  }
  const auto& filterNamesSplit = namesResult.value();
  for(const auto& filterName : filterNamesSplit)
  {
    auto filterWriteResult = nx::core::WritePythonFilterToPlugin(pluginPath, filterName, filterName, true);
    if(filterWriteResult.invalid())
    {
      return filterWriteResult;
    }
    result = MergeResults(result, filterWriteResult);
  }

  return result;
}

/**
 * @brief Escapes text so that it can be placed between single quotes in Python source code.
 * @param text The text to escape (UTF-8 bytes are kept as they are)
 * @return The escaped text without the surrounding quotes
 */
inline std::string EscapePythonSingleQuotedString(std::string_view text)
{
  std::string escaped;
  escaped.reserve(text.size());
  for(const char character : text)
  {
    switch(character)
    {
    case '\\':
      escaped.append("\\\\");
      break;
    case '\'':
      escaped.append("\\'");
      break;
    case '\n':
      escaped.append("\\n");
      break;
    case '\r':
      escaped.append("\\r");
      break;
    case '\t':
      escaped.append("\\t");
      break;
    default:
      if(const auto byte = static_cast<unsigned char>(character); byte < 0x20 || byte == 0x7F)
      {
        escaped.append(fmt::format("\\x{:02x}", byte));
      }
      else
      {
        escaped.push_back(character);
      }
      break;
    }
  }
  return escaped;
}

/**
 *
 * @param pluginName
 * @param pluginUUID
 * @param pluginShortName
 * @param pluginDescription
 * @param filterList
 * @return
 */
inline std::string GeneratePythonPlugin(const std::string& pluginName, const std::string& pluginShortName, const std::string& pluginDescription, const std::vector<std::string>& filterList)
{
  std::string content = PluginPythonFile();

  content = StringUtilities::replace(content, "#PLUGIN_NAME#", pluginName);
  content = StringUtilities::replace(content, "#PLUGIN_UUID#", Uuid::GenerateV4().str());
  content = StringUtilities::replace(content, "#PLUGIN_DESCRIPTION#", pluginDescription);

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
  // Insert the human name last so its literal text is not interpreted as template tokens.
  content = StringUtilities::replace(content, "#PLUGIN_SHORT_NAME#", EscapePythonSingleQuotedString(pluginShortName));

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
  auto namesResult = ParsePythonFilterNames(pluginFilterList);
  if(namesResult.invalid())
  {
    return ConvertResult(std::move(namesResult));
  }
  const auto& filterList = namesResult.value();
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
  auto outputPath = pluginSrcPath / "Plugin.py";
  {
    auto atomicFileResult = AtomicFile::Create(outputPath);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile tempFile = std::move(atomicFileResult.value());
    {
      // Scope this so that the file closes first before we then 'commit' with the atomic file
      std::ofstream fout(tempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
      if(!fout.is_open())
      {
        return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", tempFile.tempFilePath().string()));
      }

      fout << GeneratePythonPlugin(pluginName, pluginShortName, pluginDescription, filterList);
    }
    Result<> commitResult = tempFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  // Write the __init__.py file
  outputPath = pluginSrcPath / "__init__.py";
  {
    auto atomicFileResult = AtomicFile::Create(outputPath);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile initTempFile = std::move(atomicFileResult.value());
    {
      // Scope this so that the file closes first before we then 'commit' with the atomic file
      std::ofstream fout(initTempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
      if(!fout.is_open())
      {
        return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", initTempFile.tempFilePath().string()));
      }
      std::string content = PluginInitPythonFile();

      content = StringUtilities::replace(content, "#PLUGIN_NAME#", pluginName);
      content = StringUtilities::replace(content, "#PLUGIN_UUID#", Uuid::GenerateV4().str());
      content = StringUtilities::replace(content, "#PLUGIN_SHORT_NAME#", pluginShortName);
      content = StringUtilities::replace(content, "#PLUGIN_DESCRIPTION#", pluginDescription);

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

      fout << content;
    }
    Result<> commitResult = initTempFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  // Write the environment.yml file
  outputPath = pluginRootPath / "environment.yml";
  {
    auto atomicFileResult = AtomicFile::Create(outputPath);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile envTempFile = std::move(atomicFileResult.value());
    {
      // Scope this so that the file closes first before we then 'commit' with the atomic file
      std::ofstream fout(envTempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
      if(!fout.is_open())
      {
        return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", envTempFile.tempFilePath().string()));
      }
      std::string content = PluginEnvironmentPythonFile();

      content = StringUtilities::replace(content, "#PLUGIN_ENV_NAME#", CamelCaseToSnakeCase(pluginName));

      fout << content;
    }
    Result<> commitResult = envTempFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  // Write the pyproject.toml file
  outputPath = pluginRootPath / "pyproject.toml";
  {
    auto atomicFileResult = AtomicFile::Create(outputPath);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile pyProjTempFile = std::move(atomicFileResult.value());
    {
      // Scope this so that the file closes first before we then 'commit' with the atomic file
      std::ofstream fout(pyProjTempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
      if(!fout.is_open())
      {
        return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", pyProjTempFile.tempFilePath().string()));
      }
      std::string content = PluginPyProjectPythonFile();
      content = StringUtilities::replace(content, "#PLUGIN_NAME#", pluginName);
      fout << content;
    }
    Result<> commitResult = pyProjTempFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  // Write the meta.yaml file
  outputPath = pluginCondaPath / "meta.yaml";
  {
    auto atomicFileResult = AtomicFile::Create(outputPath);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile metaTempFile = std::move(atomicFileResult.value());
    {
      // Scope this so that the file closes first before we then 'commit' with the atomic file
      std::ofstream fout(metaTempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
      if(!fout.is_open())
      {
        return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", metaTempFile.tempFilePath().string()));
      }
      std::string content = PluginMetaPythonFile();
      content = StringUtilities::replace(content, "#PLUGIN_NAME#", pluginName);
      fout << content;
    }
    Result<> commitResult = metaTempFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  // Now loop over each Filter and generate the skeleton files
  for(const auto& filterName : filterList)
  {
    auto filterWriteResult = nx::core::WritePythonFilterToPlugin(pluginSrcPath, filterName, filterName, false);
    if(filterWriteResult.invalid())
    {
      return filterWriteResult;
    }
    result = MergeResults(result, filterWriteResult);
  }

  return result;
}
} // namespace nx::core
