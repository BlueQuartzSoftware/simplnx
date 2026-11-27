#include "ReadCSVFile.hpp"

#include "simplnx/Utilities/FileUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

using namespace nx::core;

ReadCSVFile::ReadCSVFile() = default;

Result<> ReadCSVFile::readFile(DataStructure& dataStructure, const std::string& inputFilePath, usize importStartingRow, usize headersLineNumber, const std::vector<CSVType>& columnDataTypes,
                               const std::vector<bool>& columnsSkipped, const DataPath& groupPath, const ShapeType& tupleDims, const std::vector<char>& delimiters, bool consecutiveDelimiters,
                               const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& msgHandler)
{
  auto result = file_utilities::csv::ReadHeaders(inputFilePath, headersLineNumber, delimiters, consecutiveDelimiters);
  if(result.invalid())
  {
    return ConvertResult(std::move(result));
  }
  return ReadFile(dataStructure, inputFilePath, importStartingRow, result.value(), columnDataTypes, columnsSkipped, groupPath, tupleDims, delimiters, consecutiveDelimiters, shouldCancel, msgHandler);
}

Result<> ReadCSVFile::ReadFile(DataStructure& dataStructure, const std::string& inputFilePath, usize importStartingRow, const std::vector<std::string>& columnHeaders,
                               const std::vector<CSVType>& columnDataTypes, const std::vector<bool>& columnsSkipped, const DataPath& groupPath, const ShapeType& tupleDims,
                               const std::vector<char>& delimiters, bool consecutiveDelimiters, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& msgHandler)
{
  auto headers = columnHeaders;
  headers = file_utilities::csv::RemoveIllegalCharacters(headers);

  auto parsersResult = file_utilities::csv::CreateParsers(columnDataTypes, columnsSkipped, groupPath, headers, dataStructure);
  if(parsersResult.invalid())
  {
    return ConvertResult(std::move(parsersResult));
  }

  std::fstream in(inputFilePath, std::ios_base::in);
  if(!in.is_open())
  {
    return MakeErrorResult(to_underlying(IssueCodes::FILE_NOT_OPEN), fmt::format("Could not open file for reading: {}", inputFilePath));
  }

  // Position the stream at the first requested data row.
  if(!file_utilities::csv::SkipNumberOfLines(in, importStartingRow))
  {
    return MakeErrorResult(to_underlying(IssueCodes::CANNOT_SKIP_TO_LINE), fmt::format("Could not skip to the first line in the file to import ({}).", importStartingRow));
  }

  usize numTuples = std::accumulate(tupleDims.cbegin(), tupleDims.cend(), static_cast<usize>(1), std::multiplies<>());
  ThrottledMessageHandler progressThrottle(msgHandler);
  progressThrottle.reset(numTuples, "Importing CSV Data");
  usize lineNum = importStartingRow;
  for(usize i = 0; i < numTuples && !in.eof(); i++)
  {
    if(shouldCancel)
    {
      return file_utilities::csv::FlushParsers(parsersResult.value());
    }

    bool flushRequired = false;
    Result<> parsingResult = file_utilities::csv::ParseLine(in, parsersResult.value(), headers, delimiters, consecutiveDelimiters, lineNum, importStartingRow, flushRequired);
    if(parsingResult.invalid())
    {
      Result<> flushResult = file_utilities::csv::FlushParsers(parsersResult.value());
      return MergeResults(std::move(parsingResult), std::move(flushResult));
    }

    if(flushRequired)
    {
      Result<> flushResult = file_utilities::csv::FlushParsers(parsersResult.value());
      if(flushResult.invalid())
      {
        return flushResult;
      }
    }

    if(flushRequired || (i + 1) % 1024 == 0 || i + 1 == numTuples || in.eof())
    {
      progressThrottle.updatePercent(i + 1);
    }
    lineNum++;
  }

  return file_utilities::csv::FlushParsers(parsersResult.value());
}
