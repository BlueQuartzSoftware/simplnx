#include "ReadVtkStructuredPoints.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Parsing/Text/CsvParser.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <iostream>
#include <string>
#include <vector>

#define vtkErrorMacro(msg) std::cout msg

using namespace nx::core;

namespace
{

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
  int32 err = readFile();
  return {};
}

namespace
{
constexpr usize kBufferSize = 1024ULL;
inline constexpr size_t DEFAULT_BLOCKSIZE = 1048576; // This is evenly divisible by 2,4, & 8.

} // namespace

void ReadVtkStructuredPoints::setPreflight(bool value)
{
  m_Preflight = value;
}

nx::core::DataType ConvertVtkDataType(const std::string& text)
{
  if(text == "unsigned_char")
  {
    return nx::core::DataType::uint8;
  }
  if(text == "char")
  {
    return nx::core::DataType::int8;
  }
  if(text == "unsigned_short")
  {
    return nx::core::DataType::uint16;
  }
  if(text == "short")
  {
    return nx::core::DataType::int16;
  }
  if(text == "unsigned_int")
  {
    return nx::core::DataType::uint32;
  }
  if(text == "int")
  {
    return nx::core::DataType::int32;
  }
  if(text == "unsigned_long")
  {
    return nx::core::DataType::uint64;
  }
  if(text == "long")
  {
    return nx::core::DataType::int64;
  }
  if(text == "float")
  {
    return nx::core::DataType::float32;
  }
  if(text == "double")
  {
    return nx::core::DataType::float64;
  }
  // IT CANNOT BE THIS VALUE SO USE THIS AS AN "I HAVE NO IDEA WHAT THIS IS RETURN TYPE"
  return nx::core::DataType::boolean;
}

// -----------------------------------------------------------------------------
size_t ReadVtkStructuredPoints::parseByteSize(const std::string& text)
{
  if(text == "unsigned_char")
  {
    return 1;
  }
  if(text == "char")
  {
    return 1;
  }
  if(text == "unsigned_short")
  {
    return 2;
  }
  if(text == "short")
  {
    return 2;
  }
  if(text == "unsigned_int")
  {
    return 4;
  }
  if(text == "int")
  {
    return 4;
  }
  if(text == "unsigned_long")
  {
    return 8;
  }
  if(text == "long")
  {
    return 8;
  }
  if(text == "float")
  {
    return 4;
  }
  if(text == "double")
  {
    return 8;
  }
  return 0;
}

inline usize count_tokens(char* str, char delim, bool consecutiveDelimiters, usize endPos)
{
  usize count = 0;
  for(usize i = 0; i < endPos; i++)
  {
    if(str[i] == delim && str[i + 1] != delim && str[i + 1] != '\0')
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
int32_t skipVolume(std::istream& in, bool binary, size_t numElements)
{
  int32_t err = 0;
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
    std::vector<char> buffer(BUFFER_SIZE, 0);
    while(foundItems < numElements)
    {
      memset(buffer.data(), 0, BUFFER_SIZE + 1);
      err = CsvParser::ReadLine(in, buffer.data(), buffer.size());
      foundItems += count_tokens(buffer.data(), ' ', false, BUFFER_SIZE);
    }
  }
  return err;
}

// -------------------------------------------------------------------------
template <typename T>
int32_t vtkReadBinaryData(std::istream& in, DataArray<T>& data)
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

  size_t numBytesToRead = static_cast<size_t>(numTuples) * static_cast<size_t>(numComp) * sizeof(T);
  size_t numRead = 0;
  // Cast our pointer to a pointer that std::istream will take

  numRead = 0;
  // Now start reading the data in chunks if needed.
  size_t chunkSize = DEFAULT_BLOCKSIZE;

  // Sanity check the chunk size to make sure it is not any larger than the chunk of data we are about to read
  if(numBytesToRead < DEFAULT_BLOCKSIZE)
  {
    chunkSize = numBytesToRead;
  }

  std::vector<char> chunk(chunkSize, 0);
  char* chunkPtr = chunk.data();
  nonstd::span<T> typedArray(reinterpret_cast<T*>(chunk.data()), chunkSize);

  size_t masterByteCounter = 0;
  size_t bytes_read = 0;
  size_t typeSize = sizeof(T);
  size_t totalElementsRead = 0;

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
int32_t readDataChunk(DataStructure* dataStructurePtr, std::istream& in, bool binary, const DataPath& dataArrayPath)
{
  using DataArrayType = DataArray<T>;

  DataArrayType& dataArrayRef = dataStructurePtr->getDataRefAs<DataArrayType>(dataArrayPath);
  //  size_t numTuples = dataArrayRef.getNumberOfTuples();
  std::vector<size_t> tDims = dataArrayRef.getTupleShape();
  std::vector<size_t> cDims = dataArrayRef.getComponentShape();

  dataArrayRef.fill(static_cast<T>(0));
  if(binary)
  {
    int32_t err = vtkReadBinaryData<T>(in, dataArrayRef);
    if(err < 0)
    {
      std::cout << "Error Reading Binary Data '" << dataArrayPath.toString() << " numTuples = " << dataArrayRef.getNumberOfTuples() << std::endl;
      return err;
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
        if(result.valid())
        {
          dataArrayRef[index++] = result.value();
        }
      }
    }
  }

  return 1;
}

// -----------------------------------------------------------------------------
int32_t ReadVtkStructuredPoints::readLine(std::istream& in, char* result, size_t length)
{
  in.getline(result, length);
  if(in.fail())
  {
    if(in.eof())
    {
      return 0;
    }
    if(in.gcount() == length)
    {
      // Read kBufferSize chars; ignoring the rest of the line.
      in.clear();
      in.ignore(std::numeric_limits<int>::max(), '\n');
    }
  }
  return 1;
}

// --------------------------------------------------------------------------
int32_t ReadVtkStructuredPoints::readString(std::istream& in, char* result, size_t length)
{
  in.width(length);
  in >> result;
  if(in.fail())
  {
    return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------------
char* ReadVtkStructuredPoints::lowerCase(char* str, const size_t len)
{
  size_t i;
  char* s;

  for(i = 0, s = str; *s != '\0' && i < len; s++, i++)
  {
    *s = tolower(*s);
  }
  return str;
}

void ReadVtkStructuredPoints::setErrorCondition(int error, const std::string& message)
{
  m_ErrorCode = error;
  m_ErrorMessage = message;
}

int32 ReadVtkStructuredPoints::getErrorCode() const
{
  return m_ErrorCode;
}

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

nx::core::Result<OutputActions> ReadVtkStructuredPoints::PreflightFile(ReadVtkStructuredPointsInputValues& inputValues)
{
  DataStructure dataStructure;
  const IFilter::MessageHandler mesgHandler;
  const std::atomic_bool shouldCancel = {false};
  ReadVtkStructuredPoints instance(dataStructure, mesgHandler, shouldCancel, &inputValues);
  instance.setPreflight(true);
  instance.readFile();
  return instance.getOutputActions();
}

// -----------------------------------------------------------------------------
int32_t ReadVtkStructuredPoints::readFile()
{
  int32_t err = 0;

  std::ifstream in(m_InputValues->InputFile, std::ios_base::in | std::ios_base::binary);

  if(!in.is_open())
  {
    std::string msg = fmt::format("Error opening output file '%1'", m_InputValues->InputFile.string());
    setErrorCondition(-61003, msg);
    return -100;
  }

  std::vector<char> buf(kBufferSize, '\0');
  std::string line;
  // char* buffer = buf.data();

  err = readLine(in, buf.data(), kBufferSize); // Read Line 1 - VTK Version Info
  std::fill(buf.begin(), buf.end(), '\0');     // Splat nulls across the vector

  err = readLine(in, buf.data(), kBufferSize); // Read Line 2 - User Comment
  setComment(std::string(buf.data()));
  std::fill(buf.begin(), buf.end(), '\0'); // Splat nulls across the vector

  err = readLine(in, buf.data(), kBufferSize); // Read Line 3 - BINARY or ASCII
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
    setErrorCondition(-61004, ss);
    return m_ErrorCode;
  }

  // Read Line 4 - Type of Dataset
  std::fill(buf.begin(), buf.end(), '\0'); // Splat nulls across the vector
  err = readLine(in, buf.data(), kBufferSize);
  line = std::string(buf.data());

  auto words = StringUtilities::split(line, ' ');

  if(words.size() != 2)
  {
    std::string ss = fmt::format("Error reading the type of data set. Was expecting 2 words but got {}", std::string(buf.data()));
    setErrorCondition(-61005, ss);
    return getErrorCode();
  }
  std::string dataset(words.at(1));
  dataset = StringUtilities::trimmed(dataset);
  setDatasetType(dataset); // Should be STRUCTURED_POINTS

  bool ok = false;
  std::fill(buf.begin(), buf.end(), '\0');     // Splat nulls across the vector
  err = readLine(in, buf.data(), kBufferSize); // Read Line 5 which is the Dimension values
  // But we need the 'extents' which is one less in all directions (unless dim=1)
  line = std::string(buf.data());
  auto tokens = StringUtilities::split(line, ' ');

  CreateImageGeometryAction::DimensionType pointDims(3, 0);
  auto convertResultSizeT = nx::core::ConvertTo<usize>::convert(tokens[1]);
  pointDims[0] = convertResultSizeT.value();
  convertResultSizeT = nx::core::ConvertTo<usize>::convert(tokens[2]);
  pointDims[1] = convertResultSizeT.value();
  convertResultSizeT = nx::core::ConvertTo<usize>::convert(tokens[3]);
  pointDims[2] = convertResultSizeT.value();

  CreateImageGeometryAction::DimensionType cellDims(3, 0);
  cellDims[0] = pointDims[0] - 1;
  cellDims[1] = pointDims[1] - 1;
  cellDims[2] = pointDims[2] - 1;

  std::fill(buf.begin(), buf.end(), '\0');     // Splat nulls across the vector
  err = readLine(in, buf.data(), kBufferSize); // Read Line 7 which is the Scaling values
  line = std::string(buf.data());
  tokens = StringUtilities::split(line, ' ');
  CreateImageGeometryAction::SpacingType spacing(3, 0.0f);

  auto convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[1]);
  spacing[0] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[2]);
  spacing[1] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[3]);
  spacing[2] = convertResultF32.value();

  std::fill(buf.begin(), buf.end(), '\0');     // Splat nulls across the vector
  err = readLine(in, buf.data(), kBufferSize); // Read Line 6 which is the Origin values
  line = std::string(buf.data());
  tokens = StringUtilities::split(line, ' ');
  CreateImageGeometryAction::OriginType origin(3, 0.0f);
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[1]);
  origin[0] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[2]);
  origin[1] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[3]);
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
  std::fill(buf.begin(), buf.end(), '\0');     // Splat nulls across the vector
  err = readLine(in, buf.data(), kBufferSize); // Read Line 8 which is the first type of data we are going to read
  line = std::string(buf.data());
  tokens = StringUtilities::split(line, ' ');
  std::string sectionType = std::string(tokens[0]);
  auto convertResultI32 = nx::core::ConvertTo<int32>::convert(tokens[1]);
  int32_t numValues = convertResultI32.value();

  for(int i = 0; i < 2; i++)
  {
    if(sectionType == "CELL_DATA" && m_InputValues->ReadCellData)
    {
      if(cellDims[0] * cellDims[1] * cellDims[2] != numValues)
      {
        setErrorCondition(-61007, std::string("Number of cells does not match number of tuples in the Attribute Matrix"));
        return getErrorCode();
      }
      m_CurrentSectionType = CurrentSectionType::Cell;
      m_CurrentGeomDims = cellDims;
      numValues = this->readDataTypeSection(in, numValues, "point_data");
      sectionType = numValues > 0 ? "POINT_DATA" : "";
    }
    else if(sectionType == "POINT_DATA" && m_InputValues->ReadPointData)
    {
      if(pointDims[0] * pointDims[1] * pointDims[2] != numValues)
      {
        setErrorCondition(-61007, std::string("Number of points does not match number of tuples in the Attribute Matrix"));
        return getErrorCode();
      }
      m_CurrentSectionType = CurrentSectionType::Point;
      m_CurrentGeomDims = pointDims;
      numValues = this->readDataTypeSection(in, numValues, "cell_data");
      sectionType = numValues > 0 ? "CELL_DATA" : "";
    }
  }

  // Close the file since we are done with it.
  in.close();

  return err;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int32_t ReadVtkStructuredPoints::readDataTypeSection(std::istream& in, int32_t numValues, const std::string& nextKeyWord)
{
  std::vector<char> buf(kBufferSize, '\0');

  // Read keywords until end-of-file
  while(this->readString(in, buf.data(), kBufferSize) != 0)
  {
    // read scalar data
    if(strncmp(lowerCase(buf.data(), kBufferSize), "scalars", 7) == 0)
    {
      if(this->readScalarData(in, numValues) <= 0)
      {
        return 0;
      }
    }
    // read vector data
    else if(strncmp(buf.data(), "vectors", 7) == 0)
    {
      if(this->readVectorData(in, numValues) <= 0)
      {
        return 0;
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
      this->readString(in, buf.data(), kBufferSize);
      auto convertResultI32 = nx::core::ConvertTo<int32>::convert({buf.data()});
      return convertResultI32.value();
    }
    else
    {
      // vtkErrorMacro(<< "Unsupported point attribute type: " << line
      //<< " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
      return 0;
    }

    std::fill(buf.begin(), buf.end(), '\0'); // Splat nulls across the vector
  }
  return 0;
}

// ------------------------------------------------------------------------
int32_t ReadVtkStructuredPoints::DecodeString(char* resname, const char* name)
{
  if((resname == nullptr) || (name == nullptr))
  {
    return 0;
  }
  std::ostringstream str;
  size_t cc = 0;
  unsigned int ch;
  size_t len = strlen(name);
  size_t reslen = 0;
  char buffer[10] = "0x";
  while(name[cc] != 0)
  {
    if(name[cc] == '%')
    {
      if(cc <= (len - 3))
      {
        buffer[2] = name[cc + 1];
        buffer[3] = name[cc + 2];
        buffer[4] = 0;
        sscanf(buffer, "%x", &ch);
        str << static_cast<char>(ch);
        cc += 2;
        reslen++;
      }
    }
    else
    {
      str << name[cc];
      reslen++;
    }
    cc++;
  }
  strncpy(resname, str.str().c_str(), reslen + 1);
  resname[reslen] = 0;
  return static_cast<int32_t>(reslen);
}

// ------------------------------------------------------------------------
int32_t ReadVtkStructuredPoints::readScalarData(std::istream& in, int32_t numPts)
{
  // char line[256], name[256], key[256], tableName[256];

  std::vector<char> line(256, '\0');

  //  char buffer[1024];

  std::fill(line.begin(), line.end(), '\0');    // Splat nulls across the vector
  if(this->readLine(in, line.data(), 256) == 0) // Read the rest of the line
  {
    vtkErrorMacro(<< "Cannot read scalar header!"
                  << " for file: " << (m_InputValues->InputFile));
    return 0;
  }
  std::vector<std::string> tokens = StringUtilities::split({line.data()}, ' ');

  if(tokens.size() < 2)
  {
    vtkErrorMacro(<< "Error reading SCALARS header section. Not enough tokens");
    return 0;
  }

  std::string name = tokens[0];
  std::string scalarType = tokens[1];

  usize numComp = 1;
  if(tokens.size() >= 3)
  {
    numComp = static_cast<usize>(std::atoi(tokens[2].c_str()));
  }

  // Done with that line in the file
  std::fill(line.begin(), line.end(), '\0');    // Splat nulls across the vector
  if(this->readLine(in, line.data(), 256) == 0) // Read the rest of the line
  {
    vtkErrorMacro(<< "Cannot read LOOKUP_TABLE line!"
                  << " for file: " << (m_InputValues->InputFile));
    return 0;
  }
  tokens = StringUtilities::split({line.data()}, ' ');
  if(tokens.size() != 2)
  {
    vtkErrorMacro(<< "Error reading SCALARS header section. Not enough tokens");
    return 0;
  }

  std::string key = tokens[0];
  if(key != "LOOKUP_TABLE")
  {
    vtkErrorMacro(<< "Lookup table must be specified with scalar.\n"
                  << "Use \"LOOKUP_TABLE default\" to use default table.");
    return 0;
  }
  std::string tableName = tokens[1];

  int32_t err = 1;

  DataPath arrayDataPath = m_InputValues->PointGeomPath.createChildPath(m_InputValues->PointAttributeMatrixName).createChildPath(name);
  if(m_CurrentSectionType == CurrentSectionType::Cell)
  {
    arrayDataPath = m_InputValues->ImageGeomPath.createChildPath(m_InputValues->CellAttributeMatrixName).createChildPath(name);
  }

  if(m_Preflight)
  {
    nx::core::DataType nxDType = ConvertVtkDataType(scalarType);
    if(nxDType != nx::core::DataType::boolean)
    {
      std::vector<usize> tupleShape = {m_CurrentGeomDims[2], m_CurrentGeomDims[1], m_CurrentGeomDims[0]};
      auto createArrayAction = std::make_unique<CreateArrayAction>(nxDType, tupleShape, std::vector<usize>{numComp}, arrayDataPath);
      m_OutputActions.value().appendAction(std::move(createArrayAction));
      preflightSkipVolume(nxDType, in, m_FileIsBinary, numPts * numComp);
    }

    return err;
  }

  // Read the data
  if(scalarType == "unsigned_char")
  {
    err = readDataChunk<uint8_t>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "char")
  {
    err = readDataChunk<int8_t>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "unsigned_short")
  {
    err = readDataChunk<uint16_t>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "short")
  {
    err = readDataChunk<int16_t>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "unsigned_int")
  {
    err = readDataChunk<uint32_t>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "int")
  {
    err = readDataChunk<int32_t>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "unsigned_long")
  {
    err = readDataChunk<uint64_t>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "long")
  {
    err = readDataChunk<int64_t>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "float")
  {
    err = readDataChunk<float>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }
  else if(scalarType == "double")
  {
    err = readDataChunk<double>(&m_DataStructure, in, m_FileIsBinary, arrayDataPath);
  }

  return err;
}

// ------------------------------------------------------------------------
int32_t ReadVtkStructuredPoints::readVectorData(std::istream& in, int32_t numPts)
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

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));
#endif
  return 1;
}

// -----------------------------------------------------------------------------
int32_t ReadVtkStructuredPoints::parseCoordinateLine(const char* input, size_t& value)
{
  char text[256];
  char text1[256];
  int32_t i = 0;
  int32_t n = sscanf(input, "%s %d %s", text, &i, text1);
  if(n != 3)
  {
    value = -1;
    return -1;
  }
  value = i;
  return 0;
}

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
        err = readDataChunk<uint8_t>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "char")
      {
        err = readDataChunk<int8_t>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "unsigned_short")
      {
        err = readDataChunk<uint16_t>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "short")
      {
        err = readDataChunk<int16_t>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "unsigned_int")
      {
        err = readDataChunk<uint32_t>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "int")
      {
        err = readDataChunk<int32_t>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
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
        err = readDataChunk<float>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
      }
      else if (scalarType == "double")
      {
        err = readDataChunk<double>(attrMat, instream, getInPreflight(), getFileIsBinary(), scalarName, scalarType, scalarNumComps, skipChunk);
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

// ------------------------------------------------------------------------
int32_t ReadVtkStructuredPoints::preflightSkipVolume(nx::core::DataType nxDType, std::istream& in, bool binary, size_t numElements)
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
  return 0;
}
