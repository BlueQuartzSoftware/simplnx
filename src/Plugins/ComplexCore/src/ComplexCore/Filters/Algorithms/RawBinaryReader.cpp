/* ============================================================================
 * Copyright (c) 2009-2019 BlueQuartz Software, LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The code contained herein was partially funded by the following contracts:
 *    United States Air Force Prime Contract FA8650-07-D-5800
 *    United States Air Force Prime Contract FA8650-10-D-5210
 *    United States Prime Contract Navy N00173-07-C-2068
 *    United States Air Force Prime Contract FA8650-15-D-5231
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "RawBinaryReader.hpp"

#include <algorithm>
#include <cstddef>

#include "complex/Common/ComplexConstants.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"

#if defined(_MSC_VER)
const auto FSEEK = _fseeki64;
#else
const auto FSEEK = std::fseek;
#endif

namespace complex
{

namespace
{
#ifdef CMP_WORDS_BIGENDIAN
constexpr int32_t k_EndianCheck = 0;
#else
constexpr int32_t k_EndianCheck = 1;
#endif

constexpr int32_t k_RbrNoError = 0;
constexpr int32_t k_RbrFileNotOpen = -1000;
constexpr int32_t k_RbrFileTooSmall = -1010;
constexpr int32_t k_RbrFileTooBig = -1020;
constexpr int32_t k_RbrDAError = -1040;
constexpr int32_t k_RbrComponentError = -1050;
constexpr int32_t k_RbrDANull = -1060;

// -----------------------------------------------------------------------------
int32_t SanityCheckFileSizeVersusAllocatedSize(size_t allocatedBytes, size_t fileSize, size_t skipHeaderBytes)
{
  if(fileSize - skipHeaderBytes < allocatedBytes)
  {
    return -1;
  }
  if(fileSize - skipHeaderBytes > allocatedBytes)
  {
    return 1;
  }
  // File Size and Allocated Size are equal so we are good to go
  return 0;
}

// -----------------------------------------------------------------------------
template <typename T>
int32_t readBinaryFile(IDataArray* dataArrayPtr, const std::string& filename, uint64_t skipHeaderBytes, ChoicesParameter::ValueType endian)
{
  constexpr size_t k_DefaultBlocksize = 1048576;

  DataArray<T>* dataArray = dynamic_cast<DataArray<T>*>(dataArrayPtr);

  if(dataArray == nullptr)
  {
    return k_RbrDANull;
  }

  const size_t fileSize = std::filesystem::file_size(filename);
  const size_t numBytesToRead = dataArray->getSize() * sizeof(T);
  int32_t err = SanityCheckFileSizeVersusAllocatedSize(numBytesToRead, fileSize, skipHeaderBytes);

  if(err < 0)
  {
    return k_RbrFileTooSmall;
  }
  if(err > 0)
  {
    return k_RbrFileTooBig;
  }

  FILE* f = std::fopen(filename.c_str(), "rb");
  if(f == nullptr)
  {
    return k_RbrFileNotOpen;
  }

  // Skip some header bytes if the user asked for it.
  if(skipHeaderBytes > 0)
  {
    FSEEK(f, skipHeaderBytes, SEEK_SET);
  }

  //  std::byte* chunkptr = reinterpret_cast<std::byte*>(dataArray->data());
  std::byte* chunkptr = reinterpret_cast<std::byte*>(dataArray->template getIDataStoreAs<DataStore<T>>()->data());

  // Now start reading the data in chunks if needed.
  size_t chunkSize = std::min(numBytesToRead, k_DefaultBlocksize);

  size_t master_counter = 0;
  while(master_counter < numBytesToRead)
  {
    size_t bytes_read = std::fread(chunkptr, sizeof(std::byte), chunkSize, f);
    chunkptr += bytes_read;
    master_counter += bytes_read;

    size_t bytesLeft = numBytesToRead - master_counter;

    if(bytesLeft < chunkSize)
    {
      chunkSize = bytesLeft;
    }
  }

  if(endian == k_EndianCheck)
  {
    dataArray->byteSwapElements();
  }

  std::fclose(f);

  return k_RbrNoError;
}
} // namespace

using namespace complex;

RawBinaryReader::RawBinaryReader(DataStructure& dataStructure, RawBinaryReaderInputValues* inputValues, const IFilter* filter, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_Filter(filter)
, m_MessageHandler(mesgHandler)
{
}

RawBinaryReader::~RawBinaryReader() noexcept = default;

Result<> RawBinaryReader::operator()()
{
  return execute();
}

// -----------------------------------------------------------------------------
Result<> RawBinaryReader::execute()
{
  IDataArray* binaryIDataArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->createdAttributeArrayPathValue);
  if(binaryIDataArray == nullptr)
  {
    return MakeErrorResult(k_RbrDAError, "Can't obtain DataArray from path '" + m_InputValues->createdAttributeArrayPathValue.toString() + "'.");
  }

  if(binaryIDataArray->getNumberOfComponents() != static_cast<size_t>(m_InputValues->numberOfComponentsValue))
  {
    return MakeErrorResult(k_RbrComponentError, "Failed to acquire DataArray from path '" + m_InputValues->createdAttributeArrayPathValue.toString() + "' with the correct number of components.");
  }

  const std::string inputFile = m_InputValues->inputFileValue;

  int32_t err = 0;
  switch(m_InputValues->scalarTypeValue)
  {
  case NumericType::int8:
    err = readBinaryFile<int8_t>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    break;
  case NumericType::uint8:
    err = readBinaryFile<uint8_t>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    break;
  case NumericType::int16:
    err = readBinaryFile<int16_t>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    break;
  case NumericType::uint16:
    err = readBinaryFile<uint16_t>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    break;
  case NumericType::int32:
    err = readBinaryFile<int32_t>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    break;
  case NumericType::uint32:
    err = readBinaryFile<uint32_t>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    break;
  case NumericType::int64:
    err = readBinaryFile<int64_t>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    break;
  case NumericType::uint64:
    err = readBinaryFile<uint64_t>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    break;
  case NumericType::float32:
    err = readBinaryFile<float>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    break;
  case NumericType::float64:
    err = readBinaryFile<double>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    break;
    //  case NumericType::boolean:
    //    err = readBinaryFile<uint8_t>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
    //    break;
  default:
    throw std::runtime_error("The chosen scalar type is not supported.");
  }

  switch(err)
  {
  case k_RbrFileNotOpen:
    return MakeErrorResult(k_RbrFileNotOpen, "Unable to open the specified file");
  case k_RbrFileTooSmall:
    return MakeErrorResult(k_RbrFileTooSmall, "The file size is smaller than the allocated size");
  case k_RbrFileTooBig:
    return MakeWarningVoidResult(k_RbrFileTooBig, "The file size is larger than the allocated size");
  case k_RbrDANull:
    return MakeErrorResult(k_RbrDANull, "Failed DataArray cast");
  default:
    return {};
  }
}

} // namespace complex
