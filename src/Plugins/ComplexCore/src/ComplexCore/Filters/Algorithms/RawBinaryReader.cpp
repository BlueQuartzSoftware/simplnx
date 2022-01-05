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

#include "complex/Common/Bit.hpp"
#include "complex/Common/ComplexConstants.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
#if defined(_MSC_VER)
const auto FSEEK = _fseeki64;
#else
const auto FSEEK = std::fseek;
#endif

#if COMPLEX_BYTE_ORDER == little
constexpr int32 k_EndianCheck = 0;
#else
constexpr int32 k_EndianCheck = 1;
#endif

constexpr int32 k_RbrFileNotOpen = -1000;
constexpr int32 k_RbrFileTooSmall = -1010;
constexpr int32 k_RbrFileTooBig = -1020;

// -----------------------------------------------------------------------------
int32 SanityCheckFileSizeVersusAllocatedSize(usize allocatedBytes, usize fileSize, usize skipHeaderBytes)
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
Result<> readBinaryFile(IDataArray& dataArrayPtr, const std::string& filename, uint64 skipHeaderBytes, ChoicesParameter::ValueType endian)
{
  constexpr usize k_DefaultBlocksize = 1048576;

  DataArray<T>& dataArray = dynamic_cast<DataArray<T>&>(dataArrayPtr);

  const usize fileSize = fs::file_size(filename);
  const usize numBytesToRead = dataArray.getSize() * sizeof(T);
  int32 err = SanityCheckFileSizeVersusAllocatedSize(numBytesToRead, fileSize, skipHeaderBytes);

  if(err < 0)
  {
    return MakeErrorResult(k_RbrFileTooSmall, "The file size is smaller than the allocated size");
  }
  if(err > 0)
  {
    return MakeWarningVoidResult(k_RbrFileTooBig, "The file size is larger than the allocated size");
  }

  FILE* f = std::fopen(filename.c_str(), "rb");
  if(f == nullptr)
  {
    return MakeErrorResult(k_RbrFileNotOpen, "Unable to open the specified file");
  }

  // Skip some header bytes if the user asked for it.
  if(skipHeaderBytes > 0)
  {
    FSEEK(f, skipHeaderBytes, SEEK_SET);
  }

  //  std::byte* chunkptr = reinterpret_cast<std::byte*>(dataArray->data());
  std::byte* chunkptr = reinterpret_cast<std::byte*>(dataArray.template getIDataStoreAs<DataStore<T>>()->data());

  // Now start reading the data in chunks if needed.
  usize chunkSize = std::min(numBytesToRead, k_DefaultBlocksize);

  usize master_counter = 0;
  while(master_counter < numBytesToRead)
  {
    usize bytes_read = std::fread(chunkptr, sizeof(std::byte), chunkSize, f);
    chunkptr += bytes_read;
    master_counter += bytes_read;

    usize bytesLeft = numBytesToRead - master_counter;

    if(bytesLeft < chunkSize)
    {
      chunkSize = bytesLeft;
    }
  }

  if(endian != k_EndianCheck)
  {
    dataArray.byteSwapElements();
  }

  std::fclose(f);

  return {};
}
} // namespace

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
  IDataArray& binaryIDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->createdAttributeArrayPathValue);

  if(binaryIDataArray.getNumberOfComponents() != static_cast<usize>(m_InputValues->numberOfComponentsValue))
  {
    // This was already validated in preflight, so something more fundamental has gone wrong
    throw std::runtime_error(fmt::format("Failed to acquire DataArray from path '{}' with the correct number of components.", m_InputValues->createdAttributeArrayPathValue.toString()));
  }

  const std::string inputFile = m_InputValues->inputFileValue.string();

  switch(m_InputValues->scalarTypeValue)
  {
  case NumericType::int8:
    return readBinaryFile<int8>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
  case NumericType::uint8:
    return readBinaryFile<uint8>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
  case NumericType::int16:
    return readBinaryFile<int16>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
  case NumericType::uint16:
    return readBinaryFile<uint16>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
  case NumericType::int32:
    return readBinaryFile<int32>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
  case NumericType::uint32:
    return readBinaryFile<uint32>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
  case NumericType::int64:
    return readBinaryFile<int64>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
  case NumericType::uint64:
    return readBinaryFile<uint64>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
  case NumericType::float32:
    return readBinaryFile<float32>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
  case NumericType::float64:
    return readBinaryFile<float64>(binaryIDataArray, inputFile, m_InputValues->skipHeaderBytesValue, m_InputValues->endianValue);
  default:
    return MakeErrorResult(complex::k_UnsupportedScalarType, "The chosen scalar type is not supported by this filter.");
  }
}
