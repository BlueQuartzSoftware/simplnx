#include "ReadCSVFile.hpp"

#include "simplnx/Utilities/FileUtilities.hpp"

using namespace nx::core;

namespace
{
void notifyProgress(usize lineNumber, usize numberOfTuples, float32& threshold, const IFilter::MessageHandler& msgHandler)
{
  const float32 percentCompleted = (static_cast<float32>(lineNumber) / static_cast<float32>(numberOfTuples)) * 100.0f;
  if(percentCompleted > threshold)
  {
    // Print the status of the import
    msgHandler.sendInfoMessage(fmt::format("Importing CSV Data || {:.{}f}% Complete", static_cast<double>(percentCompleted), 1));
    threshold = threshold + 5.0f;
    if(threshold < percentCompleted)
    {
      threshold = percentCompleted;
    }
  }
}
} // End anonymous namespace

ReadCSVFile::ReadCSVFile() = default;

ReadCSVFile::~ReadCSVFile() noexcept = default;

Result<> ReadCSVFile::readFile(DataStructure& dataStructure, const std::string& inputFilePath, usize importStartingRow, usize headersLineNumber, const std::vector<CSVType>& columnDataTypes,
                               const std::vector<bool>& columnsSkipped, const DataPath& groupPath, const ShapeType& tupleDims, const std::vector<char>& delimiters, bool consecutiveDelimiters,
                               const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& msgHandler)
{
  auto result = FileUtilities::CSV::ReadHeaders(inputFilePath, headersLineNumber, delimiters, consecutiveDelimiters);
  if(result.invalid())
  {
    return ConvertResult(std::move(result));
  }
  return readFile(dataStructure, inputFilePath, importStartingRow, result.value(), columnDataTypes, columnsSkipped, groupPath, tupleDims, delimiters, consecutiveDelimiters, shouldCancel, msgHandler);
}

Result<> ReadCSVFile::readFile(DataStructure& dataStructure, const std::string& inputFilePath, usize importStartingRow, const std::vector<std::string>& columnHeaders,
                               const std::vector<CSVType>& columnDataTypes, const std::vector<bool>& columnsSkipped, const DataPath& groupPath, const ShapeType& tupleDims,
                               const std::vector<char>& delimiters, bool consecutiveDelimiters, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& msgHandler)
{
  auto headers = columnHeaders;
  headers = FileUtilities::CSV::RemoveIllegalCharacters(headers);

  auto parsersResult = FileUtilities::CSV::CreateParsers(columnDataTypes, columnsSkipped, groupPath, headers, dataStructure);
  if(parsersResult.invalid())
  {
    return ConvertResult(std::move(parsersResult));
  }

  std::fstream in(inputFilePath, std::ios_base::in);
  if(!in.is_open())
  {
    return MakeErrorResult(to_underlying(IssueCodes::FILE_NOT_OPEN), fmt::format("Could not open file for reading: {}", inputFilePath));
  }

  // Skip to the first data line
  if(!FileUtilities::CSV::SkipNumberOfLines(in, importStartingRow))
  {
    return MakeErrorResult(to_underlying(IssueCodes::CANNOT_SKIP_TO_LINE), fmt::format("Could not skip to the first line in the file to import ({}).", importStartingRow));
  }

  float32 threshold = 0.0f;
  usize numTuples = std::accumulate(tupleDims.cbegin(), tupleDims.cend(), static_cast<usize>(1), std::multiplies<>());
  usize lineNum = importStartingRow;
  for(usize i = 0; i < numTuples && !in.eof(); i++)
  {
    if(shouldCancel)
    {
      return {};
    }

    Result<> parsingResult = FileUtilities::CSV::ParseLine(in, parsersResult.value(), headers, delimiters, consecutiveDelimiters, lineNum, importStartingRow);
    if(parsingResult.invalid())
    {
      return std::move(parsingResult);
    }

    notifyProgress(lineNum, numTuples, threshold, msgHandler);
    lineNum++;
  }

  return {};
}
