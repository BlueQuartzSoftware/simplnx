#include "FileUtilities.hpp"

#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <array>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <io.h>
#define FSPP_ACCESS_FUNC_NAME _access
#else
#include <unistd.h>
#define FSPP_ACCESS_FUNC_NAME access
#endif

namespace fs = std::filesystem;

namespace
{
#ifdef _WIN32
constexpr int k_CheckWritable = 2;
#else
constexpr int k_CheckWritable = W_OK;
#endif

constexpr int k_HasAccess = 0;
}; // namespace

namespace nx::core::FileUtilities
{
int64 LinesInFile(const std::string& filepath)
{
  const usize BUFFER_SIZE = 16384;
  usize lines = 0;

  FILE* fd = fopen(filepath.c_str(), "rb");
  if(nullptr == fd)
  {
    return -1;
  }

  // Count a final line that has no newline terminator.
  fseek(fd, -1, SEEK_END);
  char last[1];
  usize bytesRead = fread(last, 1, 1, fd);
  if(bytesRead != 1)
  {
    return -1;
  }
  if(last[0] != '\n')
  {
    lines++;
  }
  rewind(fd);

  char buf[BUFFER_SIZE + 1];
  while(true)
  {
    memset(buf, 0, BUFFER_SIZE + 1);
    size_t bytes_read = fread(buf, 1, BUFFER_SIZE, fd);
    if(bytes_read == 0)
    {
      break;
    }

    char* end = buf + bytes_read;
    usize buflines = 0;

    for(char* p = buf; p < end; p++)
    {
      buflines += *p == '\n';
    }
    lines += buflines;
  }
  fclose(fd);
  return lines;
}

Result<> ValidateCSVFile(const std::string& filePath)
{
  constexpr int64_t bufferSize = 2048;

  auto absPath = std::filesystem::absolute(filePath);

  if(!std::filesystem::exists({absPath}))
  {
    return MakeErrorResult(-300, fmt::format("CSV file does not exist: {}", absPath.string()));
  }
  if(std::filesystem::is_directory({absPath}))
  {
    return MakeErrorResult(-301, fmt::format("CSV input file is a directory: {}", absPath.string()));
  }

  usize fileSize = std::filesystem::file_size(absPath);

  std::ifstream in(absPath.c_str(), std::ios_base::binary);
  if(!in.is_open())
  {
    return MakeErrorResult(-301, fmt::format("Could not open file for reading: {}", absPath.string()));
  }

  auto isUtf8 = IsUtf8(absPath);
  if(isUtf8.first)
  {
    // Exclude the three-byte UTF-8 byte-order mark from the text probe.
    char a = '\0';
    char b = '\0';
    char c = '\0';
    in >> a >> b >> c;
    fileSize -= 3;
  }

  usize actualSize = bufferSize;
  if(fileSize <= bufferSize)
  {
    actualSize = fileSize;
  }

  std::vector<char> buffer(actualSize, 0);

  try
  {
    in.read(buffer.data(), actualSize);
  } catch(const std::exception& e)
  {
    return MakeErrorResult(-302, fmt::format("There was an error reading the data from file: {}.  Exception: {}", absPath.string(), e.what()));
  }

  bool hasNewLines = false;
  bool hasCarriageReturns = false;
  bool hasTabs = false;
  // A line longer than the bounded probe can contain no observed delimiter and
  // can produce the possible-binary error below.
  for(size_t i = 0; i < actualSize; i++)
  {
    const char currentChar = buffer[i];

    if(currentChar < 32 && currentChar != 9 && currentChar != 10 && currentChar != 13)
    {
      return MakeErrorResult(-303, fmt::format("Unprintable characters have been detected in file: {}.  Please import a different file.", absPath.string()));
    }
    if(currentChar == 9)
    {
      hasTabs = true;
    }
    else if(currentChar == 10)
    {
      hasNewLines = true;
    }
    else if(currentChar == 13)
    {
      hasCarriageReturns = true;
    }
  }

  if(!hasNewLines && !hasCarriageReturns && !hasTabs)
  {
    return MakeErrorResult(-304, fmt::format("The file \"{}\" might be a binary file, because line-feed, tab, or carriage return characters have not been detected. Using this file may crash the "
                                             "program or cause unexpected results.  Please import a different file.",
                                             absPath.string()));
  }

  return {};
}

bool HasWriteAccess(const std::string& path)
{
  return FSPP_ACCESS_FUNC_NAME(path.c_str(), k_CheckWritable) == k_HasAccess;
}

Result<> ValidateDirectoryWritePermission(const std::filesystem::path& path, bool isFile)
{
  if(path.empty())
  {
    return MakeErrorResult(-16, "ValidateDirectoryWritePermission() Error: Input path empty.");
  }

  auto checkedPath = path;
  auto parentPath = checkedPath.parent_path();
  if(isFile && !parentPath.empty())
  {
    checkedPath = parentPath;
  }
  // Resolve relative input before the ancestor walk so the root is explicit.
  if(!checkedPath.is_absolute())
  {
    try
    {
      checkedPath = std::filesystem::absolute(checkedPath);
    } catch(const std::filesystem::filesystem_error& error)
    {
      return MakeErrorResult(-15, fmt::format("ValidateDirectoryWritePermission() Error: Input Path '{}' was relative and trying to create an absolute path threw an exception with message '{}'. "
                                              "Further error code and message from the file system was: Code={} Message={}",
                                              path.string(), error.what(), error.code().value(), error.code().message()));
    }
  }

  auto rootPath = checkedPath.root_path();

  // Check the deepest existing ancestor. A POSIX path reaches its root. A Windows
  // path with a nonexistent drive reaches a root that also does not exist.
  while(!std::filesystem::exists(checkedPath) && checkedPath != rootPath)
  {
    checkedPath = checkedPath.parent_path();
  }

  if(checkedPath.empty())
  {
    return MakeErrorResult(-19, fmt::format("ValidateDirectoryWritePermission() Error: Input path '{}' resolved to an empty path", path.string()));
  }

  if(!std::filesystem::exists(checkedPath))
  {
    return MakeErrorResult(-11,
                           fmt::format("ValidateDirectoryWritePermission() Error: Input Path '{}' resolved to '{}'. The drive does not exist on this system.", path.string(), checkedPath.string()));
  }

  if(HasWriteAccess(checkedPath.string()))
  {
    return {};
  }
  return MakeErrorResult(-8, fmt::format("ValidateDirectoryWritePermission() Error: User does not have write permissions to path '{}'", path.string()));
}

std::pair<bool, int32> IsUtf8(const std::filesystem::path& filePath)
{
  FILE* f = fopen(filePath.string().c_str(), "rb");
  if(nullptr == f)
  {
    return {false, -1};
  }
  std::array<uint8, 3> buf = {0, 0, 0};
  if(fread(buf.data(), 1, 3, f) != 3)
  {
    std::ignore = fclose(f);
    return {false, -1};
  }
  std::ignore = fclose(f);
  if(buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF)
  {
    return {true, 0};
  }
  return {false, 0};
}

namespace CSV
{
const int32 k_InconsistentCols = -104;
const int32 k_InvalidArrayType = -106;
const int32 k_FileNotOpen = -108;
const int32 k_CannotSkipToLine = -115;
const int32 k_EmptyLine = -119;

namespace
{
/**
 * @brief Flushes each non-null parser and merges column-qualified errors.
 * @param dataParsers Supplies parsers aligned with all CSV columns.
 * @return Merged result from every attempted flush.
 */
Result<> FlushParsersImpl(const ParsersVector& dataParsers)
{
  std::vector<Result<>> flushResults;
  flushResults.reserve(dataParsers.size());
  for(const auto& dataParser : dataParsers)
  {
    if(dataParser == nullptr)
    {
      continue;
    }

    Result<> flushResult = dataParser->flush();
    if(flushResult.invalid())
    {
      for(Error& error : flushResult.errors())
      {
        error.message = fmt::format("Array \"{}\": ", dataParser->columnName()) + error.message;
      }
    }
    flushResults.push_back(std::move(flushResult));
  }
  return MergeResults(std::move(flushResults));
}
} // namespace

AbstractDataParser::AbstractDataParser(IArray& array, const std::string& columnName, usize columnIndex)
: m_Array(array)
, m_ColumnName(columnName)
, m_ColumnIndex(columnIndex)
{
}

std::string AbstractDataParser::columnName() const
{
  return m_ColumnName;
}

usize AbstractDataParser::columnIndex() const
{
  return m_ColumnIndex;
}

const IArray& AbstractDataParser::array() const
{
  return m_Array;
}

Result<ParsersVector> CreateParsers(const std::vector<CSVType>& dataTypes, const std::vector<bool>& skippedArrays, const DataPath& parentPath, const std::vector<std::string>& headers,
                                    DataStructure& dataStructure)
{
  ParsersVector dataParsers(dataTypes.size());

  for(usize i = 0; i < dataTypes.size() && i < headers.size() && i < skippedArrays.size(); i++)
  {
    CSVType csvType = dataTypes[i];
    std::string name = headers[i];
    bool skipped = skippedArrays[i];

    if(skipped)
    {
      continue;
    }

    DataPath arrayPath = parentPath;
    arrayPath = arrayPath.createChildPath(name);

    switch(csvType)
    {
    case nx::core::CSVType::int8: {
      auto& data = dataStructure.getDataRefAs<Int8Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int8Parser>(data, name, i);
      break;
    }
    case nx::core::CSVType::uint8: {
      auto& data = dataStructure.getDataRefAs<UInt8Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt8Parser>(data, name, i);
      break;
    }
    case nx::core::CSVType::int16: {
      auto& data = dataStructure.getDataRefAs<Int16Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int16Parser>(data, name, i);
      break;
    }
    case nx::core::CSVType::uint16: {
      auto& data = dataStructure.getDataRefAs<UInt16Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt16Parser>(data, name, i);
      break;
    }
    case nx::core::CSVType::int32: {
      auto& data = dataStructure.getDataRefAs<Int32Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int32Parser>(data, name, i);
      break;
    }
    case nx::core::CSVType::uint32: {
      auto& data = dataStructure.getDataRefAs<UInt32Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt32Parser>(data, name, i);
      break;
    }
    case nx::core::CSVType::int64: {
      auto& data = dataStructure.getDataRefAs<Int64Array>(arrayPath);
      dataParsers[i] = std::make_unique<Int64Parser>(data, name, i);
      break;
    }
    case nx::core::CSVType::uint64: {
      auto& data = dataStructure.getDataRefAs<UInt64Array>(arrayPath);
      dataParsers[i] = std::make_unique<UInt64Parser>(data, name, i);
      break;
    }
    case nx::core::CSVType::float32: {
      auto& data = dataStructure.getDataRefAs<Float32Array>(arrayPath);
      dataParsers[i] = std::make_unique<Float32Parser>(data, name, i);
      break;
    }
    case nx::core::CSVType::float64: {
      auto& data = dataStructure.getDataRefAs<Float64Array>(arrayPath);
      dataParsers[i] = std::make_unique<Float64Parser>(data, name, i);
      break;
    }
    case nx::core::CSVType::boolean: {
      auto& data = dataStructure.getDataRefAs<BoolArray>(arrayPath);
      dataParsers[i] = std::make_unique<BoolParser>(data, name, i);
      break;
    }
    case nx::core::CSVType::string: {
      auto& data = dataStructure.getDataRefAs<StringArray>(arrayPath);
      dataParsers[i] = std::make_unique<StringParser>(data, name, i);
      break;
    }
    default:
      return {MakeErrorResult<ParsersVector>(k_InvalidArrayType, fmt::format("The data type that was chosen for column number {} is not a valid data array type.", std::to_string(i + 1)))};
    }
  }

  return {std::move(dataParsers)};
}

Result<> FlushParsers(const ParsersVector& dataParsers)
{
  return FlushParsersImpl(dataParsers);
}

Result<> ParseLine(std::fstream& inStream, const ParsersVector& dataParsers, const std::vector<std::string>& headers, const std::vector<char>& delimiters, bool consecutiveDelimiters, usize lineNumber,
                   usize beginIndex, bool& flushRequired)
{
  std::string line;
  std::getline(inStream, line);
  line = StringUtilities::replace(line, "\r", "");
  std::vector<std::string> tokens = StringUtilities::split(line, delimiters, consecutiveDelimiters);
  if(tokens.empty())
  {
    // An empty interior row cannot provide one token for each configured column.
    return MakeErrorResult(k_EmptyLine, fmt::format("Line #{} is empty!  You should not have any empty lines in the file.", std::to_string(lineNumber)));
  }

  if(dataParsers.size() != tokens.size())
  {
    return MakeErrorResult(k_InconsistentCols, fmt::format("Expecting {} tokens but found {} tokens in the file at line #{}.\n\nInput line was:\n{}\n\nThis is because the data-"
                                                           "types/headers/skipped-array-mask all have a size of {} but the file data at line #{} has a column count of {}.",
                                                           std::to_string(dataParsers.size()), std::to_string(tokens.size()), std::to_string(lineNumber), line, std::to_string(dataParsers.size()),
                                                           std::to_string(lineNumber), std::to_string(tokens.size())));
  }

  for(int i = 0; i < dataParsers.size(); i++)
  {
    const auto& dataParser = dataParsers[i];
    if(dataParser == nullptr)
    {
      continue;
    }

    usize index = dataParser->columnIndex();

    Result<> result = dataParser->parse(tokens[index], lineNumber - beginIndex, flushRequired);
    if(result.invalid())
    {
      for(Error& error : result.errors())
      {
        error.message = fmt::format("Array \"{}\", Line {}: ", headers[i], lineNumber) + error.message;
      }
      return result;
    }
  }

  return {};
}

std::string TupleDimsToString(const ShapeType& tupleDims)
{
  std::string tupleDimsStr;
  for(usize i = 0; i < tupleDims.size(); ++i)
  {
    tupleDimsStr += std::to_string(tupleDims[i]);
    if(i != tupleDims.size() - 1)
    {
      tupleDimsStr += "x";
    }
  }
  return tupleDimsStr;
}

std::vector<std::string> RemoveIllegalCharacters(std::vector<std::string>& headers)
{
  for(auto& headerName : headers)
  {
    // Filter-level callers include Python and nxrunner, which do not apply GUI name validation.
    // Normalize each header before it becomes a data-array name.
    headerName = StringUtilities::replace(headerName, "&", "_");
    headerName = StringUtilities::replace(headerName, ":", "_");
    headerName = StringUtilities::replace(headerName, "/", "_");
    headerName = StringUtilities::replace(headerName, "\\", "_");
    headerName = StringUtilities::replace(headerName, "\"", "");
  }
  return headers;
}

bool SkipNumberOfLines(std::fstream& inStream, usize numberOfLines)
{
  for(usize i = 1; i < numberOfLines; i++)
  {
    if(inStream.eof())
    {
      return false;
    }

    std::string line;
    std::getline(inStream, line);
  }

  return true;
}

Result<std::vector<std::string>> ReadHeaders(const std::string& inputFilePath, usize headersLineNum, const std::vector<char>& delimiters, bool consecutiveDelimiters)
{
  std::fstream in(inputFilePath.c_str(), std::ios_base::in);
  if(!in.is_open())
  {
    return MakeErrorResult<std::vector<std::string>>(k_FileNotOpen, fmt::format("Could not open file for reading: {}", inputFilePath));
  }

  if(!SkipNumberOfLines(in, headersLineNum))
  {
    return MakeErrorResult<std::vector<std::string>>(k_CannotSkipToLine, fmt::format("Could not skip to the chosen header line ({}).", headersLineNum));
  }

  std::string headersLine;
  std::getline(in, headersLine);
  auto headers = StringUtilities::split(headersLine, delimiters, consecutiveDelimiters);
  return {headers};
}
} // namespace CSV
} // namespace nx::core::FileUtilities
