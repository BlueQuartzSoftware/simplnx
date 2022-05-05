/* ============================================================================
 * Copyright (c) 2022-2022 BlueQuartz Software, LLC
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
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once

#include "ComplexCore/utils/ParserFunctors.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

class AbstractDataParser
{
public:
  virtual ~AbstractDataParser() = default;

  AbstractDataParser(const AbstractDataParser&) = delete;            // Copy Constructor Not Implemented
  AbstractDataParser(AbstractDataParser&&) = delete;                 // Move Constructor Not Implemented
  AbstractDataParser& operator=(const AbstractDataParser&) = delete; // Copy Assignment Not Implemented
  AbstractDataParser& operator=(AbstractDataParser&&) = delete;      // Move Assignment

  std::string columnName() const
  {
    return m_ColumnName;
  }

  usize columnIndex() const
  {
    return m_ColumnIndex;
  }

  const IDataArray& dataArray() const
  {
    return m_DataArray;
  }

  virtual Result<> parse(const std::string& token, size_t index) = 0;

protected:
  AbstractDataParser(IDataArray& array, const std::string& columnName, usize columnIndex)
  : m_DataArray(array)
  , m_ColumnName(columnName)
  , m_ColumnIndex(columnIndex)
  {
  }

private:
  IDataArray& m_DataArray;
  usize m_ColumnIndex = 0;
  std::string m_ColumnName;
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
template <typename ArrayType, class F, typename T>
class ASCIIDataParser : public AbstractDataParser
{
public:
  ASCIIDataParser(ArrayType& array, const std::string& name, usize index)
  : AbstractDataParser(array, name, index)
  , m_Array(array)
  {
  }
  ~ASCIIDataParser() override = default;

  ASCIIDataParser(const ASCIIDataParser&) = delete;            // Copy Constructor Not Implemented
  ASCIIDataParser(ASCIIDataParser&&) = delete;                 // Move Constructor Not Implemented
  ASCIIDataParser& operator=(const ASCIIDataParser&) = delete; // Copy Assignment Not Implemented
  ASCIIDataParser& operator=(ASCIIDataParser&&) = delete;      // Move Assignment

  Result<> parse(const std::string& token, size_t index) override
  {
    //    Result<T> parseResult = F()(token);
    Result<T> parseResult = ConvertTo<T>::convert(token);
    if(parseResult.valid())
    {
      m_Array[index] = parseResult.value();
    }

    return ConvertResult(std::move(parseResult));
  }

private:
  ArrayType& m_Array;
};

using Int8Parser = ASCIIDataParser<Int8Array, SignedIntegerFunctor<int8>, int8>;
using UInt8Parser = ASCIIDataParser<UInt8Array, UnsignedIntegerFunctor<uint8>, uint8>;

using Int16Parser = ASCIIDataParser<Int16Array, SignedIntegerFunctor<int16>, int16>;
using UInt16Parser = ASCIIDataParser<UInt16Array, UnsignedIntegerFunctor<uint16>, uint16>;

using Int32Parser = ASCIIDataParser<Int32Array, SignedIntegerFunctor<int32>, int32>;
using UInt32Parser = ASCIIDataParser<UInt32Array, UnsignedIntegerFunctor<uint32>, uint32>;

using Int64Parser = ASCIIDataParser<Int64Array, SignedIntegerFunctor<int64>, int64>;
using UInt64Parser = ASCIIDataParser<UInt64Array, UnsignedIntegerFunctor<uint64>, uint64>;

using Float32Parser = ASCIIDataParser<Float32Array, FloatingPointFunctor<float32>, float32>;
using Float64Parser = ASCIIDataParser<Float64Array, FloatingPointFunctor<float64>, float64>;

// using StringParser = Parser<StringArray, StringFunctor, std::string>;
