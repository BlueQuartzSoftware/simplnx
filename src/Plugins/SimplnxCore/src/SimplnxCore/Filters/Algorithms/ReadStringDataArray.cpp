#include "ReadStringDataArray.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/Parsing/Text/CsvParser.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <cstdio>
#include <utility>

using namespace nx::core;

namespace
{
constexpr int32_t k_RBR_FILE_NOT_OPEN = -1000;
constexpr int32_t k_RBR_READ_EOF = -1030;
constexpr int32_t k_RBR_READ_ERROR = 1040;
constexpr int32_t k_RBR_FILE_NOT_EXIST = 1050;

constexpr size_t k_BufferSize = 10000;

std::string ConvertDelimiterIndexToString(uint64 index)
{
  switch(index)
  {
  case nx::core::read_string_data_array::k_CommaIndex:
    return {","};
  case nx::core::read_string_data_array::k_SemicolonIndex:
    return {";"};
  case nx::core::read_string_data_array::k_SpaceIndex:
    return {" "};
  case nx::core::read_string_data_array::k_ColonIndex:
    return {":"};
  case nx::core::read_string_data_array::k_TabIndex:
    return {"\t"};
  case nx::core::read_string_data_array::k_NewLineIndex:
    return {"\n"};
  }
  return {""};
}
} // End anonymous namespace

namespace nx::core
{
ReadStringDataArray::ReadStringDataArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                         const ReadStringDataArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ReadStringDataArray::~ReadStringDataArray() noexcept = default;

Result<> ReadStringDataArray::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Reading string data array...");

  auto& data = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->outputArrayPath);
  char delimiter = nx::core::CsvParser::IndexToDelimiter(m_InputValues->delimiterIndex);

  if(!std::filesystem::exists(m_InputValues->inputFileValue))
  {
    return MakeErrorResult(k_RBR_FILE_NOT_EXIST, fmt::format("Input file does not exist: {}", m_InputValues->inputFileValue.string()));
  }

  std::ifstream in(m_InputValues->inputFileValue.c_str(), std::ios_base::in | std::ios_base::binary);
  if(!in.is_open())
  {
    return MakeErrorResult(k_RBR_FILE_NOT_OPEN, fmt::format("Could not open file for reading: {}", m_InputValues->inputFileValue.string()));
  }

  in.imbue(std::locale(std::locale(), new nx::core::CsvParser::DelimiterType(delimiter)));

  std::string headerLine;
  // Skip some header line by just reading those bytes into the pointer knowing that the next
  // thing we are going to do it over write those bytes with the real data that we are after.
  for(int i = 0; i < m_InputValues->skipLineCount; i++)
  {
    std::getline(in, headerLine);
    if(in.fail())
    {
      return MakeErrorResult(k_RBR_READ_ERROR, fmt::format("Could not read data from file while skipping header lines: {}", m_InputValues->inputFileValue.string()));
    }
  }

  usize numTuples = data.getNumberOfTuples();
  usize scalarNumComp = data.getNumberOfComponents();

  usize totalSize = numTuples * scalarNumComp;
  usize lineNum = m_InputValues->skipLineCount;
  usize tupleIndex = 0;
  for(size_t i = 0; i < totalSize; ++i)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    std::string line;
    std::getline(in, line);
    line = StringUtilities::replace(line, "\r", "");
    std::vector<std::string> tokens = StringUtilities::split(line, ConvertDelimiterIndexToString(m_InputValues->delimiterIndex), false);
    if(tokens.empty())
    {
      // This is an empty line in the middle of the CSV file, which just shouldn't happen
      return MakeErrorResult(-76509, fmt::format("Line #{} is empty!  You should not have any empty lines in the file.", std::to_string(lineNum)));
    }

    // Copy the data into the array
    for(const auto& value : tokens)
    {
      data[tupleIndex++] = value;
    }

    lineNum++;
    // Have we read enough data from the file
    if(tupleIndex == totalSize)
    {
      break;
    }
  }

  return {};
}

} // namespace nx::core
