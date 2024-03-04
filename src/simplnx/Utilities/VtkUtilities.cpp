
#include "VtkUtilities.hpp"

namespace nx::core
{
namespace OStreamUtilities
{

size_t ParseByteSize(const std::string& text)
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
int32_t ReadLine(std::istream& in, std::vector<char>& result, size_t length)
{
  in.getline(result.data(), length);
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
int32_t ReadString(std::istream& in, std::vector<char>& result, size_t length)
{
  in.width(length);
  in >> result.data();
  if(in.fail())
  {
    return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------------
char* lowerCase(char* str, const size_t len)
{
  size_t i;
  char* s;

  for(i = 0, s = str; *s != '\0' && i < len; s++, i++)
  {
    *s = tolower(*s);
  }
  return str;
}


// -----------------------------------------------------------------------------
int32_t ReadDataTypeSection(std::istream& in, int32_t numValues, const std::string& nextKeyWord)
{
  std::vector<char> line(kBufferSize, '\0');

  // Read keywords until end-of-file
  while(readString(in, line, kBufferSize) != 0)
  {
    // read scalar data
    if(strncmp(lowerCase(line.data(), kBufferSize), "scalars", 7) == 0)
    {
      if(readScalarData(in, numValues) <= 0)
      {
        return 0;
      }
    }
    // read vector data
    else if(strncmp(line.data(), "vectors", 7) == 0)
    {
      if(readVectorData(in, numValues) <= 0)
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
      if ( ! ReadTensorData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read normals data
    //
    else if ( ! strncmp(line, "normals", 7) )
    {

      if ( ! ReadNormalData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(line, "texture_coordinates", 19) )
    {
      if ( ! ReadTCoordsData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read the global id data
    //
    else if ( ! strncmp(line, "global_ids", 10) )
    {
      if ( ! ReadGlobalIds(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read the pedigree id data
    //
    else if ( ! strncmp(line, "pedigree_ids", 10) )
    {
      if ( ! ReadPedigreeIds(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read color scalars data
    //
    else if ( ! strncmp(line, "color_scalars", 13) )
    {
      if ( ! ReadCoScalarData(a, numPts) )
      {
        return 0;
      }
    }
    //
    // read lookup table. Associate with scalar data.
    //
    else if ( ! strncmp(line, "lookup_table", 12) )
    {
      if ( ! ReadLutData(a) )
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
      if ( ! (f = ReadFieldData()) )
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
    else if(strncmp(line.data(), nextKeyWord.c_str(), 9) == 0)
    {
      bool ok = false;
      if(readString(in, line, 256) != 0)
      {
        if(nextKeyWord == "cell_data")
        {
          DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getVolumeDataContainerName());
          m_CurrentAttrMat = m->getAttributeMatrix(getCellAttributeMatrixName());
          int32_t ncells = std::string(line).toInt(&ok);
          readDataTypeSection(in, ncells, "point_data");
        }
        else if(nextKeyWord == "point_data")
        {
          DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getVertexDataContainerName());
          m_CurrentAttrMat = m->getAttributeMatrix(getVertexAttributeMatrixName());
          int32_t npts = std::string(line).toInt(&ok);
          readDataTypeSection(in, npts, "cell_data");
        }
      }
    }
    else
    {
      // vtkErrorMacro(<< "Unsupported point attribute type: " << line
      //<< " for file: " << (FileName?FileName:"(Null FileName)"));
      return 0;
    }
  }
  return 1;
}


// ------------------------------------------------------------------------
int32_t DecodeString(char* resname, const char* name)
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
int32_t ReadScalarData(std::istream& in, int32_t numPts)
{
  std::vector<char> line (256);
  std::vector<char> name(256);
  std::vector<char> key(256);
  std::vector<char> tableName(256);

  int32_t numComp = 1;
  char buffer[1024];

  if(!((readString(in, buffer, 1024) != 0) && (readString(in, line, 1024) != 0)))
  {
    vtkErrorMacro(<< "Cannot read scalar header!"
                  << " for file: " << (getInputFile()));
    return 0;
  }

  DecodeString(name, buffer);

  if(readString(in, key, 1024) == 0)
  {
    vtkErrorMacro(<< "Cannot read scalar header!"
                  << " for file: " << getInputFile());
    return 0;
  }

  std::string scalarType(line);

  // the next string could be an integer number of components or a lookup table
  if(strcmp(lowerCase(key, 256), "lookup_table") != 0)
  {
    numComp = atoi(key);
    if(numComp < 1 || (readString(in, key, 1024) == 0))
    {
      vtkErrorMacro(<< "Cannot read scalar header!"
                    << " for file: " << getInputFile());
      return 0;
    }
  }

  if(strcmp(lowerCase(key, 256), "lookup_table") != 0)
  {
    vtkErrorMacro(<< "Lookup table must be specified with scalar.\n"
                  << "Use \"LOOKUP_TABLE default\" to use default table.");
    return 0;
  }

  if(readString(in, tableName, 1024) == 0)
  {
    vtkErrorMacro(<< "Cannot read scalar header!"
                  << " for file: " << getInputFile());
    return 0;
  }

  // Suck up the newline at the end of the current line
  readLine(in, line, 1024);

  int32_t err = 1;
  // Read the data
  if(scalarType.compare("unsigned_char") == 0)
  {
    err = readDataChunk<uint8_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType.compare("char") == 0)
  {
    err = readDataChunk<int8_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType.compare("unsigned_short") == 0)
  {
    err = readDataChunk<uint16_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType.compare("short") == 0)
  {
    err = readDataChunk<int16_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType.compare("unsigned_int") == 0)
  {
    err = readDataChunk<uint32_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType.compare("int") == 0)
  {
    err = readDataChunk<int32_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType.compare("unsigned_long") == 0)
  {
    err = readDataChunk<int64_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType.compare("long") == 0)
  {
    err = readDataChunk<uint64_t>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType.compare("float") == 0)
  {
    err = readDataChunk<float>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }
  else if(scalarType.compare("double") == 0)
  {
    err = readDataChunk<double>(m_CurrentAttrMat, in, getInPreflight(), getFileIsBinary(), name, numComp);
  }

  return err;
}


// ------------------------------------------------------------------------
int32_t ReadVectorData(std::istream& in, int32_t numPts)
{
#if 0
  int skipVector = 0;
  char line[256], name[256];
  vtkDataArray* data;
  char buffer[1024];

  if (!(readString(buffer) && readString(line)))
  {
    vtkErrorMacro( << "Cannot read vector data!" << " for file: " << (FileName ? FileName : "(Null FileName)"));
    return 0;
  }
  DecodeString(name, buffer);

  //
  // See whether vector has been already read or vector name (if specified)
  // matches name in file.
  //
  if ( a->GetVectors() != nullptr || (VectorsName && strcmp(name, VectorsName)) )
  {
    skipVector = 1;
  }

  data = vtkDataArray::SafeDownCast(
           ReadArray(line, numPts, 3));
  if ( data != nullptr )
  {
    data->SetName(name);
    if ( ! skipVector )
    {
      a->SetVectors(data);
    }
    else if ( ReadAllVectors )
    {
      a->AddArray(data);
    }
    data->Delete();
  }
  else
  {
    return 0;
  }

  float progress = GetProgress();
  UpdateProgress(progress + 0.5 * (1.0 - progress));
#endif
  return 1;
}

// -----------------------------------------------------------------------------
int32_t ParseCoordinateLine(const char* input, size_t& value)
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


} // namespace OStreamUtilities
} // namespace nx::core
