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

  // Check if the very last character is NOT a newline character
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

  // Read through the rest of the file
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

  // Obtain the file size
  usize fileSize = std::filesystem::file_size(absPath);

  // Open the file
  std::ifstream in(absPath.c_str(), std::ios_base::binary);
  if(!in.is_open())
  {
    return MakeErrorResult(-301, fmt::format("Could not open file for reading: {}", absPath.string()));
  }

  auto isUtf8 = IsUtf8(absPath);
  if(isUtf8.first)
  {
    // The file is UTF8 with a BOM marker, so read the first 3 bytes and dump them.
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

  // Allocate the buffer
  std::vector<char> buffer(actualSize, 0);

  // Copy the file contents into the buffer
  try
  {
    in.read(buffer.data(), actualSize);
  } catch(const std::exception& e)
  {
    return MakeErrorResult(-302, fmt::format("There was an error reading the data from file: {}.  Exception: {}", absPath.string(), e.what()));
  }

  // Check the buffer for invalid characters, tab characters, new-line characters, and carriage return characters
  bool hasNewLines = false;
  bool hasCarriageReturns = false;
  bool hasTabs = false;
  // If the first line of the file is > 2048 then this will fail! (MJ)
  for(size_t i = 0; i < actualSize; i++)
  {
    const char currentChar = buffer[i];

    if(currentChar < 32 && currentChar != 9 && currentChar != 10 && currentChar != 13)
    {
      // This is an unprintable character
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
    // This might be a binary file
    return MakeErrorResult(-304, fmt::format("The file \"{}\" might be a binary file, because line-feed, tab, or carriage return characters have not been detected. Using this file may crash the "
                                             "program or cause unexpected results.  Please import a different file.",
                                             absPath.string()));
  }

  return {};
}

//-----------------------------------------------------------------------------
bool HasWriteAccess(const std::string& path)
{
  return FSPP_ACCESS_FUNC_NAME(path.c_str(), k_CheckWritable) == k_HasAccess;
}

//-----------------------------------------------------------------------------
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
  // We now have the parent directory. Let us see if *any* part of the path exists

  // If the path is relative, then make it absolute
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

  // The idea here is to start walking up from the deepest directory and hopefully
  // find an existing directory. If we get to the top if the path and we are still
  // empty then:
  //  On unix based systems not sure if it would happen. Even if the user set a path
  // to another drive that didn't exist, at some point you hit the '/' and then you
  // can try to create the directories.
  //  On Windows the user put in a bogus drive letter which is just a hard failure
  // because we can't make up a new drive letter.
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

  // We should be at the top of the tree with an existing directory.
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

AbstractDataParser::AbstractDataParser(AbstractArray& array, const std::string& columnName, usize columnIndex)
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

const AbstractArray& AbstractDataParser::array() const
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

Result<> ParseLine(std::fstream& inStream, const ParsersVector& dataParsers, const std::vector<std::string>& headers, const std::vector<char>& delimiters, bool consecutiveDelimiters, usize lineNumber,
                   usize beginIndex)
{
  std::string line;
  std::getline(inStream, line);
  line = StringUtilities::replace(line, "\r", "");
  std::vector<std::string> tokens = StringUtilities::split(line, delimiters, consecutiveDelimiters);
  if(tokens.empty())
  {
    // This is an empty line in the middle of the CSV file, which just shouldn't happen
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

    Result<> result = dataParser->parse(tokens[index], lineNumber - beginIndex);
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
    // Replace all illegal characters with '_' character. The header names become array names which is the issue.
    // This should have been taken care of in the GUI, but if someone is trying this from Python they will not have done that
    // or if they are just reading it in through nxrunner.
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

  // Skip to the headers line
  if(!SkipNumberOfLines(in, headersLineNum))
  {
    return MakeErrorResult<std::vector<std::string>>(k_CannotSkipToLine, fmt::format("Could not skip to the chosen header line ({}).", headersLineNum));
  }

  // Read the headers line
  std::string headersLine;
  std::getline(in, headersLine);
  auto headers = StringUtilities::split(headersLine, delimiters, consecutiveDelimiters);
  return {headers};
}
} // namespace CSV
} // namespace nx::core::FileUtilities
