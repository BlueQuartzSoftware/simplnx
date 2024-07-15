#pragma once

#include "simplnx/Common/AtomicFile.hpp"
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
  for(char ch : input)
  {
    if(std::isupper(ch))
    {
      if(!result.empty())
      {
        result += '_';
      }
      result += std::tolower(ch);
    }
    else
    {
      result += ch;
    }
  }
  return result;
}

inline const std::string k_FilterIncludeInsertToken = "# FILTER_INCLUDE_INSERT";
inline const std::string k_FilterNameInsertToken = "# FILTER_NAME_INSERT";

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
  std::filesystem::path plugPath(pluginName);
  pluginName = plugPath.stem().string();

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

  // Update the __init__.py file
  {
    std::ifstream file(initPyPath.string());
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    std::vector<std::string> lines = nx::core::StringUtilities::split(content, "\n", true);
    if(lines.back().empty())
    {
      lines.pop_back();
    }
    // Create the output file by opening the same file for OVER WRITE.
    std::ofstream outFile = std::ofstream(initPyPath.string(), std::ios_base::binary | std::ios_base::trunc);

    std::string filterMarkerLine = fmt::format("# FILTER_START: {}", filterName);
    std::string lastMarkerLine = "def get_plugin():";

    std::string filterImportToken = fmt::format("from {0}.{1} import {1}", pluginName, filterName);
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
    std::string content = buffer.str();
    file.close();
    std::vector<std::string> lines = nx::core::StringUtilities::split(content, "\n", true);
    if(lines.back().empty())
    {
      lines.pop_back();
    }
    // Create the output file by opening the same file for OVER WRITE.
    std::ofstream outFile = std::ofstream(pluginPyPath.string(), std::ios_base::binary | std::ios_base::trunc);

    std::string filterMarkerLine = fmt::format("# FILTER_START: {}", filterName);
    std::string lastMarkerLine = "import simplnx as nx";

    std::string filterImportToken = fmt::format("from {0}.{1} import {1}", pluginName, filterName);
    std::string filterInsertToken = fmt::format("'{}'", filterName);

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

    fout << content;
  }

  Result<> commitResult = tempFile.commit();
  if(commitResult.invalid())
  {
    return commitResult;
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
  Result<> result;
  auto filterNamesSplit = StringUtilities::split(filterNames, ',');
  for(const auto& filterName : filterNamesSplit)
  {
    auto filterWriteResult = nx::core::WritePythonFilterToPlugin(pluginPath, filterName);
    if(filterWriteResult.invalid())
    {
      return filterWriteResult;
    }
    result = MergeResults(result, filterWriteResult);
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
inline Result<> WritePythonFilterToFile(const std::filesystem::path& outputPath, const std::string& filterName, const std::string& humanName, const std::string& uuidString)
{

  std::string content = GeneratePythonFilter(filterName, humanName, uuidString);
  fs::path outputFilePath = outputPath / fmt::format("{}.py", filterName);
  auto atomicFileResult = AtomicFile::Create(outputFilePath);
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

    fout << content;
  }

  Result<> commitResult = tempFile.commit();
  if(commitResult.invalid())
  {
    return commitResult;
  }

  return {};
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

  std::stringstream ss;

  for(const auto& name : filterList)
  {
    ss << "# FILTER_START: " << name << "\n"
       << "from ." << name << " import " << name << "\n"
       << "_filters.append(" << name << ")\n"
       << "# FILTER_END: " << name << "\n\n";
  }
  content = StringUtilities::replace(content, "#PLUGIN_IMPORT_CODE#", ss.str());

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

      std::string content = GeneratePythonPlugin(pluginName, pluginShortName, pluginDescription, pluginFilterList);

      fout << content;
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

      auto filterList = StringUtilities::split(pluginFilterList, ',');

      std::string aList(fmt::format("'{}', ", pluginName));
      for(const auto& name : filterList)
      {
        aList.append(fmt::format("'{}', ", name));
      }
      aList.append("'get_plugin'");
      content = StringUtilities::replace(content, "#PLUGIN_FILTER_LIST#", aList);

      std::stringstream ss;
      for(const auto& name : filterList)
      {
        ss << "# FILTER_START: " << name << "\n"
           << "from ." << name << " import " << name << "\n"
           << "__all__.append('" << name << "')\n"
           << "# FILTER_END: " << name << "\n\n";
      }
      content = StringUtilities::replace(content, "#PLUGIN_IMPORT_CODE#", ss.str());

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
  auto filterList = StringUtilities::split(pluginFilterList, ',');
  for(const auto& name : filterList)
  {
    WritePythonFilterToFile(pluginSrcPath, name, name, Uuid::GenerateV4().str());
  }

  return {};
}
} // namespace nx::core
