#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>

#define vtkErrorMacro(msg) std::cout msg

#define kBufferSize 1024

namespace nx::core
{

inline constexpr size_t DEFAULT_BLOCKSIZE = 1048576;

namespace VtkUtilities
{

// -----------------------------------------------------------------------------
template <typename T>
int32_t SkipVolume(std::istream& in, bool binary, size_t totalSize)
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
  size_t chunkSize = nx::core::DEFAULT_BLOCKSIZE;
  // Sanity check the chunk size to make sure it is not any larger than the chunk of data we are about to read
  if(numBytesToRead < nx::core::DEFAULT_BLOCKSIZE)
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
template <typename T>
int32_t ReadDataChunk(const AttributeMatrix& attrMat, std::istream& in, bool inPreflight, bool binary, const std::string& scalarName, int32_t scalarNumComp)
{
  size_t numTuples = attrMat->getNumberOfTuples();

  std::vector<size_t> tDims = attrMat->getTupleDimensions();
  std::vector<size_t> cDims(1, scalarNumComp);

  typename DataArray<T>::Pointer data = DataArray<T>::CreateArray(tDims, cDims, scalarName, !inPreflight);
  data->initializeWithZeros();
  attrMat->insertOrAssign(data);
  if(inPreflight)
  {
    return skipVolume<T>(in, binary, numTuples * scalarNumComp);
  }

  if(binary)
  {
    int32_t err = vtkReadBinaryData<T>(in, data->getPointer(0), numTuples, scalarNumComp);
    if(err < 0)
    {
      std::cout << "Error Reading Binary Data '" << scalarName << "' " << attrMat->getName() << " numTuples = " << numTuples << std::endl;
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
SIMPLNX_EXPORT size_t ParseByteSize(const std::string& text);
SIMPLNX_EXPORT int32_t ReadLine(std::istream& in, std::vector<char>& result, size_t length);
SIMPLNX_EXPORT int32_t ReadString(std::istream& in, std::vector<char>& result, size_t length);
SIMPLNX_EXPORT char* LowerCase(char* str, const size_t len);

SIMPLNX_EXPORT int32_t ReadDataTypeSection(std::istream& in, int32_t numValues, const std::string& nextKeyWord);
SIMPLNX_EXPORT int32_t DecodeString(char* resname, const char* name);
SIMPLNX_EXPORT int32_t ReadScalarData(std::istream& in, int32_t numPts);
SIMPLNX_EXPORT int32_t ReadVectorData(std::istream& in, int32_t numPts);
SIMPLNX_EXPORT int32_t ParseCoordinateLine(const char* input, size_t& value);

} // namespace VtkUtilities
} // namespace nx::core
