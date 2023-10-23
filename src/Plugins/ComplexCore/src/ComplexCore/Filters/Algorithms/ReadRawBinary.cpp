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

#include "ReadRawBinary.hpp"

#include "complex/Common/Bit.hpp"
#include "complex/Common/ComplexConstants.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
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
Result<> ReadBinaryFile(IDataArray& dataArrayPtr, const std::string& filename, uint64 skipHeaderBytes, ChoicesParameter::ValueType endian)
{
  constexpr usize k_DefaultBlocksize = 1000000;

  auto& dataArray = dynamic_cast<DataArray<T>&>(dataArrayPtr);

  const usize fileSize = fs::file_size(filename);
  const usize numBytesToRead = dataArray.getSize() * sizeof(T);
  const int32 err = SanityCheckFileSizeVersusAllocatedSize(numBytesToRead, fileSize, skipHeaderBytes);

  if(err < 0)
  {
    return MakeErrorResult(k_RbrFileTooSmall, "The file size is smaller than the allocated size");
  }

  Result<> result = ImportFromBinaryFile(fs::path(filename), dataArray, skipHeaderBytes, k_DefaultBlocksize);
  if(result.invalid())
  {
    return result;
  }

  if(endian != static_cast<ChoicesParameter::ValueType>(complex::endian::native))
  {
    dataArray.byteSwapElements();
  }

  return result;
}
} // namespace

namespace complex
{
ReadRawBinary::ReadRawBinary(DataStructure& dataStructure, const ReadRawBinaryInputValues& inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ReadRawBinary::~ReadRawBinary() noexcept = default;

Result<> ReadRawBinary::operator()()
{
  return execute();
}

// -----------------------------------------------------------------------------
Result<> ReadRawBinary::execute()
{
  IDataArray& binaryIDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues.createdAttributeArrayPathValue);

  if(binaryIDataArray.getNumberOfComponents() != static_cast<usize>(m_InputValues.numberOfComponentsValue))
  {
    // This was already validated in preflight, so something more fundamental has gone wrong
    throw std::runtime_error(fmt::format("Failed to acquire DataArray from path '{}' with the correct number of components.", m_InputValues.createdAttributeArrayPathValue.toString()));
  }

  const std::string inputFile = m_InputValues.inputFileValue.string();

  switch(m_InputValues.scalarTypeValue)
  {
  case NumericType::int8:
    return ReadBinaryFile<int8>(binaryIDataArray, inputFile, m_InputValues.skipHeaderBytesValue, m_InputValues.endianValue);
  case NumericType::uint8:
    return ReadBinaryFile<uint8>(binaryIDataArray, inputFile, m_InputValues.skipHeaderBytesValue, m_InputValues.endianValue);
  case NumericType::int16:
    return ReadBinaryFile<int16>(binaryIDataArray, inputFile, m_InputValues.skipHeaderBytesValue, m_InputValues.endianValue);
  case NumericType::uint16:
    return ReadBinaryFile<uint16>(binaryIDataArray, inputFile, m_InputValues.skipHeaderBytesValue, m_InputValues.endianValue);
  case NumericType::int32:
    return ReadBinaryFile<int32>(binaryIDataArray, inputFile, m_InputValues.skipHeaderBytesValue, m_InputValues.endianValue);
  case NumericType::uint32:
    return ReadBinaryFile<uint32>(binaryIDataArray, inputFile, m_InputValues.skipHeaderBytesValue, m_InputValues.endianValue);
  case NumericType::int64:
    return ReadBinaryFile<int64>(binaryIDataArray, inputFile, m_InputValues.skipHeaderBytesValue, m_InputValues.endianValue);
  case NumericType::uint64:
    return ReadBinaryFile<uint64>(binaryIDataArray, inputFile, m_InputValues.skipHeaderBytesValue, m_InputValues.endianValue);
  case NumericType::float32:
    return ReadBinaryFile<float32>(binaryIDataArray, inputFile, m_InputValues.skipHeaderBytesValue, m_InputValues.endianValue);
  case NumericType::float64:
    return ReadBinaryFile<float64>(binaryIDataArray, inputFile, m_InputValues.skipHeaderBytesValue, m_InputValues.endianValue);
  default:
    return MakeErrorResult(complex::k_UnsupportedScalarType, "The chosen scalar type is not supported by this filter.");
  }
}
} // namespace complex
