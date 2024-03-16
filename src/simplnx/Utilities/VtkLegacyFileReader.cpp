#include "VtkLegacyFileReader.hpp"

#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <iostream>
#include <string>
#include <vector>

#define vtkErrorMacro(msg) std::cout msg

using namespace nx::core;

namespace
{
constexpr usize kBufferSize = 1024ULL;
inline constexpr size_t DEFAULT_BLOCKSIZE = 1048576;

} // namespace

VtkLegacyFileReader::VtkLegacyFileReader(const std::string& filePath)
: m_InputFile(filePath)
{
}

VtkLegacyFileReader::~VtkLegacyFileReader()
{
}

void VtkLegacyFileReader::setPreflight(bool value)
{
  m_Preflight = value;
}

// -----------------------------------------------------------------------------
size_t VtkLegacyFileReader::parseByteSize(const std::string& text)
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

// -----------------------------------------------------------------------------
template <typename T>
int32_t skipVolume(std::istream& in, bool binary, size_t totalSize)
{
  int32_t err = 0;
  if(binary)
  {
    std::istream::pos_type pos = in.tellg();
    int64 newPos = totalSize * sizeof(T) + 1;
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
    T tmp;
    for(size_t z = 0; z < totalSize; ++z)
    {
      in >> tmp;
    }
  }
  return err;
}

// -------------------------------------------------------------------------
template <typename T>
int32_t vtkReadBinaryData(std::istream& in, T* data, int32_t numTuples, int32_t numComp)
{
  if(numTuples == 0 || numComp == 0)
  {
    // nothing to read here.
    return 1;
  }

  size_t numBytesToRead = static_cast<size_t>(numTuples) * static_cast<size_t>(numComp) * sizeof(T);
  size_t numRead = 0;
  // Cast our pointer to a pointer that std::istream will take
  char* chunkptr = reinterpret_cast<char*>(data);

  numRead = 0;
  // Now start reading the data in chunks if needed.
  size_t chunkSize = DEFAULT_BLOCKSIZE;
  // Sanity check the chunk size to make sure it is not any larger than the chunk of data we are about to read
  if(numBytesToRead < DEFAULT_BLOCKSIZE)
  {
    chunkSize = numBytesToRead;
  }

  size_t master_counter = 0;
  size_t bytes_read = 0;

  // Now chunk through the file reading up chunks of data that can actually be
  // read in a single read. DEFAULT_BLOCKSIZE will control this.
  while(true)
  {
    in.read(chunkptr, chunkSize);
    bytes_read = in.gcount();

    chunkptr = chunkptr + bytes_read;
    master_counter += bytes_read;

    if(numBytesToRead - master_counter < chunkSize)
    {
      chunkSize = numBytesToRead - master_counter;
    }
    if(master_counter >= numBytesToRead)
    {
      break;
    }
    if(in.good())
    {
      // std::cout << "all data read successfully." << in.gcount() << std::endl;
    }

    if((in.rdstate() & std::ifstream::failbit) != 0)
    {
      std::cout << "FAIL " << in.gcount() << " could be read. Needed " << chunkSize << " total bytes read = " << master_counter << std::endl;
      return -12020;
    }
    if((in.rdstate() & std::ifstream::eofbit) != 0)
    {
      std::cout << "EOF " << in.gcount() << " could be read. Needed " << chunkSize << " total bytes read = " << master_counter << std::endl;
      return -12021;
    }
    if((in.rdstate() & std::ifstream::badbit) != 0)
    {
      std::cout << "BAD " << in.gcount() << " could be read. Needed " << chunkSize << " total bytes read = " << master_counter << std::endl;
      return -12021;
    }
  }
  return 0;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
template <typename T>
int32_t readDataChunk(AttributeMatrix& attrMat, std::istream& in, bool inPreflight, bool binary, const std::string& scalarName, int32_t scalarNumComp)
{
  size_t numTuples = attrMat.getNumTuples();

  std::vector<size_t> tDims = attrMat.getShape();
  std::vector<size_t> cDims(1, scalarNumComp);

  typename DataArray<T>::Pointer data = DataArray<T>::CreateArray(tDims, cDims, scalarName, !inPreflight);
  data->initializeWithZeros();
  // TODO: FIX THIS ==> attrMat->insertOrAssign(data);
  if(inPreflight)
  {
    return skipVolume<T>(in, binary, numTuples * scalarNumComp);
  }

  if(binary)
  {
    int32_t err = vtkReadBinaryData<T>(in, data->getPointer(0), numTuples, scalarNumComp);
    if(err < 0)
    {
      std::cout << "Error Reading Binary Data '" << scalarName << "' " << attrMat.getName() << " numTuples = " << numTuples << std::endl;
      return err;
    }
    if(nx::core::checkEndian() == nx::core::endian::big)
    {
      data->byteSwapElements();
    }
  }
  else
  {
    T value = static_cast<T>(0.0);
    size_t totalSize = numTuples * scalarNumComp;
    for(size_t i = 0; i < totalSize; ++i)
    {
      in >> value;
      data->setValue(i, value);
    }
  }

  return 0;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int32_t VtkLegacyFileReader::readHeader()
{
  return 0;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int32_t VtkLegacyFileReader::readLine(std::istream& in, char* result, size_t length)
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
//
// --------------------------------------------------------------------------
int32_t VtkLegacyFileReader::readString(std::istream& in, char* result, size_t length)
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
//
// -----------------------------------------------------------------------------
char* VtkLegacyFileReader::lowerCase(char* str, const size_t len)
{
  size_t i;
  char* s;

  for(i = 0, s = str; *s != '\0' && i < len; s++, i++)
  {
    *s = tolower(*s);
  }
  return str;
}

void VtkLegacyFileReader::setErrorCondition(int error, const std::string& message)
{
  m_ErrorCode = error;
  m_ErrorMessage = message;
}

int32 VtkLegacyFileReader::getErrorCode() const
{
  return m_ErrorCode;
}

void VtkLegacyFileReader::setComment(const std::string& comment)
{
  m_Comment = comment;
}

void VtkLegacyFileReader::setFileIsBinary(bool value)
{
  m_FileIsBinary = value;
}

void VtkLegacyFileReader::setDatasetType(const std::string& dataSetType)
{
  m_DatasetType = dataSetType;
}

// -----------------------------------------------------------------------------
int32_t VtkLegacyFileReader::readFile()
{
  int32_t err = 0;

  // TODO: Implement This ===>  DataContainer::Pointer volDc = getDataContainerArray()->getDataContainer(getVolumeDataContainerName());
  // TODO: Implement This ===> AttributeMatrix::Pointer volAm = volDc->getAttributeMatrix(getCellAttributeMatrixName());
  //
  // TODO: Implement This ===> DataContainer::Pointer vertDc = getDataContainerArray()->getDataContainer(getVertexDataContainerName());
  // TODO: Implement This ===> AttributeMatrix::Pointer vertAm = vertDc->getAttributeMatrix(getVertexAttributeMatrixName());

  std::ifstream in(m_InputFile, std::ios_base::in | std::ios_base::binary);

  if(!in.is_open())
  {
    std::string msg = fmt::format("Error opening output file '%1'", m_InputFile);
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
  std::vector<size_t> dims(3, 0);
  line = std::string(buf.data());
  auto tokens = StringUtilities::split(line, ' ');

  auto convertResultI32 = nx::core::ConvertTo<int32>::convert(tokens[1]);
  dims[0] = convertResultI32.value();
  convertResultI32 = nx::core::ConvertTo<int32>::convert(tokens[2]);
  dims[1] = convertResultI32.value();
  convertResultI32 = nx::core::ConvertTo<int32>::convert(tokens[3]);
  dims[2] = convertResultI32.value();

  std::vector<size_t> tDims(3, 0);
  tDims[0] = dims[0];
  tDims[1] = dims[1];
  tDims[2] = dims[2];
  // TODO: Implement This ===>  vertAm->setTupleDimensions(tDims);
  // TODO: Implement This ===>  vertDc->getGeometryAs<ImageGeom>()->setDimensions(dims.data());

  tDims[0] = dims[0] - 1;
  tDims[1] = dims[1] - 1;
  tDims[2] = dims[2] - 1;
  // TODO: Implement This ===>  volAm->setTupleDimensions(tDims);
  // TODO: Implement This ===>  volDc->getGeometryAs<ImageGeom>()->setDimensions(tDims.data());

  std::fill(buf.begin(), buf.end(), '\0');     // Splat nulls across the vector
  err = readLine(in, buf.data(), kBufferSize); // Read Line 7 which is the Scaling values
  line = std::string(buf.data());
  tokens = StringUtilities::split(line, ' ');
  float resolution[3];

  auto convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[1]);
  resolution[0] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[2]);
  resolution[1] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[3]);
  resolution[2] = convertResultF32.value();

  // TODO: Implement This ===>  volDc->getGeometryAs<ImageGeom>()->setSpacing(resolution);
  // TODO: Implement This ===>  vertDc->getGeometryAs<ImageGeom>()->setSpacing(resolution);

  std::fill(buf.begin(), buf.end(), '\0');     // Splat nulls across the vector
  err = readLine(in, buf.data(), kBufferSize); // Read Line 6 which is the Origin values
  line = std::string(buf.data());
  tokens = StringUtilities::split(line, ' ');
  float origin[3];
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[1]);
  origin[0] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[2]);
  origin[1] = convertResultF32.value();
  convertResultF32 = nx::core::ConvertTo<float32>::convert(tokens[3]);
  origin[2] = convertResultF32.value();

  // TODO: Implement This ===>  volDc->getGeometryAs<ImageGeom>()->setOrigin(origin);
  // TODO: Implement This ===>  vertDc->getGeometryAs<ImageGeom>()->setOrigin(origin);

  // Read the first key word which should be POINT_DATA or CELL_DATA
  std::fill(buf.begin(), buf.end(), '\0');     // Splat nulls across the vector
  err = readLine(in, buf.data(), kBufferSize); // Read Line 6 which is the first type of data we are going to read
  line = std::string(buf.data());
  tokens = StringUtilities::split(line, ' ');
  std::string word = std::string(tokens[0]);
  int32_t npts = 0, ncells = 0;
  int32_t numPts = 0;

  if(StringUtilities::starts_with(word, "CELL_DATA"))
  {
    // TODO: Implement This ===>   DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getVolumeDataContainerName());
    // TODO: Implement This ===>    m_CurrentAttrMat = m->getAttributeMatrix(getCellAttributeMatrixName());

    convertResultI32 = nx::core::ConvertTo<int32>::convert(tokens[1]);
    ncells = convertResultI32.value();

    // TODO: Implement This ===>    if(m_CurrentAttrMat->getNumberOfTuples() != ncells)
    // TODO: Implement This ===>    {
    // TODO: Implement This ===>      setErrorCondition(-61006, std::string("Number of cells does not match number of tuples in the Attribute Matrix"));
    // TODO: Implement This ===>      return getErrorCode();
    // TODO: Implement This ===>    }
    this->readDataTypeSection(in, ncells, "point_data");
  }
  else if(StringUtilities::starts_with(word, "POINT_DATA"))
  {
    // TODO: Implement This ===>   DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getVertexDataContainerName());
    // TODO: Implement This ===>   m_CurrentAttrMat = m->getAttributeMatrix(getVertexAttributeMatrixName());
    convertResultI32 = nx::core::ConvertTo<int32>::convert(tokens[1]);
    npts = convertResultI32.value();

    // TODO: Implement This ===>    if(m_CurrentAttrMat->getNumberOfTuples() != npts)
    // TODO: Implement This ===>    {
    // TODO: Implement This ===>      setErrorCondition(-61007, std::string("Number of points does not match number of tuples in the Attribute Matrix"));
    // TODO: Implement This ===>      return getErrorCode();
    // TODO: Implement This ===>    }
    this->readDataTypeSection(in, numPts, "cell_data");
  }

  // Close the file since we are done with it.
  in.close();

  return err;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int32_t VtkLegacyFileReader::readDataTypeSection(std::istream& in, int32_t numValues, const std::string& nextKeyWord)
{
  std::vector<char> buf(kBufferSize, '\0');
  char* line = buf.data();

  // Read keywords until end-of-file
  while(this->readString(in, line, kBufferSize) != 0)
  {
    // read scalar data
    if(strncmp(lowerCase(line, kBufferSize), "scalars", 7) == 0)
    {
      if(this->readScalarData(in, numValues) <= 0)
      {
        return 0;
      }
    }
    // read vector data
    else if(strncmp(line, "vectors", 7) == 0)
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
    else if ( ! strncmp(line, "tensors", 7) )
    {
      if ( ! this->ReadTensorData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read normals data
    //
    else if ( ! strncmp(line, "normals", 7) )
    {

      if ( ! this->ReadNormalData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(line, "texture_coordinates", 19) )
    {
      if ( ! this->ReadTCoordsData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read the global id data
    //
    else if ( ! strncmp(line, "global_ids", 10) )
    {
      if ( ! this->ReadGlobalIds(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read the pedigree id data
    //
    else if ( ! strncmp(line, "pedigree_ids", 10) )
    {
      if ( ! this->ReadPedigreeIds(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read color scalars data
    //
    else if ( ! strncmp(line, "color_scalars", 13) )
    {
      if ( ! this->ReadCoScalarData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read lookup table. Associate with scalar data.
    //
    else if ( ! strncmp(line, "lookup_table", 12) )
    {
      if ( ! this->ReadLutData(a) )
      {
        return 0;
      }
    }
    //
    // read field of data
    //
    else if ( ! strncmp(line, "field", 5) )
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
    else if(strncmp(line, nextKeyWord.c_str(), 9) == 0)
    {
      bool ok = false;
      if(readString(in, line, 256) != 0)
      {
        if(nextKeyWord == "cell_data")
        {
          // TODO: Implement This ===>     DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getVolumeDataContainerName());
          // TODO: Implement This ===>     m_CurrentAttrMat = m->getAttributeMatrix(getCellAttributeMatrixName());
          auto convertResultI32 = nx::core::ConvertTo<int32>::convert(line);
          int32_t ncells = convertResultI32.value();
          this->readDataTypeSection(in, ncells, "point_data");
        }
        else if(nextKeyWord == "point_data")
        {
          // TODO: Implement This ===>      DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getVertexDataContainerName());
          // TODO: Implement This ===>      m_CurrentAttrMat = m->getAttributeMatrix(getVertexAttributeMatrixName());
          auto convertResultI32 = nx::core::ConvertTo<int32>::convert(line);
          int32_t npts = convertResultI32.value();
          this->readDataTypeSection(in, npts, "cell_data");
        }
      }
    }
    else
    {
      // vtkErrorMacro(<< "Unsupported point attribute type: " << line
      //<< " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
      return 0;
    }
  }
  return 1;
}

// ------------------------------------------------------------------------
int32_t VtkLegacyFileReader::DecodeString(char* resname, const char* name)
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
int32_t VtkLegacyFileReader::readScalarData(std::istream& in, int32_t numPts)
{
  char line[256], name[256], key[256], tableName[256];
  int32_t numComp = 1;
  char buffer[1024];

  if(!((this->readString(in, buffer, 1024) != 0) && (this->readString(in, line, 1024) != 0)))
  {
    vtkErrorMacro(<< "Cannot read scalar header!"
                  << " for file: " << (m_InputFile));
    return 0;
  }

  this->DecodeString(name, buffer);

  if(this->readString(in, key, 1024) == 0)
  {
    vtkErrorMacro(<< "Cannot read scalar header!"
                  << " for file: " << m_InputFile);
    return 0;
  }

  std::string scalarType(line);

  // the next string could be an integer number of components or a lookup table
  if(strcmp(this->lowerCase(key, 256), "lookup_table") != 0)
  {
    numComp = atoi(key);
    if(numComp < 1 || (this->readString(in, key, 1024) == 0))
    {
      vtkErrorMacro(<< "Cannot read scalar header!"
                    << " for file: " << m_InputFile);
      return 0;
    }
  }

  if(strcmp(this->lowerCase(key, 256), "lookup_table") != 0)
  {
    vtkErrorMacro(<< "Lookup table must be specified with scalar.\n"
                  << "Use \"LOOKUP_TABLE default\" to use default table.");
    return 0;
  }

  if(this->readString(in, tableName, 1024) == 0)
  {
    vtkErrorMacro(<< "Cannot read scalar header!"
                  << " for file: " << m_InputFile);
    return 0;
  }

  // Suck up the newline at the end of the current line
  this->readLine(in, line, 1024);

  int32_t err = 1;
#if IMPLEMENT_THIS
  // Read the data
  if(scalarType == "unsigned_char")
  {
    err = readDataChunk<uint8_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType == "char")
  {
    err = readDataChunk<int8_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType == "unsigned_short")
  {
    err = readDataChunk<uint16_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType == "short")
  {
    err = readDataChunk<int16_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType == "unsigned_int")
  {
    err = readDataChunk<uint32_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType == "int")
  {
    err = readDataChunk<int32_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType == "unsigned_long")
  {
    err = readDataChunk<int64_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType == "long")
  {
    err = readDataChunk<uint64_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType == "float")
  {
    err = readDataChunk<float>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType == "double")
  {
    err = readDataChunk<double>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
#endif
  return err;
}

// ------------------------------------------------------------------------
int32_t VtkLegacyFileReader::readVectorData(std::istream& in, int32_t numPts)
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
int32_t VtkLegacyFileReader::parseCoordinateLine(const char* input, size_t& value)
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
void VtkLegacyFileReader::readData(std::istream& instream)
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
