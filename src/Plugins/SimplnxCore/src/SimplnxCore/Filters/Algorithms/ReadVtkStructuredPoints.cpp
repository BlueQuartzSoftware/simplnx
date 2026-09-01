#include "ReadVtkStructuredPoints.hpp"

#include "simplnx/Common/Bit.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize kBufferSize = 1024ULL;
constexpr usize k_MaxChunkBytes = 1048576;
constexpr usize k_AsciiInputBufferSize = 16384;
constexpr usize k_MaxAsciiTokenLength = 1024;

constexpr StringLiteral k_DatasetKeyword = "DATASET";
constexpr StringLiteral k_StructuredPointsKeyword = "STRUCTURED_POINTS";
constexpr StringLiteral k_DimensionsKeyword = "DIMENSIONS";
constexpr StringLiteral k_SpacingKeyword = "SPACING";
constexpr StringLiteral k_OriginKeyword = "ORIGIN";
constexpr StringLiteral k_PointDataKeyword = "POINT_DATA";
constexpr StringLiteral k_CellDataKeyword = "CELL_DATA";
constexpr StringLiteral k_ScalarsKeyword = "SCALARS";
constexpr StringLiteral k_VectorsKeyword = "VECTORS";

/**
 * @brief Parses a declared count of ASCII tokens through a fixed input buffer.
 * @tparam TokenHandler Handles one token and its zero-based index.
 * @param in Provides the VTK stream at the first value.
 * @param totalTokens Specifies the declared value count.
 * @param dataArrayPath Identifies the array for diagnostics.
 * @param shouldCancel Stops before later input buffers when true.
 * @param tokenHandler Receives each complete token.
 * @return Token, stream, conversion, or handler error, or success after cancellation.
 *
 * The parser seeks back unread bytes after the final value. This preserves the
 * next VTK header in the shared stream.
 */
template <typename TokenHandler>
Result<> ReadAsciiTokens(std::istream& in, usize totalTokens, const DataPath& dataArrayPath, const std::atomic_bool& shouldCancel, TokenHandler&& tokenHandler)
{
  if(totalTokens == 0)
  {
    return {};
  }

  auto inputBuffer = std::make_unique<char[]>(k_AsciiInputBufferSize);
  std::string token;
  token.reserve(k_MaxAsciiTokenLength);
  usize parsedTokens = 0;

  while(parsedTokens < totalTokens)
  {
    if(shouldCancel)
    {
      return {};
    }

    in.read(inputBuffer.get(), static_cast<std::streamsize>(k_AsciiInputBufferSize));
    const std::streamsize bytesRead = in.gcount();
    if(in.bad() || (in.fail() && !in.eof()))
    {
      return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::AsciiDataReadErr),
                             fmt::format("Failed to read ASCII data array '{}' after parsing {} of {} declared values.", dataArrayPath.toString(), parsedTokens, totalTokens));
    }

    for(std::streamsize index = 0; index < bytesRead; index++)
    {
      const char character = inputBuffer[static_cast<usize>(index)];
      if(std::isspace(static_cast<unsigned char>(character)) != 0)
      {
        if(token.empty())
        {
          continue;
        }

        auto tokenResult = tokenHandler(token, parsedTokens);
        if(tokenResult.invalid())
        {
          return tokenResult;
        }
        token.clear();
        parsedTokens++;

        if(parsedTokens == totalTokens)
        {
          const std::streamoff unreadBytes = bytesRead - index - 1;
          if(unreadBytes > 0)
          {
            in.clear();
            in.seekg(-unreadBytes, std::ios_base::cur);
            if(in.fail())
            {
              return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::AsciiStreamPositionErr),
                                     fmt::format("Failed to preserve the stream position after reading {} values from ASCII data array '{}'.", totalTokens, dataArrayPath.toString()));
            }
          }
          return {};
        }
        continue;
      }

      if(token.size() == k_MaxAsciiTokenLength)
      {
        return MakeErrorResult(
            to_underlying(ReadVtkStructuredPoints::ErrorCodes::AsciiTokenTooLongErr),
            fmt::format("ASCII data array '{}' contains a token longer than the supported maximum of {} characters at value index {}.", dataArrayPath.toString(), k_MaxAsciiTokenLength, parsedTokens));
      }
      token.push_back(character);
    }

    if(in.eof())
    {
      if(!token.empty())
      {
        auto tokenResult = tokenHandler(token, parsedTokens);
        if(tokenResult.invalid())
        {
          return tokenResult;
        }
        token.clear();
        parsedTokens++;
        if(parsedTokens == totalTokens)
        {
          return {};
        }
      }

      return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::AsciiDataReadErr),
                             fmt::format("ASCII data array '{}' ended after {} values, but {} values were declared.", dataArrayPath.toString(), parsedTokens, totalTokens));
    }
  }

  return {};
}

/**
 * @brief Skips a declared binary value block through bounded reads.
 * @tparam T Specifies the declared scalar type.
 * @param in Provides the VTK stream at the first value.
 * @param numElements Specifies values to skip.
 * @param dataArrayPath Identifies the array for diagnostics.
 * @param shouldCancel Stops before later input buffers when true.
 * @return Size or stream error, or success after completion or cancellation.
 */
template <typename T>
Result<> SkipBinaryData(std::istream& in, usize numElements, const DataPath& dataArrayPath, const std::atomic_bool& shouldCancel)
{
  if(numElements > std::numeric_limits<usize>::max() / sizeof(T))
  {
    return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::VtkReadBinaryDataErr),
                           fmt::format("Binary data array '{}' declares too many values ({}) to calculate its byte count.", dataArrayPath.toString(), numElements));
  }

  auto buffer = std::make_unique<char[]>(k_AsciiInputBufferSize);
  usize bytesRemaining = numElements * sizeof(T);
  while(bytesRemaining > 0)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize bytesToRead = std::min(k_AsciiInputBufferSize, bytesRemaining);
    in.read(buffer.get(), static_cast<std::streamsize>(bytesToRead));
    const std::streamsize bytesRead = in.gcount();
    if(bytesRead != static_cast<std::streamsize>(bytesToRead))
    {
      return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::VtkReadBinaryDataErr),
                             fmt::format("Binary data array '{}' ended with {} bytes remaining in its declared data block.", dataArrayPath.toString(), bytesRemaining - static_cast<usize>(bytesRead)));
    }
    bytesRemaining -= static_cast<usize>(bytesRead);
  }

  return {};
}

/**
 * @brief Skips one ASCII or binary array during preflight.
 * @tparam T Specifies the declared scalar type.
 * @param in Provides the VTK stream at the first value.
 * @param binary Selects binary or ASCII parsing.
 * @param numElements Specifies values to skip.
 * @param dataArrayPath Identifies the array for diagnostics.
 * @param shouldCancel Stops before later input buffers when true.
 * @return Parser or stream error, or success after completion or cancellation.
 */
template <typename T>
Result<> SkipVolume(std::istream& in, bool binary, usize numElements, const DataPath& dataArrayPath, const std::atomic_bool& shouldCancel)
{
  if(binary)
  {
    return SkipBinaryData<T>(in, numElements, dataArrayPath, shouldCancel);
  }

  return ReadAsciiTokens(in, numElements, dataArrayPath, shouldCancel, [](const std::string&, usize) -> Result<> { return {}; });
}

/**
 * @brief Calculates values in one typed transfer chunk.
 * @tparam T Specifies the scalar type.
 * @return At least one value and at most 1 MiB of values.
 */
template <typename T>
constexpr usize ChunkValueCapacity()
{
  return std::max<usize>(1, k_MaxChunkBytes / sizeof(T));
}

/**
 * @brief Writes one typed value chunk.
 * @tparam T Specifies the scalar type.
 * @param dataStore Receives values.
 * @param offset Specifies the first destination value.
 * @param values Provides contiguous values.
 * @return Destination bulk-write result.
 */
template <typename T>
Result<> WriteChunk(AbstractDataStore<T>& dataStore, usize offset, nonstd::span<const T> values)
{
  return dataStore.copyFromBuffer(offset, values);
}

/**
 * @brief Reads and writes one big-endian binary array in bounded chunks.
 * @tparam T Specifies the scalar type.
 * @param in Provides the VTK stream at the first value.
 * @param dataStore Receives converted native-endian values.
 * @param totalValues Specifies the declared value count.
 * @param dataArrayPath Identifies the array for diagnostics.
 * @param shouldCancel Stops before later chunks when true.
 * @return Stream or destination-write error, or success after cancellation.
 */
template <typename T>
Result<> ReadBinaryData(std::istream& in, AbstractDataStore<T>& dataStore, usize totalValues, const DataPath& dataArrayPath, const std::atomic_bool& shouldCancel)
{
  constexpr usize k_ChunkValues = ChunkValueCapacity<T>();
  auto values = std::make_unique<T[]>(k_ChunkValues);

  for(usize offset = 0; offset < totalValues; offset += k_ChunkValues)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkValues, totalValues - offset);
    const usize bytesToRead = count * sizeof(T);
    in.read(reinterpret_cast<char*>(values.get()), static_cast<std::streamsize>(bytesToRead));
    if(static_cast<usize>(in.gcount()) != bytesToRead)
    {
      return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::VtkReadBinaryDataErr),
                             fmt::format("Error reading binary data array '{}'. Read {} of {} bytes for values [{}, {}).", dataArrayPath.toString(), in.gcount(), bytesToRead, offset, offset + count));
    }

    if constexpr(endian::native == endian::little && sizeof(T) > 1)
    {
      std::transform(values.get(), values.get() + count, values.get(), [](T value) { return nx::core::byteswap(value); });
    }

    auto writeResult = WriteChunk(dataStore, offset, nonstd::span<const T>(values.get(), count));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  return {};
}

/**
 * @brief Parses and writes one ASCII array in bounded chunks.
 * @tparam T Specifies the scalar type.
 * @param in Provides the VTK stream at the first value.
 * @param dataStore Receives converted values.
 * @param totalValues Specifies the declared value count.
 * @param dataArrayPath Identifies the array for diagnostics.
 * @param shouldCancel Stops before later chunks when true.
 * @return Stream, conversion, or destination-write error, or success after cancellation.
 */
template <typename T>
Result<> ReadAsciiData(std::istream& in, AbstractDataStore<T>& dataStore, usize totalValues, const DataPath& dataArrayPath, const std::atomic_bool& shouldCancel)
{
  constexpr usize k_ChunkValues = ChunkValueCapacity<T>();
  auto values = std::make_unique<T[]>(k_ChunkValues);
  usize writeOffset = 0;
  usize bufferedValues = 0;

  return ReadAsciiTokens(in, totalValues, dataArrayPath, shouldCancel, [&](const std::string& token, usize tokenIndex) -> Result<> {
    auto conversionResult = StringInterpretationUtilities::Convert<T>(token);
    if(conversionResult.invalid())
    {
      return ConvertResult(std::move(conversionResult));
    }
    values[bufferedValues++] = conversionResult.value();

    if(bufferedValues == k_ChunkValues || tokenIndex + 1 == totalValues)
    {
      auto writeResult = WriteChunk(dataStore, writeOffset, nonstd::span<const T>(values.get(), bufferedValues));
      if(writeResult.invalid())
      {
        return writeResult;
      }
      writeOffset += bufferedValues;
      bufferedValues = 0;
    }
    return {};
  });
}

/**
 * @brief Reads one typed ASCII or binary DataArray.
 * @tparam T Specifies the scalar type.
 * @param dataStructure Provides the destination array.
 * @param in Provides the VTK stream at the first value.
 * @param binary Selects binary or ASCII parsing.
 * @param dataArrayPath Identifies the destination array.
 * @param shouldCancel Stops before later chunks when true.
 * @return Stream, conversion, or destination-write error, or success after cancellation.
 */
template <typename T>
Result<> readDataChunk(DataStructure& dataStructure, std::istream& in, bool binary, const DataPath& dataArrayPath, const std::atomic_bool& shouldCancel)
{
  auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(dataArrayPath);
  auto& dataStore = dataArray.getDataStoreRef();
  const usize totalValues = dataArray.size();
  if(totalValues == 0)
  {
    return {};
  }

  if(binary)
  {
    return ReadBinaryData(in, dataStore, totalValues, dataArrayPath, shouldCancel);
  }
  return ReadAsciiData(in, dataStore, totalValues, dataArrayPath, shouldCancel);
}

/**
 * @brief Reads one bounded header line.
 * @param in Provides the VTK stream.
 * @param result Receives a null-terminated line prefix.
 * @param length Specifies result capacity.
 * @return End-of-file error, or success.
 *
 * An overlong line is truncated and its remaining characters are discarded.
 */
Result<> ReadLine(std::istream& in, char* result, usize length)
{
  in.getline(result, length);
  if(in.fail())
  {
    if(in.eof())
    {
      return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadLineErr), "Failed to read line: Reached end of file.");
    }
    if(in.gcount() == length)
    {
      in.clear();
      in.ignore(std::numeric_limits<int>::max(), '\n');
    }
  }
  return {};
}

/**
 * @brief Reads one bounded whitespace-delimited token.
 * @param in Provides the VTK stream.
 * @param result Receives token characters.
 * @param length Specifies the stream width and result capacity.
 * @return Token-stream error, or success.
 */
Result<> ReadString(std::istream& in, char* result, usize length)
{
  in.width(length);
  std::string temp;
  in >> temp;
  std::copy(temp.begin(), temp.end(), result);
  if(in.fail())
  {
    if(in.eof())
    {
      return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadStringEofErr), "Failed to read string: Reached end of the file.");
    }
    else if(in.bad())
    {
      return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadStringReadErr), "Failed to read string: Read error on input operation.");
    }
    else if(in.fail())
    {
      return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadStringLogicalIOErr), "Failed to read string: Logical error on i/o operation.");
    }
    else
    {
      return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::ReadStringUnknownErr), "Failed to read string: Unknown error.");
    }
  }
  return {};
}

/**
 * @brief Converts a mutable C string to lowercase.
 * @param str Provides and receives characters.
 * @param len Specifies maximum characters to inspect.
 * @return str.
 */
char* LowerCase(char* str, const usize len)
{
  usize i;
  char* s;

  for(i = 0, s = str; *s != '\0' && i < len; s++, i++)
  {
    *s = tolower(*s);
  }
  return str;
}

/**
 * @brief Dispatches a preflight data-block skip from an NX DataType.
 * @param nxDType Specifies the converted VTK scalar type.
 * @param in Provides the VTK stream at the first value.
 * @param binary Selects binary or ASCII parsing.
 * @param numElements Specifies values to skip.
 * @param dataArrayPath Identifies the array for diagnostics.
 * @param shouldCancel Stops before later input buffers when true.
 * @return Parser or stream error, or success after cancellation.
 */
Result<> preflightSkipVolume(nx::core::DataType nxDType, std::istream& in, bool binary, usize numElements, const DataPath& dataArrayPath, const std::atomic_bool& shouldCancel)
{
  switch(nxDType)
  {
  case nx::core::DataType::int8: {
    return SkipVolume<int8>(in, binary, numElements, dataArrayPath, shouldCancel);
  }
  case nx::core::DataType::uint8: {
    return SkipVolume<uint8>(in, binary, numElements, dataArrayPath, shouldCancel);
  }
  case nx::core::DataType::int16: {
    return SkipVolume<int16>(in, binary, numElements, dataArrayPath, shouldCancel);
  }
  case nx::core::DataType::uint16: {
    return SkipVolume<uint16>(in, binary, numElements, dataArrayPath, shouldCancel);
  }
  case nx::core::DataType::int32: {
    return SkipVolume<int32>(in, binary, numElements, dataArrayPath, shouldCancel);
  }
  case nx::core::DataType::uint32: {
    return SkipVolume<uint32>(in, binary, numElements, dataArrayPath, shouldCancel);
  }
  case nx::core::DataType::int64: {
    return SkipVolume<int64>(in, binary, numElements, dataArrayPath, shouldCancel);
  }
  case nx::core::DataType::uint64: {
    return SkipVolume<uint64>(in, binary, numElements, dataArrayPath, shouldCancel);
  }
  case nx::core::DataType::float32: {
    return SkipVolume<float32>(in, binary, numElements, dataArrayPath, shouldCancel);
  }
  case nx::core::DataType::float64: {
    return SkipVolume<float64>(in, binary, numElements, dataArrayPath, shouldCancel);
  }
  case nx::core::DataType::boolean: {
    break;
  }
  default:
    break;
  }
  return {};
}

/**
 * @brief Converts a legacy VTK scalar token to an NX DataType.
 * @param text Specifies the VTK type token.
 * @return Converted type or unsupported-token error.
 */
Result<nx::core::DataType> ConvertVtkDataType(const std::string& text)
{
  if(text == "unsigned_char")
  {
    return {nx::core::DataType::uint8};
  }
  if(text == "char")
  {
    return {nx::core::DataType::int8};
  }
  if(text == "unsigned_short")
  {
    return {nx::core::DataType::uint16};
  }
  if(text == "short")
  {
    return {nx::core::DataType::int16};
  }
  if(text == "unsigned_int")
  {
    return {nx::core::DataType::uint32};
  }
  if(text == "int")
  {
    return {nx::core::DataType::int32};
  }
  if(text == "unsigned_long")
  {
    return {nx::core::DataType::uint64};
  }
  if(text == "long")
  {
    return {nx::core::DataType::int64};
  }
  if(text == "float")
  {
    return {nx::core::DataType::float32};
  }
  if(text == "double")
  {
    return {nx::core::DataType::float64};
  }

  return MakeErrorResult<nx::core::DataType>(to_underlying(ReadVtkStructuredPoints::ErrorCodes::ConvertVtkDataTypeErr), fmt::format("Unable to convert VTK data type '{}' to NX data type.", text));
}
} // namespace

ReadVtkStructuredPoints::ReadVtkStructuredPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ReadVtkStructuredPointsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ReadVtkStructuredPoints::~ReadVtkStructuredPoints() noexcept = default;

const std::atomic_bool& ReadVtkStructuredPoints::getCancel()
{
  return m_ShouldCancel;
}

Result<> ReadVtkStructuredPoints::operator()()
{
  return readFile();
}

void ReadVtkStructuredPoints::setPreflight(bool value)
{
  m_Preflight = value;
}

//// -----------------------------------------------------------------------------
// Result<usize> ReadVtkStructuredPoints::parseByteSize(const std::string& text)
//{
//   if(text == "unsigned_char")
//   {
//     return {1};
//   }
//   if(text == "char")
//   {
//     return {1};
//   }
//   if(text == "unsigned_short")
//   {
//     return {2};
//   }
//   if(text == "short")
//   {
//     return {2};
//   }
//   if(text == "unsigned_int")
//   {
//     return {4};
//   }
//   if(text == "int")
//   {
//     return {4};
//   }
//   if(text == "unsigned_long")
//   {
//     return {8};
//   }
//   if(text == "long")
//   {
//     return {8};
//   }
//   if(text == "float")
//   {
//     return {4};
//   }
//   if(text == "double")
//   {
//     return {8};
//   }
//   return {0};
// }

void ReadVtkStructuredPoints::setComment(const std::string& comment)
{
  m_Comment = comment;
}

void ReadVtkStructuredPoints::setFileIsBinary(bool value)
{
  m_FileIsBinary = value;
}

void ReadVtkStructuredPoints::setDatasetType(const std::string& dataSetType)
{
  m_DatasetType = dataSetType;
}

Result<> ReadVtkStructuredPoints::readFile()
{
  std::ifstream in(m_InputValues->InputFile, std::ios_base::in | std::ios_base::binary);

  if(!in.is_open())
  {
    std::string msg = fmt::format("Error opening output file '%1'", m_InputValues->InputFile.string());
    return MakeErrorResult(to_underlying(ErrorCodes::FileOpenErr), msg);
  }

  std::vector<char> buf(kBufferSize, '\0');
  std::string line;
  // char* buffer = buf.data();
  auto result = ReadLine(in, buf.data(), kBufferSize);
  if(result.invalid())
  {
    return result;
  }
  std::fill(buf.begin(), buf.end(), '\0');

  result = ReadLine(in, buf.data(), kBufferSize);
  if(result.invalid())
  {
    return result;
  }
  setComment(std::string(buf.data()));
  std::fill(buf.begin(), buf.end(), '\0');

  result = ReadLine(in, buf.data(), kBufferSize);
  if(result.invalid())
  {
    return result;
  }
  std::string fileType(buf.data());
  if(StringUtilities::starts_with(fileType, "BINARY"))
  {
    setFileIsBinary(true);
  }
  else if(StringUtilities::starts_with(fileType, "ASCII"))
  {
    setFileIsBinary(false);
  }
  else
  {
    std::string ss = fmt::format("The file type of the VTK legacy file could not be determined. It should be 'ASCII' or 'BINARY' and should appear on line 3 of the file");
    return MakeErrorResult(to_underlying(ErrorCodes::FileTypeErr), ss);
  }

  std::fill(buf.begin(), buf.end(), '\0');
  result = ReadLine(in, buf.data(), kBufferSize);
  if(result.invalid())
  {
    return result;
  }
  line = std::string(buf.data());

  auto words = StringUtilities::split(line, ' ');

  if(words.size() != 2)
  {
    std::string ss = fmt::format("Error reading the type of data set. Was expecting 2 words but got '{}'.", std::string(buf.data()));
    return MakeErrorResult(to_underlying(ErrorCodes::DatasetWordCountErr), ss);
  }
  if(words[0] != k_DatasetKeyword)
  {
    std::string ss = fmt::format("Error reading the type of data set. Could not find the '{}' keyword on line 4.", k_DatasetKeyword);
    return MakeErrorResult(to_underlying(ErrorCodes::DatasetKeywordErr), ss);
  }
  std::string dataset(words.at(1));
  dataset = StringUtilities::trimmed(dataset);
  if(dataset != k_StructuredPointsKeyword)
  {
    std::string ss = fmt::format("Invalid dataset type. The dataset type is '{}', but only dataset type '{}' is supported in this filter.", dataset, k_StructuredPointsKeyword);
    return MakeErrorResult(to_underlying(ErrorCodes::DatasetStructuredPtsErr), ss);
  }
  setDatasetType(dataset);

  std::fill(buf.begin(), buf.end(), '\0');
  result = ReadLine(in, buf.data(), kBufferSize);
  if(result.invalid())
  {
    return result;
  }
  // Cell dimensions subtract one from each point dimension.
  line = std::string(buf.data());
  auto tokens = StringUtilities::split(line, ' ');
  if(tokens.size() != 4)
  {
    std::string ss = fmt::format("Error reading the dataset dimensions. Was expecting 4 tokens but got '{}'.", std::string(buf.data()));
    return MakeErrorResult(to_underlying(ErrorCodes::DimsWordCountErr), ss);
  }
  if(tokens[0] != k_DimensionsKeyword)
  {
    std::string ss = fmt::format("Error reading the dataset dimensions. Could not find the '{}' keyword on line 5.", k_DimensionsKeyword);
    return MakeErrorResult(to_underlying(ErrorCodes::DimsKeywordErr), ss);
  }

  CreateImageGeometryAction::DimensionType pointDims(3, 0);
  auto convertResultSizeT = StringInterpretationUtilities::Convert<usize>(tokens[1]);
  if(convertResultSizeT.invalid())
  {
    return ConvertResult(std::move(convertResultSizeT));
  }
  pointDims[0] = convertResultSizeT.value();
  convertResultSizeT = StringInterpretationUtilities::Convert<usize>(tokens[2]);
  if(convertResultSizeT.invalid())
  {
    return ConvertResult(std::move(convertResultSizeT));
  }
  pointDims[1] = convertResultSizeT.value();
  convertResultSizeT = StringInterpretationUtilities::Convert<usize>(tokens[3]);
  if(convertResultSizeT.invalid())
  {
    return ConvertResult(std::move(convertResultSizeT));
  }
  pointDims[2] = convertResultSizeT.value();

  CreateImageGeometryAction::DimensionType cellDims(3, 0);
  cellDims[0] = pointDims[0] - 1;
  cellDims[1] = pointDims[1] - 1;
  cellDims[2] = pointDims[2] - 1;

  std::fill(buf.begin(), buf.end(), '\0');
  result = ReadLine(in, buf.data(), kBufferSize);
  if(result.invalid())
  {
    return result;
  }
  line = std::string(buf.data());
  tokens = StringUtilities::split(line, ' ');
  if(tokens.size() != 4)
  {
    std::string ss = fmt::format("Error reading the dataset spacing. Was expecting 4 tokens but got '{}'.", std::string(buf.data()));
    return MakeErrorResult(to_underlying(ErrorCodes::SpacingWordCountErr), ss);
  }
  if(tokens[0] != k_SpacingKeyword)
  {
    std::string ss = fmt::format("Error reading the dataset spacing. Could not find the '{}' keyword on line 6.", k_SpacingKeyword);
    return MakeErrorResult(to_underlying(ErrorCodes::SpacingKeywordErr), ss);
  }

  CreateImageGeometryAction::SpacingType spacing(3, 0.0f);

  auto convertResultF32 = StringInterpretationUtilities::Convert<float32>(tokens[1]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  spacing[0] = convertResultF32.value();
  convertResultF32 = StringInterpretationUtilities::Convert<float32>(tokens[2]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  spacing[1] = convertResultF32.value();
  convertResultF32 = StringInterpretationUtilities::Convert<float32>(tokens[3]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  spacing[2] = convertResultF32.value();

  std::fill(buf.begin(), buf.end(), '\0');
  result = ReadLine(in, buf.data(), kBufferSize);
  if(result.invalid())
  {
    return result;
  }
  line = std::string(buf.data());
  tokens = StringUtilities::split(line, ' ');
  if(tokens.size() != 4)
  {
    std::string ss = fmt::format("Error reading the dataset origin. Was expecting 4 tokens but got '{}'.", std::string(buf.data()));
    return MakeErrorResult(to_underlying(ErrorCodes::OriginWordCountErr), ss);
  }
  if(tokens[0] != k_OriginKeyword)
  {
    std::string ss = fmt::format("Error reading the dataset origin. Could not find the '{}' keyword on line 7.", k_OriginKeyword);
    return MakeErrorResult(to_underlying(ErrorCodes::OriginKeywordErr), ss);
  }

  CreateImageGeometryAction::OriginType origin(3, 0.0f);
  convertResultF32 = StringInterpretationUtilities::Convert<float32>(tokens[1]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  origin[0] = convertResultF32.value();
  convertResultF32 = StringInterpretationUtilities::Convert<float32>(tokens[2]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  origin[1] = convertResultF32.value();
  convertResultF32 = StringInterpretationUtilities::Convert<float32>(tokens[3]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  origin[2] = convertResultF32.value();

  if(m_InputValues->ReadPointData && m_Preflight)
  {
    auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(m_InputValues->PointGeomPath, pointDims, origin, spacing, m_InputValues->PointAttributeMatrixName);
    m_OutputActions.value().appendAction(std::move(createImageGeometryAction));
  }
  if(m_InputValues->ReadCellData && m_Preflight)
  {
    auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(m_InputValues->ImageGeomPath, cellDims, origin, spacing, m_InputValues->CellAttributeMatrixName);
    m_OutputActions.value().appendAction(std::move(createImageGeometryAction));
  }

  std::fill(buf.begin(), buf.end(), '\0');
  result = ReadLine(in, buf.data(), kBufferSize);
  if(result.invalid())
  {
    return result;
  }
  line = std::string(buf.data());
  tokens = StringUtilities::split(line, ' ');
  if(tokens.size() != 2)
  {
    std::string ss = fmt::format("Error reading the dataset type. Was expecting 2 tokens but got '{}'.", std::string(buf.data()));
    return MakeErrorResult(to_underlying(ErrorCodes::DatasetTypeWordCountErr), ss);
  }
  if(tokens[0] != k_PointDataKeyword && tokens[0] != k_CellDataKeyword)
  {
    std::string ss = fmt::format("Error reading the dataset type. Could not find the '{}' or '{}' keywords on line 7.", k_PointDataKeyword, k_CellDataKeyword);
    return MakeErrorResult(to_underlying(ErrorCodes::DatasetTypeKeywordErr), ss);
  }

  std::string sectionType = std::string(tokens[0]);
  auto convertResultI32 = StringInterpretationUtilities::Convert<int32>(tokens[1]);
  if(convertResultI32.invalid())
  {
    return ConvertResult(std::move(convertResultI32));
  }
  int32 numValues = convertResultI32.value();

  for(int32 i = 0; i < 2; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    if(sectionType == k_CellDataKeyword && m_InputValues->ReadCellData)
    {
      if(cellDims[0] * cellDims[1] * cellDims[2] != numValues)
      {
        return MakeErrorResult(to_underlying(ErrorCodes::MismatchedCellsAndTuplesErr),
                               fmt::format("Number of cells ({}) does not match number of tuples ({}) in the Attribute Matrix.", cellDims[0] * cellDims[1] * cellDims[2], numValues));
      }
      m_CurrentSectionType = CurrentSectionType::Cell;
      m_CurrentGeomDims = cellDims;
      auto dtResult = readDataTypeSection(in, numValues, "point_data");
      if(dtResult.invalid())
      {
        return ConvertResult(std::move(dtResult));
      }
      numValues = dtResult.value();
      sectionType = numValues > 0 ? k_PointDataKeyword : "";
    }
    else if(sectionType == k_PointDataKeyword && m_InputValues->ReadPointData)
    {
      if(pointDims[0] * pointDims[1] * pointDims[2] != numValues)
      {
        return MakeErrorResult(to_underlying(ErrorCodes::MismatchedPointsAndTuplesErr),
                               fmt::format("Number of points ({}) does not match number of tuples ({}) in the Attribute Matrix.", pointDims[0] * pointDims[1] * pointDims[2], numValues));
      }
      m_CurrentSectionType = CurrentSectionType::Point;
      m_CurrentGeomDims = pointDims;
      auto dtResult = readDataTypeSection(in, numValues, "cell_data");
      if(dtResult.invalid())
      {
        return ConvertResult(std::move(dtResult));
      }
      numValues = dtResult.value();
      sectionType = numValues > 0 ? k_CellDataKeyword : "";
    }
  }

  in.close();

  return {};
}

Result<int32> ReadVtkStructuredPoints::readDataTypeSection(std::istream& in, int32 numValues, const std::string& nextKeyWord)
{
  std::vector<char> buf(kBufferSize, '\0');

  while(!m_ShouldCancel && ReadString(in, buf.data(), kBufferSize).valid())
  {
    if(strncmp(LowerCase(buf.data(), kBufferSize), "scalars", 7) == 0)
    {
      auto result = readScalarData(in, numValues);
      if(result.invalid())
      {
        return ConvertResultTo<int32>(std::move(result), {});
      }
    }
    else if(strncmp(buf.data(), "vectors", 7) == 0)
    {
      auto result = readVectorData(in, numValues);
      if(result.invalid())
      {
        return ConvertResultTo<int32>(std::move(result), {});
      }
    }
#if 0
    //
    // read 3x3 tensor data
    //
    else if ( ! strncmp(buf.data(), "tensors", 7) )
    {
      if ( ! this->ReadTensorData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read normals data
    //
    else if ( ! strncmp(buf.data(), "normals", 7) )
    {

      if ( ! this->ReadNormalData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(buf.data(), "texture_coordinates", 19) )
    {
      if ( ! this->ReadTCoordsData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read the global id data
    //
    else if ( ! strncmp(buf.data(), "global_ids", 10) )
    {
      if ( ! this->ReadGlobalIds(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read the pedigree id data
    //
    else if ( ! strncmp(buf.data(), "pedigree_ids", 10) )
    {
      if ( ! this->ReadPedigreeIds(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read color scalars data
    //
    else if ( ! strncmp(buf.data(), "color_scalars", 13) )
    {
      if ( ! this->ReadCoScalarData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read lookup table. Associate with scalar data.
    //
    else if ( ! strncmp(buf.data(), "lookup_table", 12) )
    {
      if ( ! this->ReadLutData(a) )
      {
        return 0;
      }
    }
    //
    // read field of data
    //
    else if ( ! strncmp(buf.data(), "field", 5) )
    {
      vtkFieldData* f;
      if ( ! (f = this->ReadFieldData()) )
      {
        return 0;
      }
      for(int i = 0; i < f->GetNumberOfArrays(); i++)
      {
        a->AddArray(f->GetAbstractArray(i));
      }
      f->Delete();
    }
#endif

    else if(strncmp(buf.data(), nextKeyWord.c_str(), 9) == 0)
    {
      std::string line(buf.data());
      std::vector<std::string> tokens = StringUtilities::split(line, ' ');
      std::string sectionType = std::string(tokens[0]);
      std::fill(buf.begin(), buf.end(), '\0');
      ReadString(in, buf.data(), kBufferSize);
      auto convertResultI32 = StringInterpretationUtilities::Convert<int32>({buf.data()});
      return {convertResultI32.value()};
    }
    else
    {
      return MakeErrorResult<int32>(to_underlying(ErrorCodes::UnknownSectionKeywordErr),
                                    fmt::format("Unable to read data section.  Section does not contain any of the following keywords to determine the type of data: '{}', '{}', '{}', or '{}'.",
                                                k_ScalarsKeyword, k_VectorsKeyword, k_PointDataKeyword, k_CellDataKeyword));
    }

    std::fill(buf.begin(), buf.end(), '\0');
  }
  return {0};
}

//// ------------------------------------------------------------------------
// Result<int32> ReadVtkStructuredPoints::DecodeString(char* resname, const char* name)
//{
//   if(resname == nullptr)
//   {
//     return MakeErrorResult<int32>(-18200, "resname is NULL");
//   }
//   if(name == nullptr)
//   {
//     return MakeErrorResult<int32>(-18201, "name is NULL");
//   }
//   std::ostringstream str;
//   usize cc = 0;
//   unsigned int ch;
//   usize len = strlen(name);
//   usize reslen = 0;
//   char buffer[10] = "0x";
//   while(name[cc] != 0)
//   {
//     if(name[cc] == '%')
//     {
//       if(cc <= (len - 3))
//       {
//         buffer[2] = name[cc + 1];
//         buffer[3] = name[cc + 2];
//         buffer[4] = 0;
//         sscanf(buffer, "%x", &ch);
//         str << static_cast<char>(ch);
//         cc += 2;
//         reslen++;
//       }
//     }
//     else
//     {
//       str << name[cc];
//       reslen++;
//     }
//     cc++;
//   }
//   strncpy(resname, str.str().c_str(), reslen + 1);
//   resname[reslen] = 0;
//   return {static_cast<int32>(reslen)};
// }

Result<> ReadVtkStructuredPoints::readScalarData(std::istream& in, int32 numPts)
{
  // char line[256], name[256], key[256], tableName[256];

  std::vector<char> line(256, '\0');

  //  char buffer[1024];

  std::fill(line.begin(), line.end(), '\0');
  if(::ReadLine(in, line.data(), 256).invalid())
  {
    return MakeErrorResult(to_underlying(ErrorCodes::ReadScalarHeaderLineErr), fmt::format("Cannot read scalar header for file: {}", m_InputValues->InputFile.string()));
  }
  std::vector<std::string> tokens = StringUtilities::split({line.data()}, ' ');

  if(tokens.size() < 2)
  {
    return MakeErrorResult(to_underlying(ErrorCodes::ReadScalarHeaderWordCountErr), "Error reading SCALARS header section. Not enough tokens.");
  }

  std::string name = tokens[0];
  std::string scalarType = tokens[1];

  usize numComp = 1;
  if(tokens.size() >= 3)
  {
    numComp = static_cast<usize>(std::atoi(tokens[2].c_str()));
  }

  std::fill(line.begin(), line.end(), '\0');
  if(::ReadLine(in, line.data(), 256).invalid())
  {
    return MakeErrorResult(to_underlying(ErrorCodes::ReadLookupTableLineErr), fmt::format("Cannot read LOOKUP_TABLE line for file: {}", m_InputValues->InputFile.string()));
  }
  tokens = StringUtilities::split({line.data()}, ' ');
  if(tokens.size() != 2)
  {
    return MakeErrorResult(to_underlying(ErrorCodes::ReadLookupTableWordCountErr), "Error reading SCALARS LOOKUP_TABLE header section. Not enough tokens.");
  }

  std::string key = tokens[0];
  if(key != "LOOKUP_TABLE")
  {
    return MakeErrorResult(to_underlying(ErrorCodes::ReadLookupTableKeywordErr), "Lookup table must be specified with scalar.\nUse \"LOOKUP_TABLE default\" to use default table.");
  }
  std::string tableName = tokens[1];

  return readDataArray(in, numPts, name, scalarType, numComp);
}

Result<> ReadVtkStructuredPoints::readVectorData(std::istream& in, int32 numPts)
{
  std::vector<char> line(256, '\0');
  if(::ReadLine(in, line.data(), line.size()).invalid())
  {
    return MakeErrorResult(to_underlying(ErrorCodes::ReadScalarHeaderLineErr), fmt::format("Cannot read vector header for file '{}'.", m_InputValues->InputFile.string()));
  }

  const std::vector<std::string> tokens = StringUtilities::split({line.data()}, ' ');
  if(tokens.size() < 2)
  {
    return MakeErrorResult(to_underlying(ErrorCodes::ReadScalarHeaderWordCountErr), "Error reading VECTORS header section. Not enough tokens.");
  }

  return readDataArray(in, numPts, tokens[0], tokens[1], 3);
}

Result<> ReadVtkStructuredPoints::readDataArray(std::istream& in, int32 numPts, const std::string& name, const std::string& scalarType, usize numComp)
{
  DataPath arrayDataPath = m_InputValues->PointGeomPath.createChildPath(m_InputValues->PointAttributeMatrixName).createChildPath(name);
  if(m_CurrentSectionType == CurrentSectionType::Cell)
  {
    arrayDataPath = m_InputValues->ImageGeomPath.createChildPath(m_InputValues->CellAttributeMatrixName).createChildPath(name);
  }

  if(m_Preflight)
  {
    Result<nx::core::DataType> nxDTypeResult = ConvertVtkDataType(scalarType);
    if(nxDTypeResult.invalid())
    {
      return ConvertResult(std::move(nxDTypeResult));
    }
    const nx::core::DataType nxDType = nxDTypeResult.value();

    ShapeType tupleShape = {m_CurrentGeomDims[2], m_CurrentGeomDims[1], m_CurrentGeomDims[0]};
    auto createArrayAction = std::make_unique<CreateArrayAction>(nxDType, tupleShape, std::vector<usize>{numComp}, arrayDataPath);
    m_OutputActions.value().appendAction(std::move(createArrayAction));
    return preflightSkipVolume(nxDType, in, m_FileIsBinary, static_cast<usize>(numPts) * numComp, arrayDataPath, m_ShouldCancel);
  }

  if(scalarType == "unsigned_char")
  {
    return readDataChunk<uint8>(m_DataStructure, in, m_FileIsBinary, arrayDataPath, m_ShouldCancel);
  }
  if(scalarType == "char")
  {
    return readDataChunk<int8>(m_DataStructure, in, m_FileIsBinary, arrayDataPath, m_ShouldCancel);
  }
  if(scalarType == "unsigned_short")
  {
    return readDataChunk<uint16>(m_DataStructure, in, m_FileIsBinary, arrayDataPath, m_ShouldCancel);
  }
  if(scalarType == "short")
  {
    return readDataChunk<int16>(m_DataStructure, in, m_FileIsBinary, arrayDataPath, m_ShouldCancel);
  }
  if(scalarType == "unsigned_int")
  {
    return readDataChunk<uint32>(m_DataStructure, in, m_FileIsBinary, arrayDataPath, m_ShouldCancel);
  }
  if(scalarType == "int")
  {
    return readDataChunk<int32>(m_DataStructure, in, m_FileIsBinary, arrayDataPath, m_ShouldCancel);
  }
  if(scalarType == "unsigned_long")
  {
    return readDataChunk<uint64>(m_DataStructure, in, m_FileIsBinary, arrayDataPath, m_ShouldCancel);
  }
  if(scalarType == "long")
  {
    return readDataChunk<int64>(m_DataStructure, in, m_FileIsBinary, arrayDataPath, m_ShouldCancel);
  }
  if(scalarType == "float")
  {
    return readDataChunk<float32>(m_DataStructure, in, m_FileIsBinary, arrayDataPath, m_ShouldCancel);
  }
  if(scalarType == "double")
  {
    return readDataChunk<float64>(m_DataStructure, in, m_FileIsBinary, arrayDataPath, m_ShouldCancel);
  }

  return {};
}

//// -----------------------------------------------------------------------------
// int32 ReadVtkStructuredPoints::parseCoordinateLine(const char* input, usize& value)
//{
//   char text[256];
//   char text1[256];
//   int32 i = 0;
//   int32 n = sscanf(input, "%s %d %s", text, &i, text1);
//   if(n != 3)
//   {
//     value = -1;
//     return -1;
//   }
//   value = i;
//   return 0;
// }

void ReadVtkStructuredPoints::readData(std::istream& instream)
{
#if 0
  std::vector<char> buf(kBufferSize, '\0');
  char* buffer = buf.data();

  QList<std::vector<char>> tokens;
  int err = 0;

  bool hasPointData = false;
  bool hasCellData = false;
  bool skipChunk = false;

  AttributeMatrix::Pointer attrMat;


  while(instream.atEnd() == false)
  {
    buf = instream.readLine().trimmed();
  }
  // Check to make sure we didn't read to the end of the file
  if(instream.atEnd() == true)
  {
    return;
  }
  tokens = buf.split(' ');

  bool readDataSections;
  while(instream.atEnd() == false)
  {
    skipChunk = false;
    readDataSections = false;
    std::string dataStr(tokens.at(0));
    dataStr = "CELL_DATA";
    if (dataStr.compare("POINT_DATA")
    {
      attrMat = getDataContainerArray()->getDataContainer(getVolumeDataContainerName())->getAttributeMatrix(getVertexAttributeMatrixName());
      readDataSections = true;
      if(m_ReadPointData == true) { hasPointData = true; }
      else { skipChunk = true; }
    }
    else if (dataStr.compare("CELL_DATA")
    {
      attrMat = getDataContainerArray()->getDataContainer(getVolumeDataContainerName())->getAttributeMatrix(getCellAttributeMatrixName());
      readDataSections = true;
      if(m_ReadCellData == true) { hasCellData = true; }
      else { skipChunk = true; }
    }

    while(readDataSections == true)
    {
      // Read the SCALARS/VECTORS line which should be 3 or 4 words
      buf = instream.readLine().trimmed();
      // If we read an empty line, then we should drop into this while loop and start reading lines until
      // we find a line with something on it.
      while(buf.isEmpty() == true && instream.atEnd() == false)
      {
        buf = instream.readLine().trimmed();
      }
      // Check to make sure we didn't read to the end of the file
      if(instream.atEnd() == true)
      {
        readDataSections = false;
        continue;
      }
      tokens = buf.split(' ');

      std::string scalarNumComps;
      std::string scalarKeyWord = tokens[0];

      //Check to see if the line read is actually a POINT_DATA or CELL_DATA line
      //This would happen on the second or later block of data and means we have switched data types and need to jump out of this while loop
      if(scalarKeyWord.compare("POINT_DATA") == 0 || scalarKeyWord.compare("CELL_DATA")
      {
        readDataSections = false;
        continue;
      }

      //if we didn't exit from the POINT_DATA/CELL_DATA check, then make sure the scalars line has the correct info on it
      if (tokens.size() < 3 || tokens.size() > 4)
      {
        std::string ss = fmt::format("Error reading SCALARS header section of VTK file. 3 or 4 words are needed. Found %1. Read Line was\n  %2").arg(tokens.size()).arg(std::string(buf));
        setErrorCondition(-61009, ss);
        return;
      }

      if(scalarKeyWord.compare("SCALARS")
      {
        scalarNumComps = std::string("1");
      }
      else if (scalarKeyWord.compare("VECTORS")
      {
        scalarNumComps = std::string("3");
      }
      else
      {
        std::string ss = fmt::format("Error reading Dataset section. Unknown Keyword found. %1").arg(scalarKeyWord);
        setErrorCondition(-61010, ss);
        return;
      }
      std::string scalarName = tokens[1];
      scalarName = scalarName.replace("%20", " "); // Replace URL style encoding of string names. %20 is a Space.
      std::string scalarType = tokens[2];

      if(tokens.size() == 4)
      {
        scalarNumComps = tokens[3];
      }

      // Read the LOOKUP_TABLE line which should be 2 words
      buf = instream.readLine().trimmed();
      tokens = buf.split(' ');
      std::string lookupKeyWord = tokens[0];
      if (lookupKeyWord.compare("LOOKUP_TABLE") != 0 || tokens.size() != 2)
      {
        std::string ss = fmt::format("Error reading LOOKUP_TABLE header section of VTK file. 2 words are needed. Found %1. Read Line was\n  %2").arg(tokens.size()).arg(std::string(buf));
        setErrorCondition(-61011, ss);
        return;
      }

      if (scalarType == "unsigned_char")
      {
        err = readDataChunk<uint8>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "char")
      {
        err = readDataChunk<int8>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "unsigned_short")
      {
        err = readDataChunk<uint16>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "short")
      {
        err = readDataChunk<int16>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "unsigned_int")
      {
        err = readDataChunk<uint32>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "int")
      {
        err = readDataChunk<int32>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "unsigned_long")
      {
        err = readDataChunk<int64>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "long")
      {
        err = readDataChunk<quint64>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "float")
      {
        err = readDataChunk<float32>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "double")
      {
        err = readDataChunk<float64>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }

      if(err < 0)
      {
        std::string ss = fmt::format("Error Reading Dataset from VTK File. Dataset Type %1\n  DataSet Name %2\n  Numerical Type: %3\n  File Pos").arg(scalarKeyWord).arg(scalarKeyWord).arg(scalarType).arg(filePos);
        setErrorCondition(err, ss);
        return;
      }

    }

  }

#endif
}
