#include "ReadVtkStructuredPoints.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Parsing/Text/CsvParser.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize kBufferSize = 1024ULL;
constexpr usize DEFAULT_BLOCKSIZE = 1048576; // This is evenly divisible by 2,4, & 8.

constexpr StringLiteral k_DatasetKeyword = "DATASET";
constexpr StringLiteral k_StructuredPointsKeyword = "STRUCTURED_POINTS";
constexpr StringLiteral k_DimensionsKeyword = "DIMENSIONS";
constexpr StringLiteral k_SpacingKeyword = "SPACING";
constexpr StringLiteral k_OriginKeyword = "ORIGIN";
constexpr StringLiteral k_PointDataKeyword = "POINT_DATA";
constexpr StringLiteral k_CellDataKeyword = "CELL_DATA";
constexpr StringLiteral k_ScalarsKeyword = "SCALARS";
constexpr StringLiteral k_VectorsKeyword = "VECTORS";

constexpr char k_VolumeDelimiter = ' ';

// consecutiveDelimiters removed because it was never used in function
// numerous optimizations can be made to
usize count_tokens(const char* str, usize endPos)
{
  usize count = 0;
  for(usize i = 0; i < endPos; i++)
  {
    if(str[i] == k_VolumeDelimiter && str[i + 1] != k_VolumeDelimiter && str[i + 1] != '\0')
    {
      count++;
    }
    // Stop on null termination
    if(str[i] == 0)
    {
      break;
    }
  }
  return ++count;
}

// -----------------------------------------------------------------------------
template <typename T>
int32 skipVolume(std::istream& in, bool binary, usize numElements)
{
  int32 err = 0;
  if(binary)
  {
    std::istream::pos_type pos = in.tellg();
    int64 newPos = numElements * sizeof(T) + 1;
    in.seekg(newPos, std::ios_base::cur); // Move relative to the current position
    pos = in.tellg();
    if(in.fail())
    {
      // check if the position to jump to is past the end of the file
      return -1;
    }
  }
  else
  {
    const usize BUFFER_SIZE = 16384;
    usize foundItems = 0;
    std::vector<char> buffer(BUFFER_SIZE + 1, 0);
    while(foundItems < numElements)
    {
      std::fill(buffer.begin(), buffer.end(), '\0');             // Splat Zeros across everything
      err = CsvParser::ReadLine(in, buffer.data(), BUFFER_SIZE); // Read BUFFER_SIZE worth of data.
      foundItems += count_tokens(buffer.data(), BUFFER_SIZE + 1);
    }
  }
  return err;
}

// -------------------------------------------------------------------------
template <typename T>
int32 vtkReadBinaryData(std::istream& in, DataArray<T>& data)
{
  if(data.getNumberOfComponents() == 0 || data.getNumberOfTuples() == 0)
  {
    // nothing to read here.
    return 1;
  }
  usize numTuples = data.getNumberOfTuples();
  usize numComp = data.getNumberOfComponents();

  using DataStoreType = typename DataArray<T>::store_type;

  DataStoreType& dataStore = data.getDataStoreRef();

  usize numBytesToRead = static_cast<usize>(numTuples) * static_cast<usize>(numComp) * sizeof(T);
  // Cast our pointer to a pointer that std::istream will take

  usize numRead = 0;
  // Now start reading the data in chunks if needed.
  usize chunkSize = DEFAULT_BLOCKSIZE;

  // Sanity check the chunk size to make sure it is not any larger than the chunk of data we are about to read
  if(numBytesToRead < DEFAULT_BLOCKSIZE)
  {
    chunkSize = numBytesToRead;
  }

  std::vector<char> chunk(chunkSize, 0);
  char* chunkPtr = chunk.data();
  nonstd::span<T> typedArray(reinterpret_cast<T*>(chunk.data()), chunkSize);

  usize masterByteCounter = 0;
  usize bytes_read = 0;
  usize typeSize = sizeof(T);
  usize totalElementsRead = 0;

  // Now chunk through the file reading up chunks of data that can actually be
  // read in a single read. DEFAULT_BLOCKSIZE will control this.
  while(true)
  {
    in.read(chunkPtr, chunkSize);
    bytes_read = in.gcount();

    // Copy the buffer to the DataArray<T> object
    std::copy(typedArray.begin(), typedArray.end(), dataStore.begin() + totalElementsRead);

    totalElementsRead += bytes_read / typeSize; // Keep track of the number of actual values read, not just the bytes
    masterByteCounter += bytes_read;            // Keep track of the total number of bytes that have been read.

    // Check if we are done reading the data, if so, break from the loop
    if(masterByteCounter >= numBytesToRead)
    {
      break;
    }

    // Adjust the chunk size for next time
    if(numBytesToRead - masterByteCounter < chunkSize)
    {
      chunkSize = numBytesToRead - masterByteCounter;
    }

    if(in.good())
    {
      // std::cout << "all data read successfully." << in.gcount() << std::endl;
    }

    if((in.rdstate() & std::ifstream::failbit) != 0)
    {
      std::cout << "FAIL " << in.gcount() << " could be read. Needed " << chunkSize << " total bytes read = " << masterByteCounter << std::endl;
      return -12020;
    }
    if((in.rdstate() & std::ifstream::eofbit) != 0)
    {
      std::cout << "EOF " << in.gcount() << " could be read. Needed " << chunkSize << " total bytes read = " << masterByteCounter << std::endl;
      return -12021;
    }
    if((in.rdstate() & std::ifstream::badbit) != 0)
    {
      std::cout << "BAD " << in.gcount() << " could be read. Needed " << chunkSize << " total bytes read = " << masterByteCounter << std::endl;
      return -12021;
    }
  }

  // Swap to big Endian... because
  if constexpr(endian::little == endian::native)
  {
    data.byteSwapElements();
  }
  return 0;
}

// -----------------------------------------------------------------------------
template <typename T>
Result<> readDataChunk(DataStructure* dataStructurePtr, std::istream& in, bool binary, const DataPath& dataArrayPath)
{
  using DataArrayType = DataArray<T>;

  auto& dataArrayRef = dataStructurePtr->getDataRefAs<DataArrayType>(dataArrayPath);
  std::vector<usize> tDims = dataArrayRef.getTupleShape();
  std::vector<usize> cDims = dataArrayRef.getComponentShape();

  dataArrayRef.fill(static_cast<T>(0));
  if(binary)
  {
    int32 err = vtkReadBinaryData<T>(in, dataArrayRef);
    if(err < 0)
    {
      return MakeErrorResult(to_underlying(ReadVtkStructuredPoints::ErrorCodes::VtkReadBinaryDataErr),
                             fmt::format("Error Reading Binary Data '{}'.  numTuples = {}\n", dataArrayPath.toString(), dataArrayRef.getNumberOfTuples()));
    }
    if(nx::core::checkEndian() == nx::core::endian::big)
    {
      dataArrayRef.byteSwapElements();
    }
  }
  else
  {
    auto start = std::chrono::steady_clock::now();

    usize totalSize = dataArrayRef.size();
    const usize BUFFER_SIZE = 16384;

    std::vector<char> buffer(BUFFER_SIZE, 0);
    usize index = 0;
    while(index < totalSize)
    {
      auto now = std::chrono::steady_clock::now();
      if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
      {
        std::cout << "Read " << index << "/" << totalSize << "  " << std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() << std::endl;
        start = std::chrono::steady_clock::now();
      }

      int32 bytesRead = CsvParser::ReadLine(in, buffer.data(), buffer.size());
      auto tokens = StringUtilities::split({buffer.data()}, ' ');
      std::fill(buffer.begin(), buffer.begin() + bytesRead, 0);
      for(const auto& token : tokens)
      {
        auto result = nx::core::ConvertTo<T>::convert(token);
        if(result.invalid())
        {
          return ConvertResult(std::move(result));
        }
        dataArrayRef[index++] = result.value();
      }
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
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
      // Read kBufferSize chars; ignoring the rest of the line.
      in.clear();
      in.ignore(std::numeric_limits<int>::max(), '\n');
    }
  }
  return {};
}

// --------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
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

// ------------------------------------------------------------------------
Result<> preflightSkipVolume(nx::core::DataType nxDType, std::istream& in, bool binary, usize numElements)
{
  switch(nxDType)
  {
  case nx::core::DataType::int8: {
    skipVolume<int8>(in, binary, numElements);
    break;
  }
  case nx::core::DataType::uint8: {
    skipVolume<uint8>(in, binary, numElements);
    break;
  }
  case nx::core::DataType::int16: {
    skipVolume<int16>(in, binary, numElements);
    break;
  }
  case nx::core::DataType::uint16: {
    skipVolume<uint16>(in, binary, numElements);
    break;
  }
  case nx::core::DataType::int32: {
    skipVolume<int32>(in, binary, numElements);
    break;
  }
  case nx::core::DataType::uint32: {
    skipVolume<uint32>(in, binary, numElements);
    break;
  }
  case nx::core::DataType::int64: {
    skipVolume<int64>(in, binary, numElements);
    break;
  }
  case nx::core::DataType::uint64: {
    skipVolume<uint64>(in, binary, numElements);
    break;
  }
  case nx::core::DataType::float32: {
    skipVolume<float32>(in, binary, numElements);
    break;
  }
  case nx::core::DataType::float64: {
    skipVolume<float64>(in, binary, numElements);
    break;
  }
  case nx::core::DataType::boolean: {
    break;
  }
  default:
    break;
  }
  return {};
}

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

// -----------------------------------------------------------------------------
ReadVtkStructuredPoints::ReadVtkStructuredPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ReadVtkStructuredPointsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadVtkStructuredPoints::~ReadVtkStructuredPoints() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ReadVtkStructuredPoints::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
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

  auto result = ReadLine(in, buf.data(), kBufferSize); // Read Line 1 - VTK Version Info
  if(result.invalid())
  {
    return result;
  }
  std::fill(buf.begin(), buf.end(), '\0'); // Splat nulls across the vector

  result = ReadLine(in, buf.data(), kBufferSize); // Read Line 2 - User Comment
  if(result.invalid())
  {
    return result;
  }
  setComment(std::string(buf.data()));
  std::fill(buf.begin(), buf.end(), '\0'); // Splat nulls across the vector

  result = ReadLine(in, buf.data(), kBufferSize); // Read Line 3 - BINARY or ASCII
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

  // Read Line 4 - Type of Dataset
  std::fill(buf.begin(), buf.end(), '\0'); // Splat nulls across the vector
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

  std::fill(buf.begin(), buf.end(), '\0');        // Splat nulls across the vector
  result = ReadLine(in, buf.data(), kBufferSize); // Read Line 5 which is the Dimension values
  if(result.invalid())
  {
    return result;
  }
  // But we need the 'extents' which is one less in all directions (unless dim=1)
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
  auto convertResultSizeT = nx::core::ConvertTo<usize>::convert(tokens[1]);
  if(convertResultSizeT.invalid())
  {
    return ConvertResult(std::move(convertResultSizeT));
  }
  pointDims[0] = convertResultSizeT.value();
  convertResultSizeT = nx::core::ConvertTo<usize>::convert(tokens[2]);
  if(convertResultSizeT.invalid())
  {
    return ConvertResult(std::move(convertResultSizeT));
  }
  pointDims[1] = convertResultSizeT.value();
  convertResultSizeT = nx::core::ConvertTo<usize>::convert(tokens[3]);
  if(convertResultSizeT.invalid())
  {
    return ConvertResult(std::move(convertResultSizeT));
  }
  pointDims[2] = convertResultSizeT.value();

  CreateImageGeometryAction::DimensionType cellDims(3, 0);
  cellDims[0] = pointDims[0] - 1;
  cellDims[1] = pointDims[1] - 1;
  cellDims[2] = pointDims[2] - 1;

  std::fill(buf.begin(), buf.end(), '\0');        // Splat nulls across the vector
  result = ReadLine(in, buf.data(), kBufferSize); // Read Line 6 which is the Scaling values
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

  auto convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[1]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  spacing[0] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[2]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  spacing[1] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[3]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  spacing[2] = convertResultF32.value();

  std::fill(buf.begin(), buf.end(), '\0');        // Splat nulls across the vector
  result = ReadLine(in, buf.data(), kBufferSize); // Read Line 7 which is the Origin values
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
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[1]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  origin[0] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[2]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  origin[1] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[3]);
  if(convertResultF32.invalid())
  {
    return ConvertResult(std::move(convertResultF32));
  }
  origin[2] = convertResultF32.value();

  // Create the Image Geometry
  // Define a custom class that generates the changes to the DataStructure.
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

  // Read the first key word which should be POINT_DATA or CELL_DATA
  std::fill(buf.begin(), buf.end(), '\0');        // Splat nulls across the vector
  result = ReadLine(in, buf.data(), kBufferSize); // Read Line 8 which is the first type of data we are going to read
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
  auto convertResultI32 = nx::core::ConvertTo<int32>::convert(tokens[1]);
  if(convertResultI32.invalid())
  {
    return ConvertResult(std::move(convertResultI32));
  }
  int32 numValues = convertResultI32.value();

  for(int i = 0; i < 2; i++)
  {
    if(sectionType == k_CellDataKeyword && m_InputValues->ReadCellData)
    {
      if(cellDims[0] * cellDims[1] * cellDims[2] != numValues)
      {
        return MakeErrorResult(to_underlying(ErrorCodes::MismatchedCellsAndTuplesErr), "Number of cells does not match number of tuples in the Attribute Matrix");
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
        return MakeErrorResult(to_underlying(ErrorCodes::MismatchedPointsAndTuplesErr), "Number of points does not match number of tuples in the Attribute Matrix");
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

  // Close the file since we are done with it.
  in.close();

  return {};
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Result<int32> ReadVtkStructuredPoints::readDataTypeSection(std::istream& in, int32 numValues, const std::string& nextKeyWord)
{
  std::vector<char> buf(kBufferSize, '\0');

  // Read keywords until end-of-file
  while(ReadString(in, buf.data(), kBufferSize).valid())
  {
    // read scalar data
    if(strncmp(LowerCase(buf.data(), kBufferSize), "scalars", 7) == 0)
    {
      auto result = readScalarData(in, numValues);
      if(result.invalid())
      {
        return ConvertResultTo<int32>(std::move(result), {});
      }
    }
    // read vector data
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

    // maybe bumped into cell data
    else if(strncmp(buf.data(), nextKeyWord.c_str(), 9) == 0)
    {
      std::string line(buf.data());
      std::vector<std::string> tokens = StringUtilities::split(line, ' ');
      std::string sectionType = std::string(tokens[0]);
      // Read the number of values
      std::fill(buf.begin(), buf.end(), '\0'); // Splat nulls across the vector
      ReadString(in, buf.data(), kBufferSize);
      auto convertResultI32 = nx::core::ConvertTo<int32>::convert({buf.data()});
      return {convertResultI32.value()};
    }
    else
    {
      return MakeErrorResult<int32>(to_underlying(ErrorCodes::UnknownSectionKeywordErr),
                                    fmt::format("Unable to read data section.  Section does not contain any of the following keywords to determine the type of data: '{}', '{}', '{}', or '{}'.",
                                                k_ScalarsKeyword, k_VectorsKeyword, k_PointDataKeyword, k_CellDataKeyword));
    }

    std::fill(buf.begin(), buf.end(), '\0'); // Splat nulls across the vector
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

// ------------------------------------------------------------------------
Result<> ReadVtkStructuredPoints::readScalarData(std::istream& in, int32 numPts)
{
  // char line[256], name[256], key[256], tableName[256];

  std::vector<char> line(256, '\0');

  //  char buffer[1024];

  std::fill(line.begin(), line.end(), '\0');     // Splat nulls across the vector
  if(::ReadLine(in, line.data(), 256).invalid()) // Read the rest of the line
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

  // Done with that line in the file
  std::fill(line.begin(), line.end(), '\0');     // Splat nulls across the vector
  if(::ReadLine(in, line.data(), 256).invalid()) // Read the rest of the line
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
    nx::core::DataType nxDType = nxDTypeResult.value();

    std::vector<usize> tupleShape = {m_CurrentGeomDims[2], m_CurrentGeomDims[1], m_CurrentGeomDims[0]};
    auto createArrayAction = std::make_unique<CreateArrayAction>(nxDType, tupleShape, std::vector<usize>{numComp}, arrayDataPath);
    m_OutputActions.value().appendAction(std::move(createArrayAction));
    preflightSkipVolume(nxDType, in, m_FileIsBinary, numPts * numComp);

    return {};
  }

  // Read the data
  if(scalarType == "unsigned_char")
  {
    return readDataChunk<uint8>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "char")
  {
    return readDataChunk<int8>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "unsigned_short")
  {
    return readDataChunk<uint16>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "short")
  {
    return readDataChunk<int16>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "unsigned_int")
  {
    return readDataChunk<uint32>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "int")
  {
    return readDataChunk<int32>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "unsigned_long")
  {
    return readDataChunk<uint64>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "long")
  {
    return readDataChunk<int64>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "float")
  {
    return readDataChunk<float32>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "double")
  {
    return readDataChunk<float64>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }

  return {};
}

// !!! Does Nothing !!!
// ------------------------------------------------------------------------
Result<> ReadVtkStructuredPoints::readVectorData(std::istream& in, int32 numPts)
{
#if 0
  int skipVector = 0;
  char line[256], name[256];
  vtkDataArray* data;
  char buffer[1024];

  if (!(this->readString(buffer) && this->readString(line)))
  {
    vtkErrorMacro( << "Cannot read vector data!" << " for file: " << (this->FileName ? this->FileName : "(Null FileName)"));
    return 0;
  }
  this->DecodeString(name, buffer);

  //
  // See whether vector has been already read or vector name (if specified)
  // matches name in file.
  //
  if ( a->GetVectors() != nullptr || (this->VectorsName && strcmp(name, this->VectorsName)) )
  {
    skipVector = 1;
  }

  data = vtkDataArray::SafeDownCast(
           this->ReadArray(line, numPts, 3));
  if ( data != nullptr )
  {
    data->SetName(name);
    if ( ! skipVector )
    {
      a->SetVectors(data);
    }
    else if ( this->ReadAllVectors )
    {
      a->AddArray(data);
    }
    data->Delete();
  }
  else
  {
    return 0;
  }

  float32 progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));
#endif
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

// -----------------------------------------------------------------------------
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
