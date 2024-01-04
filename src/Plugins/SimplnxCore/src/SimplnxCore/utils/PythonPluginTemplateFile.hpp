#pragma once

#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/Uuid.hpp"

#include <fmt/format.h>

#include <string>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;



namespace nx::core
{


inline const std::string k_PythonFilterTemplate = R"(

from typing import List
import simplnx as nx

class @PYTHON_FILTER_NAME@:
  """
  This section should contain the 'keys' that store each parameter. The value of the key should be snake_case. The name
  of the value should be ALL_CAPITOL_KEY
  """
  TEST_KEY = 'test'

  def uuid(self) -> nx.Uuid:
    """This returns the UUID of the filter. Each filter has a unique UUID value
    :return: The Filter's Uuid value
    :rtype: string
    """
    return nx.Uuid('@PYTHON_FILTER_UUID@')

  def human_name(self) -> str:
    """This returns the name of the filter as a user of DREAM3DNX would see it
    :return: The filter's human name
    :rtype: string
    """
    return '@PYTHON_FILTER_HUMAN_NAME@'

  def class_name(self) -> str:
    """The returns the name of the class that implements the filter
    :return: The name of the implementation class
    :rtype: string
    """
    return '@PYTHON_FILTER_NAME@'

  def name(self) -> str:
    """The returns the name of filter
    :return: The name of the filter
    :rtype: string
    """
    return '@PYTHON_FILTER_NAME@'

  def default_tags(self) -> List[str]:
    """This returns the default tags for this filter
    :return: The default tags for the filter
    :rtype: list
    """
    return ['python']

  def clone(self):
    """Clones the filter
    :return: A new instance of the filter
    :rtype:  @PYTHON_FILTER_NAME@
    """
    return @PYTHON_FILTER_NAME@()

  def parameters(self) -> nx.Parameters:
    """This function defines the parameters that are needed by the filter. Parameters collect the values from the user
       or through a pipeline file.
    """
    params = nx.Parameters()

    params.insert(nx.Float64Parameter(@PYTHON_FILTER_NAME@.TEST_KEY, 'Test', '', 0.0))

    return params

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    """This method preflights the filter and should ensure that all inputs are sanity checked as best as possible. Array
    sizes can be checked if the arrays are actually know at preflight time. Some filters will not be able to report output
    array sizes during preflight (segmentation filters for example).
    :returns:
    :rtype: nx.IFilter.PreflightResult
    """
    value: float = args[@PYTHON_FILTER_NAME@.TEST_KEY]
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Preflight: {value}'))
    return nx.IFilter.PreflightResult()

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    """ This method actually executes the filter algorithm and reports results.
    :returns:
    :rtype: nx.IFilter.ExecuteResult
    """

    value: float = args[@PYTHON_FILTER_NAME@.TEST_KEY]
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Execute: {value}'))
    return nx.Result()

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

  content = StringUtilities::replace(content, "@PYTHON_FILTER_NAME@", filterName);
  content = StringUtilities::replace(content, "@PYTHON_FILTER_HUMAN_NAME@", humanName);
  content = StringUtilities::replace(content, "@PYTHON_FILTER_UUID@", uuidString);

  return content;
}

/**
 *
 * @param outputPath
 * @param filterName
 * @param humanName
 * @param uuidString
 * @return
 */
inline Result<> WritePythonFilterToFile(const std::filesystem::path& outputPath, const std::string& filterName,
                                        const std::string& humanName, const std::string& uuidString)
{

  std::string content = GeneratePythonFilter(filterName, humanName, uuidString);
  auto outputFilePath = fmt::format("{}{}{}.py", outputPath.string(), std::filesystem::path::preferred_separator, filterName);
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
"""
Insert documentation here.
"""

@PLUGIN_IMPORT_CODE@

import simplnx as nx

class @PLUGIN_NAME@:
  def __init__(self) -> None:
    pass

  def id(self) -> nx.Uuid:
    return nx.Uuid('@PLUGIN_UUID@')

  def name(self) -> str:
    return '@PLUGIN_NAME@'

  def description(self) -> str:
    return '@PLUGIN_SHORT_NAME@'

  def vendor(self) -> str:
    return '@PLUGIN_DESCRIPTION@'

  def get_filters(self):
    return [@PLUGIN_FILTER_LIST@]


)";

inline const std::string k_PluginInitPythonFile = R"(
from @PLUGIN_NAME@.Plugin import @PLUGIN_NAME@

@PLUGIN_IMPORT_CODE@

def get_plugin():
  return @PLUGIN_NAME@()

__all__ = [@PLUGIN_FILTER_LIST@]

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
inline std::string GeneratePythonPlugin(const std::string& pluginName, const std::string& pluginShortName,
                                        const std::string& pluginDescription, const std::string& pluginFilterList)
{
  std::string content = k_PluginPythonFile;

  content = StringUtilities::replace(content, "@PLUGIN_NAME@", pluginName);
  content = StringUtilities::replace(content, "@PLUGIN_UUID@", Uuid::GenerateV4().str());
  content = StringUtilities::replace(content, "@PLUGIN_SHORT_NAME@", pluginShortName);
  content = StringUtilities::replace(content, "@PLUGIN_DESCRIPTION@", pluginDescription);

  auto filterList = StringUtilities::split(pluginFilterList, ',');
  content = StringUtilities::replace(content, "@PLUGIN_FILTER_LIST@", fmt::format("{}", fmt::join(filterList, ",")));

  std::string importStatements;
  for(const auto& name : filterList)
  {
    importStatements.append(fmt::format("from {}.{} import {}\n", pluginName, name, name));
  }

  content = StringUtilities::replace(content, "@PLUGIN_IMPORT_CODE@", importStatements);

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
inline Result<> WritePythonPluginFiles(const std::filesystem::path& outputDirectory, const std::string& pluginName,
                                        const std::string& pluginShortName,
                                       const std::string& pluginDescription, const std::string& pluginFilterList)
{

  auto pluginRootPath = outputDirectory / pluginName;
  auto result = nx::core::CreateOutputDirectories(pluginRootPath);
  if(result.invalid())
  {
    return result;
  }
  auto outputPath = pluginRootPath / "Plugin.py";
  {
    AtomicFile tempFile(outputPath, true);
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
    AtomicFile initTempFile(outputPath, true);
    {
      // Scope this so that the file closes first before we then 'commit' with the atomic file
      std::ofstream fout(initTempFile.tempFilePath(), std::ios_base::out | std::ios_base::binary);
      if(!fout.is_open())
      {
        return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", initTempFile.tempFilePath().string()));
      }
      std::string content = k_PluginInitPythonFile;

      content = StringUtilities::replace(content, "@PLUGIN_NAME@", pluginName);
      content = StringUtilities::replace(content, "@PLUGIN_UUID@", Uuid::GenerateV4().str());
      content = StringUtilities::replace(content, "@PLUGIN_SHORT_NAME@", pluginShortName);
      content = StringUtilities::replace(content, "@PLUGIN_DESCRIPTION@", pluginDescription);

      auto filterList = StringUtilities::split(pluginFilterList, ',');

      std::string aList(fmt::format("'{}', ",pluginName));
      for(const auto& name : filterList)
      {
        aList.append(fmt::format("'{}', ", name));
      }
      aList.append("'get_plugin'");
      content = StringUtilities::replace(content, "@PLUGIN_FILTER_LIST@", aList);

      std::string importStatements;
      for(const auto& name : filterList)
      {
        importStatements.append(fmt::format("from {}.{} import {}\n", pluginName, name, name));
      }

      content = StringUtilities::replace(content, "@PLUGIN_IMPORT_CODE@", importStatements);


      fout << content;
    }
  }
  // Now loop over each Filter and generate the skeleton files
  auto filterList = StringUtilities::split(pluginFilterList, ',');
  for(const auto& name : filterList)
  {
    WritePythonFilterToFile(pluginRootPath, name, "human " + name, Uuid::GenerateV4().str());
  }

  return {};
}


}
