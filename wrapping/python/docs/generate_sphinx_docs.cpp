#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataObjectUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "plugin_source_dirs.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>

using namespace nx::core;

namespace
{

std::map<nx::core::Uuid, std::string> s_ParameterMap;

const std::string k_WebServerAddress = "http://www.dream3d.io/nx_reference_manual";
const std::string k_WebServerFilterFolder = "html";
} // namespace

#define ADD_PARAMETER_TRAIT(thing1, thing2)                                                                                                                                                            \
  {                                                                                                                                                                                                    \
    auto uuidResult = nx::core::Uuid::FromString(thing2);                                                                                                                                              \
    if(uuidResult.has_value())                                                                                                                                                                         \
    {                                                                                                                                                                                                  \
      ::s_ParameterMap[uuidResult.value()] = #thing1;                                                                                                                                                  \
    }                                                                                                                                                                                                  \
  }

void GenerateParameterList()
{
  ::s_ParameterMap.clear();
  ADD_PARAMETER_TRAIT(simplnx.OEMEbsdScanSelectionParameter, "3935c833-aa51-4a58-81e9-3a51972c05ea")
  ADD_PARAMETER_TRAIT(simplnx.ReadH5EbsdFileParameter, "FAC15aa6-b367-508e-bf73-94ab6be0058b")
  ADD_PARAMETER_TRAIT(simplnx.NumericTypeParameter, "a8ff9dbd-45e7-4ed6-8537-12dd53069bce")
  ADD_PARAMETER_TRAIT(simplnx.StringParameter, "5d6d1868-05f8-11ec-9a03-0242ac130003")
  ADD_PARAMETER_TRAIT(simplnx.DataStoreFormatParameter, "cfd5c150-2938-42a7-b023-4a9288fb6899")
  ADD_PARAMETER_TRAIT(simplnx.MultiPathSelectionParameter, "b5632f4f-fc13-4234-beb2-8fd8820eb6b6")
  ADD_PARAMETER_TRAIT(simplnx.DataTypeParameter, "d31358d5-3253-4c69-aff0-eb98618f851b")
  ADD_PARAMETER_TRAIT(simplnx.EnsembleInfoParameter, "10d3924f-b4c9-4e06-9225-ce11ec8dff89")
  ADD_PARAMETER_TRAIT(simplnx.ArrayThresholdsParameter, "e93251bc-cdad-44c2-9332-58fe26aedfbe")
  ADD_PARAMETER_TRAIT(simplnx.GenerateColorTableParameter, "7b0e5b25-564e-4797-b154-4324ef276bf0")
  ADD_PARAMETER_TRAIT(simplnx.DataObjectNameParameter, "fbc89375-3ca4-4eb2-8257-aad9bf8e1c94")
  ADD_PARAMETER_TRAIT(simplnx.NeighborListSelectionParameter, "ab0b7a7f-f9ab-4e6f-99b5-610e7b69fc5b")
  ADD_PARAMETER_TRAIT(simplnx.ChoicesParameter, "ee4d5ce2-9582-48fa-b182-8a766ce0feff")
  ADD_PARAMETER_TRAIT(simplnx.GeneratedFileListParameter, "aac15aa6-b367-508e-bf73-94ab6be0058b")
  ADD_PARAMETER_TRAIT(simplnx.DataPathSelectionParameter, "cd12d081-fbf0-46c4-8f4a-15e2e06e98b8")
  ADD_PARAMETER_TRAIT(simplnx.CalculatorParameter, "ba2d4937-dbec-5536-8c5c-c0a406e80f77")
  ADD_PARAMETER_TRAIT(simplnx.ReadCSVFileParameter, "4f6d6a33-48da-427a-8b17-61e07d1d5b45")

  ADD_PARAMETER_TRAIT(simplnx.MultiArraySelectionParameter, "d11e0bd8-f227-4fd1-b618-b6f16b259fc8")
  ADD_PARAMETER_TRAIT(simplnx.ArraySelectionParameter, "ab047a7f-f9ab-4e6f-99b5-610e7b69fc5b")
  ADD_PARAMETER_TRAIT(simplnx.DataGroupSelectionParameter, "bff3d4ac-04a6-5251-b178-4f83f7865074")
  ADD_PARAMETER_TRAIT(simplnx.AttributeMatrixSelectionParameter, "a3619d74-a1d9-4bc2-9e03-ca001d65b119")
  ADD_PARAMETER_TRAIT(simplnx.GeometrySelectionParameter, "3804cd7f-4ee4-400f-80ad-c5af17735de2")

  ADD_PARAMETER_TRAIT(simplnx.DataGroupCreationParameter, "bff2d4ac-04a6-5251-b188-4f83f7865074")
  ADD_PARAMETER_TRAIT(simplnx.ArrayCreationParameter, "ab047a7d-f81b-4e6f-99b5-610e7b69fc5b")

  ADD_PARAMETER_TRAIT(simplnx.FileSystemPathParameter, "f9a93f3d-21ef-43a1-a958-e57cbf3b2909")
  ADD_PARAMETER_TRAIT(simplnx.BoolParameter, "b6936d18-7476-4855-9e13-e795d717c50f")
  ADD_PARAMETER_TRAIT(simplnx.ReadHDF5DatasetParameter, "32e83e13-ee4c-494e-8bab-4e699df74a5a")
  ADD_PARAMETER_TRAIT(simplnx.Dream3dImportParameter, "170a257d-5952-4854-9a91-4281cd06f4f5")
  ADD_PARAMETER_TRAIT(simplnx.DynamicTableParameter, "eea76f1a-fab9-4704-8da5-4c21057cf44e")

  ADD_PARAMETER_TRAIT(simplnx.Int8Parameter, "cae73834-68f8-4235-b010-8bea87d8ff7a")
  ADD_PARAMETER_TRAIT(simplnx.UInt8Parameter, "6c3efeff-ce8f-47c0-83d1-262f2b2dd6cc")
  ADD_PARAMETER_TRAIT(simplnx.Int16Parameter, "44ae56e8-e6e7-4e4d-8128-dd3dc2c6696e")
  ADD_PARAMETER_TRAIT(simplnx.UInt16Parameter, "156a6f46-77e5-41d8-8f5a-65ba1da52f2a")
  ADD_PARAMETER_TRAIT(simplnx.Int32Parameter, "21acff45-a653-45db-a0d1-f43cd344b93a")
  ADD_PARAMETER_TRAIT(simplnx.UInt32Parameter, "e9521130-276c-40c7-95d7-0b4cb4f80649")
  ADD_PARAMETER_TRAIT(simplnx.Int64Parameter, "b2039349-bd3a-4dbb-93d2-b4b5c633e697")
  ADD_PARAMETER_TRAIT(simplnx.UInt64Parameter, "36d91b23-5500-4ed4-bdf3-d680f54ee5d1")
  ADD_PARAMETER_TRAIT(simplnx.Float32Parameter, "e4452dfe-2f70-4833-819e-0cbbec21289b")
  ADD_PARAMETER_TRAIT(simplnx.Float64Parameter, "f2a18fff-a095-47d7-b436-ede41b5ea21a")

  ADD_PARAMETER_TRAIT(simplnx.VectorInt8Parameter, "9f5f9683-e492-4a79-8378-79d727b2356a")
  ADD_PARAMETER_TRAIT(simplnx.VectorUInt8Parameter, "bff78ff3-35ef-482a-b3b1-df8806e7f7ef")
  ADD_PARAMETER_TRAIT(simplnx.VectorInt16Parameter, "43810a29-1a5f-4472-bec6-41de9ffe27f7")
  ADD_PARAMETER_TRAIT(simplnx.VectorUInt16Parameter, "2f1ba2f4-c5d5-403c-8b90-0bf60d2bde9b")
  ADD_PARAMETER_TRAIT(simplnx.VectorInt32Parameter, "d3188e18-e383-4727-ab32-88b5fda56ae8")
  ADD_PARAMETER_TRAIT(simplnx.VectorUInt32Parameter, "37322aa6-1a2f-4ecb-9aa1-8922d7ac1e49")
  ADD_PARAMETER_TRAIT(simplnx.VectorInt64Parameter, "4ceaffc1-7326-4f65-a33a-eae263dc22d1")
  ADD_PARAMETER_TRAIT(simplnx.VectorUInt64Parameter, "17309744-c4e8-4d1e-807e-e7012387f1ec")
  ADD_PARAMETER_TRAIT(simplnx.VectorFloat32Parameter, "88f231a1-7956-41f5-98b7-4471705d2805")
  ADD_PARAMETER_TRAIT(simplnx.VectorFloat64Parameter, "57cbdfdf-9d1a-4de8-95d7-71d0c01c5c96")
}

std::string ParseScalarType(const std::string& paramType)
{
  std::string temp = nx::core::StringUtilities::replace(paramType, "simplnx.", "Scalar Value |");
  temp = nx::core::StringUtilities::replace(temp, "Parameter", "");
  // temp = nx::core::StringUtilities::replace(temp, "Vector", " (Vector)");
  return temp;
}

// std::string ParseVectorType(const std::string& paramType)
//{
//   std::string temp = nx::core::StringUtilities::replace(paramType, "simplnx.", "");
//   temp = nx::core::StringUtilities::replace(temp, "Vector", "Vector of Values |");
//   temp = nx::core::StringUtilities::replace(temp, "Parameter", "");
//
//   return temp;
// }

bool CheckScalarPrimitive(std::ostream& rstStream, const nx::core::AnyParameter& paramValue)
{

  static const std::set<nx::core::Uuid> scalarPrimitives = {
      nx::core::Uuid::FromString("a8ff9dbd-45e7-4ed6-8537-12dd53069bce").value(), nx::core::Uuid::FromString("44ae56e8-e6e7-4e4d-8128-dd3dc2c6696e").value(),
      nx::core::Uuid::FromString("156a6f46-77e5-41d8-8f5a-65ba1da52f2a").value(), nx::core::Uuid::FromString("21acff45-a653-45db-a0d1-f43cd344b93a").value(),
      nx::core::Uuid::FromString("e9521130-276c-40c7-95d7-0b4cb4f80649").value(), nx::core::Uuid::FromString("b2039349-bd3a-4dbb-93d2-b4b5c633e697").value(),
      nx::core::Uuid::FromString("36d91b23-5500-4ed4-bdf3-d680f54ee5d1").value(), nx::core::Uuid::FromString("e4452dfe-2f70-4833-819e-0cbbec21289b").value(),
      nx::core::Uuid::FromString("f2a18fff-a095-47d7-b436-ede41b5ea21a").value(), nx::core::Uuid::FromString("e9521130-276c-40c7-95d7-0b4cb4f80649").value()};

  if(scalarPrimitives.find(paramValue->uuid()) != scalarPrimitives.end())
  {
    rstStream << "| " << paramValue->humanName() << " | " << ParseScalarType(s_ParameterMap[paramValue->uuid()]) << " | " << paramValue->helpText() << " |\n";
    return true;
  }
  return false;
}

bool CheckVectorPrimitive(std::ostream& rstStream, const nx::core::AnyParameter& paramValue)
{
  static const std::set<nx::core::Uuid> vectorPrimitives = {
      nx::core::Uuid::FromString("9f5f9683-e492-4a79-8378-79d727b2356a").value(), nx::core::Uuid::FromString("bff78ff3-35ef-482a-b3b1-df8806e7f7ef").value(),
      nx::core::Uuid::FromString("43810a29-1a5f-4472-bec6-41de9ffe27f7").value(), nx::core::Uuid::FromString("2f1ba2f4-c5d5-403c-8b90-0bf60d2bde9b").value(),
      nx::core::Uuid::FromString("d3188e18-e383-4727-ab32-88b5fda56ae8").value(), nx::core::Uuid::FromString("37322aa6-1a2f-4ecb-9aa1-8922d7ac1e49").value(),
      nx::core::Uuid::FromString("4ceaffc1-7326-4f65-a33a-eae263dc22d1").value(), nx::core::Uuid::FromString("17309744-c4e8-4d1e-807e-e7012387f1ec").value(),
      nx::core::Uuid::FromString("88f231a1-7956-41f5-98b7-4471705d2805").value(), nx::core::Uuid::FromString("57cbdfdf-9d1a-4de8-95d7-71d0c01c5c96").value()};

  std::string paramTypeName = nx::core::StringUtilities::replace(s_ParameterMap[paramValue->uuid()], "simplnx.", "");
  paramTypeName = nx::core::StringUtilities::replace(paramTypeName, "Vector", "");
  paramTypeName = nx::core::StringUtilities::replace(paramTypeName, "Parameter", "");

  if(vectorPrimitives.find(paramValue->uuid()) != vectorPrimitives.end())
  {
    const VectorParameterBase* paramPtr = dynamic_cast<const VectorParameterBase*>(paramValue.get());
    rstStream << "| " << paramValue->humanName() << " | Vector of " << paramTypeName << " Values | Order=";

    auto names = paramPtr->names();
    int i = 0;
    for(const auto& name : names)
    {
      rstStream << name;
      i++;
      if(i != names.size())
      {
        rstStream << ",";
      }
    }

    rstStream << " | " << paramValue->helpText() << " |\n";
    return true;
  }
  return false;
}

/**
 *
 * @param rstStream
 * @param paramValue
 * @return
 */
bool CheckArraySelectionParameter(std::ostream& rstStream, const nx::core::AnyParameter& paramValue)
{
  static const std::set<nx::core::Uuid> param = {nx::core::Uuid::FromString("ab047a7f-f9ab-4e6f-99b5-610e7b69fc5b").value()};

  std::string paramTypeName = s_ParameterMap[paramValue->uuid()];
  paramTypeName = nx::core::StringUtilities::replace(paramTypeName, "simplnx.", "");
  paramTypeName = nx::core::StringUtilities::replace(paramTypeName, "Parameter", "");
  paramTypeName = nx::core::StringUtilities::replace(paramTypeName, "Selection", " Selection");
  const ArraySelectionParameter* paramPtr = dynamic_cast<const ArraySelectionParameter*>(paramValue.get());
  if(paramPtr != nullptr)
  {
    rstStream << "| " << paramValue->humanName() << " | " << paramTypeName << " | Allowed Types: ";
    auto allowedTypes = paramPtr->allowedTypes();
    int i = 0;
    for(const auto& allowedType : allowedTypes)
    {
      rstStream << nx::core::DataTypeToString(allowedType).str();
      i++;
      if(i != allowedTypes.size())
      {
        rstStream << ", ";
      }
    }
    auto compShapes = paramPtr->requiredComponentShapes();
    if(!compShapes.empty())
    {
      rstStream << " Comp. Shape: ";
    }
    for(const auto& compShape : compShapes)
    {
      rstStream << fmt::format("{}", fmt::join(compShape, ","));
    }
    rstStream << " | " << paramValue->helpText() << " |\n";
    return true;
  }

  return false;
}

bool CheckGeometrySelectionParameter(std::ostream& rstStream, const nx::core::AnyParameter& paramValue)
{
  static const std::set<nx::core::Uuid> param = {nx::core::Uuid::FromString("d11e0bd8-f227-4fd1-b618-b6f16b259fc8").value()};

  std::string paramTypeName = s_ParameterMap[paramValue->uuid()];
  paramTypeName = nx::core::StringUtilities::replace(paramTypeName, "simplnx.", "");
  paramTypeName = nx::core::StringUtilities::replace(paramTypeName, "Parameter", "");
  paramTypeName = nx::core::StringUtilities::replace(paramTypeName, "Selection", " Selection");
  const GeometrySelectionParameter* paramPtr = dynamic_cast<const GeometrySelectionParameter*>(paramValue.get());
  if(paramPtr != nullptr)
  {

    rstStream << "| " << paramValue->humanName() << " | " << paramTypeName << " | ";

    auto allowedTypes = paramPtr->allowedTypes();
    int i = 0;
    for(const auto& allowedType : allowedTypes)
    {
      rstStream << nx::core::GeometryTypeToString(allowedType).str();
      i++;
      if(i != allowedTypes.size())
      {
        rstStream << ", ";
      }
    }

    rstStream << " | " << paramValue->helpText() << " |\n";
    return true;
  }

  return false;
}

/**
 *
 * @param rstStream
 * @param paramValue
 */
void CheckParameter(std::ostream& rstStream, const nx::core::AnyParameter& paramValue)
{
  std::string paramTypeName = s_ParameterMap[paramValue->uuid()];
  paramTypeName = nx::core::StringUtilities::replace(paramTypeName, "simplnx.", "");
  paramTypeName = nx::core::StringUtilities::replace(paramTypeName, "Parameter", " | ");

  rstStream << "| " << paramValue->humanName() << " | " << paramTypeName << " | " << paramValue->helpText() << " |\n";
}

/**
 *
 * @param filePath
 * @param trimLine
 * @return
 */
std::vector<std::string> ReadFile(const std::filesystem::path& filePath, bool trimLine)
{
  std::ifstream file = std::ifstream(filePath);
  std::vector<std::string> lines;

  if(!file.is_open())
  {
    std::cerr << "Failed to open the file: " << filePath << std::endl;
    return lines;
  }

  std::string line;
  while(std::getline(file, line))
  {
    if(trimLine)
    {
      line = nx::core::StringUtilities::trimmed(line);
    }
    lines.push_back(line);
  }

  file.close();
  return lines;
}

std::string GenerateUnderline(size_t length, char value)
{
  std::string result;
  for(size_t i = 0; i < length; ++i)
  {
    result.push_back(value);
  }
  return result;
}

std::string MarkdownToRstLink(const std::string& markDown)
{
  std::string rst = markDown;

  // Find the start of a Markdown link
  size_t start = rst.find('[');
  while(start != std::string::npos)
  {
    size_t const endText = rst.find(']', start);
    if(endText == std::string::npos)
    {
      break; // No matching ']'
    }

    size_t const startUrl = rst.find('(', endText);
    if(startUrl == std::string::npos)
    {
      break; // No matching '(' after ']'
    }
    size_t const endUrl = rst.find(')', startUrl);
    if(endUrl == std::string::npos)
    {
      break; // No matching ')'
    }

    // Extract link text and URL
    const std::string linkText = rst.substr(start + 1, endText - start - 1);
    const std::string url = rst.substr(startUrl + 1, endUrl - startUrl - 1);

    // Replace with reStructuredText format
    const std::string rstLink = fmt::format("`{} <{}>`_", linkText, url);
    rst.replace(start, endUrl - start + 1, rstLink);

    // Find the next link
    start = rst.find('[', endUrl);
  }

  return rst;
}

std::string CreateFilledString(const std::string& input_string, int max_width, char fill_value)
{
  if(max_width <= 0)
  {
    return "";
  }

  if(input_string.size() > max_width)
  {
    throw std::invalid_argument("Input string is longer than the specified max_width.");
  }

  // Create an initial string filled with 'fill_value' of size 'max_width'
  std::string output(max_width, fill_value);

  // The input_string should start from the second character, so calculate the actual
  // maximum characters we can copy from input_string
  const int actualCopyWidth = std::min(max_width - 1, static_cast<int>(input_string.size()));

  // Copy the relevant part of the input_string into the output string
  std::copy_n(input_string.begin(), actualCopyWidth, output.begin() + 1);

  return output;
}

std::string MarkdownToRst(const std::string& markDown)
{
  std::string rst = markDown;

  // Convert headers
  std::smatch headerMatch;
  std::regex const headerRegex("^# (.+)$");
  if(std::regex_search(rst, headerMatch, headerRegex))
  {
    std::string const title = headerMatch[1];
    rst = std::regex_replace(rst, headerRegex, ".. title:: " + title);
  }

  rst = std::regex_replace(rst, std::regex("^## (.+)$"), "$1\n" + GenerateUnderline(40, '-'));
  rst = std::regex_replace(rst, std::regex("^### (.+)$"), "$1\n" + GenerateUnderline(40, '^'));

  // Convert bold text
  rst = std::regex_replace(rst, std::regex("\\*\\*(.+)\\*\\*"), "**$1**");

  // Convert italic text
  rst = std::regex_replace(rst, std::regex("\\*(.+)\\*"), "*$1*");

  // Convert unordered lists
  rst = std::regex_replace(rst, std::regex("^\\s*[-*+] (.+)$"), "* $1");

  // Convert ordered lists
  rst = std::regex_replace(rst, std::regex("^\\s*\\d+\\. (.+)$"), "#. $1");

  // Convert List
  // rst = std::regex_replace(rst, std::regex("\\[(.+)\\]\\((.+)\\)"), "`$1 <$2>`_");

  // Convert links
  // rst = std::regex_replace(rst, std::regex("\\[(.+)\\]\\((.+)\\)"), "`$1 <$2>`_");
  rst = MarkdownToRstLink(rst);

  // Convert images
  //  rst = std::regex_replace(rst, std::regex("!\\[(.*)\\]\\((.+?)\\)"), ".. image:: $2\n   :alt: $1");

  // Convert inline code
  // rst = std::regex_replace(rst, std::regex("`(.+?)`"), "``$1``");

  // Convert code blocks
  rst = std::regex_replace(rst, std::regex("```\n?(.+?)\n?```"), "::\n\n   $1");
  rst = std::regex_replace(rst, std::regex("    (.+?)\n"), "|$1\n");

  return rst;
}

std::string ExtractRstFilterSummary(const std::filesystem::path& docFilePath)
{
  // Open the existing doc file
  // Extract out the first paragraph of the Filter's markdown documentation after the ## Description section marker
  std::vector<std::string> mdLines = ReadFile(docFilePath, true);
  std::stringstream mdStream;
  bool extractLines = false;
  int emptyLine = 0;
  for(const auto& line : mdLines)
  {
    if(nx::core::StringUtilities::starts_with(line, "## Description"))
    {
      extractLines = true;
      continue;
    }
    if(line.empty() && extractLines)
    {
      emptyLine++;
    }

    if(nx::core::StringUtilities::starts_with(line, "##"))
    {
      extractLines = false;
    }
    if(extractLines)
    {
      mdStream << line << '\n';
    }
    if(emptyLine > 1)
    {
      break;
    }
  }

  std::string rstDescription = MarkdownToRst(mdStream.str());
  rstDescription = nx::core::StringUtilities::replace(rstDescription, "\n", "");
  rstDescription = nx::core::StringUtilities::trimmed(rstDescription);

  return rstDescription;
}

std::vector<int32_t> FindTableColumnWidths(const Parameters& parameters)
{
  int32_t maxUIDisplay = 10;
  int32_t maxPythonArg = 21;
  for(const auto& parameterPair : parameters)
  {
    auto const& anyParameter = parameterPair.second;
    if(anyParameter->humanName().size() > maxUIDisplay)
    {
      maxUIDisplay = static_cast<int32_t>(anyParameter->humanName().size());
    }
    if(anyParameter->name().size() > maxPythonArg)
    {
      maxPythonArg = static_cast<int32_t>(anyParameter->name().size());
    }
  }
  maxUIDisplay += 2;
  maxPythonArg += 2;
  return {maxUIDisplay, maxPythonArg};
}

std::vector<std::string> ConvertFilterDoc(const std::filesystem::path& docFilePath)
{
  // Open the existing doc file
  // Extract out the first paragraph of the Filter's markdown documentation after the ## Description section marker
  std::vector<std::string> mdLines = ReadFile(docFilePath, false);
  std::vector<std::string> outputLines;
  std::stringstream mdStream;
  bool extractLines = true;
  for(const auto& line : mdLines)
  {
    if(nx::core::StringUtilities::starts_with(line, "## Parameters"))
    {
      extractLines = false;
      continue;
    }

    if(nx::core::StringUtilities::starts_with(line, "## Example Pipelines"))
    {
      extractLines = true;
      outputLines.push_back("@PARAMETER_TABLE@");
    }

    if(!extractLines && nx::core::StringUtilities::starts_with(line, "## License & Copyright"))
    {
      extractLines = true;
      outputLines.push_back("@PARAMETER_TABLE@");
    }
    if(extractLines)
    {
      outputLines.push_back(line);
    }
  }

  return outputLines;
}

void GenerateMarkdownFilterParameterTable()
{
  auto* filterListPtr = Application::Instance()->getFilterList();

  // Loop over each plugin and create a .rst file
  const auto pluginListPtr = Application::Instance()->getPluginList();
  for(const auto& plugin : pluginListPtr)
  {
    std::string plugName = plugin->getName();
    const std::string pluginRootDir = fmt::format("{}", s_PluginDirMap[plugName]);
    if(pluginRootDir.empty())
    {
      continue;
    }

    // This will alphabetize the filter list
    const auto& pluginFilterHandles = plugin->getFilterHandles();
    std::map<std::string, std::pair<FilterHandle::FilterIdType, FilterHandle::PluginIdType>> filterHandles;
    for(const auto& filterHandle : pluginFilterHandles)
    {
      filterHandles[filterHandle.getClassName()] = {filterHandle.getFilterId(), filterHandle.getPluginId()};
    }

    // Loop on each Filter
    for(const auto& filterHandleIter : filterHandles)
    {
      std::string filterClassName = filterHandleIter.first;
      FilterHandle const filterHandle(filterHandleIter.second.first, filterHandleIter.second.second);
      IFilter::UniquePointer filter = filterListPtr->createFilter(filterHandle);

      std::stringstream pTableStream;

      // std::vector<int32_t> columnWidths = FindTableColumnWidths(parameters);
      int needTableHeader = 0;

      const auto& parameters = filter->parameters();

      for(const auto& layoutObj : parameters.getLayout())
      {
        if(std::holds_alternative<nx::core::Parameters::ParameterKey>(layoutObj))
        {

          if(needTableHeader == 0)
          {
            pTableStream << "### Filter Parameters\n\n";
            pTableStream << "| Parameter Name | Parameter Type | Parameter Notes | Description |\n";
            pTableStream << "|----------------|----------------|-----------------|-------------|\n";
          }

          std::string key = std::get<nx::core::Parameters::ParameterKey>(layoutObj).key;
          const auto& paramValue = parameters.at(key);
          if(CheckScalarPrimitive(pTableStream, paramValue))
          {
            continue;
          }
          if(CheckVectorPrimitive(pTableStream, paramValue))
          {
            continue;
          }
          if(CheckArraySelectionParameter(pTableStream, paramValue))
          {
            continue;
          }
          if(CheckGeometrySelectionParameter(pTableStream, paramValue))
          {
            continue;
          }
          CheckParameter(pTableStream, paramValue);
        }
        else
        {
          std::string key = std::get<nx::core::Parameters::Separator>(layoutObj).name;
          pTableStream << "\n";
          pTableStream << "### " << key << "\n\n";
          pTableStream << "| Parameter Name | Parameter Type | Parameter Notes | Description |\n";
          pTableStream << "|----------------|----------------|-----------------|-------------|\n";
        }
        needTableHeader++;
      }



      pTableStream << '\n';

      // Read the Current Filter's Documentation File
      const std::filesystem::path docFilePath = fmt::format("{}/docs/{}.md", pluginRootDir, filterClassName);
      std::vector<std::string> filterDocContents = ConvertFilterDoc(docFilePath);

      // Create a new output file
      const std::filesystem::path rstFilePath = fmt::format("{}/nx_docs/source/{}/{}.md", SIMPLNX_BUILD_DIR, plugName, filterClassName);
      const Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(rstFilePath.parent_path());

      std::ofstream rstStream = std::ofstream(rstFilePath, std::ios_base::binary | std::ios_base::trunc);
      if(!rstStream.is_open())
      {
        std::cout << "ERROR:" << rstFilePath << "\n";
        return;
      }

      rstStream << "```{index} single: Filters; " << filter->humanName() << "\n```\n";

      for(const auto& line : filterDocContents)
      {
        if(line == "@PARAMETER_TABLE@")
        {
          rstStream << pTableStream.str();
        }
        else
        {
          rstStream << line << "\n";
        }
      }
    }
  }
}

/**
 * @brief
 */
void GeneratePythonRstFiles()
{
  auto* filterListPtr = Application::Instance()->getFilterList();

  // Loop over each plugin and create a .rst file
  const auto pluginListPtr = Application::Instance()->getPluginList();
  for(const auto& plugin : pluginListPtr)
  {
    std::string plugName = plugin->getName();
    const std::string pluginRootDir = fmt::format("{}", s_PluginDirMap[plugName]);
    if(pluginRootDir.empty())
    {
      continue;
    }

    if(plugName == "SimplnxCore")
    {
      plugName = "simplnx";
    }

    // const std::filesystem::path rstFilePath = fmt::format("{}/sphinx_docs/source/{}.rst", SIMPLNX_BUILD_DIR, plugName);
    const std::filesystem::path rstFilePath = fmt::format("{}/wrapping/python/docs/source/{}.rst", SIMPLNX_BUILD_DIR, plugName);

    const Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(rstFilePath.parent_path());
    std::ofstream rstStream = std::ofstream(rstFilePath, std::ios_base::binary | std::ios_base::trunc);
    if(!rstStream.is_open())
    {
      std::cout << "ERROR Opening file:" << rstFilePath << "\n *** Aborting filter documentation generation!!\n";
      return;
    }

    rstStream << plugName << " Filter API"
              << "\n";
    rstStream << GenerateUnderline(plugName.size(), '=') << "============="
              << "\n\n";

    rstStream << "   This is the documentation for all filters included in the " << plugName << " module. These filters can"
              << " be used by importing the appropriate module. Each filter is contained in the module:\n\n"
              << ".. code:: python\n\n"
              << "   import " << nx::core::StringUtilities::toLower(plugName) << "\n"
              << "   # Instantiate and execute a filter from the module\n"
              << "   result = " << nx::core::StringUtilities::toLower(plugName) << ".SomeFilterName.execute(...)\n"
              << "\n";

    rstStream << ".. py:module:: " << plugName << "\n\n";

    // This will alphabetize the filter list
    const auto& pluginFilterHandles = plugin->getFilterHandles();
    std::map<std::string, std::pair<FilterHandle::FilterIdType, FilterHandle::PluginIdType>> filterHandles;
    for(const auto& filterHandle : pluginFilterHandles)
    {
      filterHandles[filterHandle.getClassName()] = {filterHandle.getFilterId(), filterHandle.getPluginId()};
    }

    // Loop on each Filter
    for(const auto& filterHandleIter : filterHandles)
    {
      std::string filterClassName = filterHandleIter.first;
      FilterHandle const filterHandle(filterHandleIter.second.first, filterHandleIter.second.second);
      IFilter::UniquePointer filter = filterListPtr->createFilter(filterHandle);
      // auto plugin = filterListPtr->getPlugin(filterHandle);

      rstStream << filterClassName << "\n";
      rstStream << GenerateUnderline(filterClassName.size(), '-') << "\n\n";
      rstStream << ".. index:: pair: Filter Human Names; " << filter->humanName() << "\n";
      rstStream << ".. index:: pair: Filter Class Names; " << filter->className() << "\n";

      rstStream << "\n";
      rstStream << "-  **UI Name**: " << filter->humanName() << "\n\n";

      rstStream << ".. _" << filterClassName << ":\n";

      rstStream << ".. py:class:: " << filterClassName << "\n\n";

      // Extract out the first paragraph of the Filter's markdown documentation after the ## Description section marker
      const std::filesystem::path docFilePath = fmt::format("{}/docs/{}.md", pluginRootDir, filterClassName);
      std::string rstDescription = ExtractRstFilterSummary(docFilePath);
      rstStream << "   " << rstDescription << "\n\n";

      // Tack on the link to the web address for the full documentation
      const std::string webAddress = fmt::format("{}/{}/{}/{}.html", k_WebServerAddress, k_WebServerFilterFolder, plugName, filterClassName);
      rstStream << "   `Link to the full online documentation for " << filterClassName << " <" << webAddress << ">`_ \n\n";

      const auto& parameters = filter->parameters();

      // First loop through the parameters to get the max string length to build up the rst table correctly
      //      size_t maxUIDisplay = 10;
      //      size_t maxPythonArg = 21;
      std::vector<int32_t> columnWidths = FindTableColumnWidths(parameters);

      rstStream << "   Mapping of UI display to python named argument\n\n";
      rstStream << "   +" << std::string(columnWidths[0], '-') << "+" << std::string(columnWidths[1], '-') << "+\n";
      rstStream << "   |" << CreateFilledString("UI Display", columnWidths[0], ' ') << "|" << CreateFilledString("Python Named Argument", columnWidths[1], ' ') << "|" << '\n';
      rstStream << "   +" << std::string(columnWidths[0], '=') << "+" << std::string(columnWidths[1], '=') << "+\n";
      for(const auto& parameter : parameters)
      {
        auto const& paramValue = parameter.second;

        rstStream << "   |" << CreateFilledString(paramValue->humanName(), columnWidths[0], ' ') << "|" << CreateFilledString(paramValue->name(), columnWidths[1], ' ') << "|\n";
        rstStream << "   +" << std::string(columnWidths[0], '-') << "+" << std::string(columnWidths[1], '-') << "+\n";
      }
      rstStream << '\n';

      rstStream << "   .. py:method:: Execute(data_structure";

      std::stringstream memberStream;
      memberStream << "      :param DataStructure data_structure: The DataStructure object that holds the data to be processed.\n";

      size_t index = 0;
      for(const auto& parameterPair : parameters)
      {
        auto const& anyParameter = parameterPair.second;
        rstStream << ", ";

        rstStream << parameterPair.first;
        memberStream << "      :param nx." << nx::core::StringUtilities::replace(s_ParameterMap[anyParameter->uuid()], "simplnx.", "") << " " << anyParameter->name() << ": "
                     << anyParameter->helpText() << "\n";
        index++;
      }
      rstStream << ")\n\n";

      rstStream << memberStream.str();
      rstStream << "      :return: Returns a :ref:`Result <result>` object that holds any warnings and/or errors that were encountered during execution.\n";
      rstStream << "      :rtype: :ref:`simplnx.Result <result>`\n\n";
      rstStream << '\n';
    }
  }
}

/**
 * @brief This will read the index template file in order to fill in the plugins that are being
 * used.
 * @param path The path to the index template file
 * @return
 */
std::string ReadIndexTemplateFile(const std::filesystem::path& path)
{
  std::ifstream file(path);

  if(!file.is_open())
  {
    throw std::runtime_error("Failed to open file: " + path.string());
  }

  // Read the entire content of the file into a string
  const std::string content(std::istreambuf_iterator<char>(file), {});

  return content;
}

/**
 * @brief Generates the index.rst file for the sphinx docs
 */
void GeneratePythonSphinxIndex()
{
  std::string indexTemplate;
  {
    const std::filesystem::path rstIndexTemplatePath = fmt::format("{}/wrapping/python/docs/index_template.rst", SIMPLNX_SOURCE_DIR);
    indexTemplate = ReadIndexTemplateFile(rstIndexTemplatePath);
  }

  // Loop over each plugin and fill in the list
  const auto pluginListPtr = Application::Instance()->getPluginList();

  std::stringstream pluginList;

  for(const auto& plugin : pluginListPtr)
  {
    std::string plugName = plugin->getName();
    if(plugName == "TestOne" || plugName == "TestTwo")
    {
      continue;
    }
    const std::string pluginRootDir = fmt::format("{}", s_PluginDirMap[plugName]);

    if(plugName == "SimplnxCore")
    {
      plugName = "simplnx";
    }
    pluginList << "   " << plugName << "\n";
  }

  indexTemplate = nx::core::StringUtilities::replace(indexTemplate, "@PLUGIN_LIST@", pluginList.str());

  const std::filesystem::path rstFilePath = fmt::format("{}/wrapping/python/docs/source/index.rst", SIMPLNX_BUILD_DIR);
  std::ofstream rstStream = std::ofstream(rstFilePath, std::ios_base::binary | std::ios_base::trunc);
  if(!rstStream.is_open())
  {
    std::cout << "ERROR:" << rstFilePath << "\n";
    return;
  }
  rstStream << indexTemplate;
}

/**
 * @brief Main point of entry into this utility program.
 * @param argc
 * @param argv
 * @return
 */
int main(int32_t argc, char** argv)
{
  // try
  {
    GenerateParameterList();

    auto appPtr = nx::core::Application::GetOrCreateInstance();
    appPtr->loadPlugins(SIMPLNX_BIN_DIR, true);

    GeneratePythonRstFiles();
    GeneratePythonSphinxIndex();

    GenerateMarkdownFilterParameterTable();
  }
  // catch(const std::exception& except)
  {
    // std::cout << "RST Doc Generator threw a runtime exception which should NOT happen" << std::endl;
  }
  return 0;
}
