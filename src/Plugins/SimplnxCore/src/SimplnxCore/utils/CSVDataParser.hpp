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

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

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
template <typename ArrayType, typename T>
class CSVDataParser : public AbstractDataParser
{
public:
  CSVDataParser(ArrayType& array, const std::string& name, usize index)
  : AbstractDataParser(array, name, index)
  , m_Array(array)
  {
  }
  ~CSVDataParser() override = default;

  CSVDataParser(const CSVDataParser&) = delete;            // Copy Constructor Not Implemented
  CSVDataParser(CSVDataParser&&) = delete;                 // Move Constructor Not Implemented
  CSVDataParser& operator=(const CSVDataParser&) = delete; // Copy Assignment Not Implemented
  CSVDataParser& operator=(CSVDataParser&&) = delete;      // Move Assignment

  Result<> parse(const std::string& token, size_t index) override
  {
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

using Int8Parser = CSVDataParser<Int8Array, int8>;
using UInt8Parser = CSVDataParser<UInt8Array, uint8>;

using Int16Parser = CSVDataParser<Int16Array, int16>;
using UInt16Parser = CSVDataParser<UInt16Array, uint16>;

using Int32Parser = CSVDataParser<Int32Array, int32>;
using UInt32Parser = CSVDataParser<UInt32Array, uint32>;

using Int64Parser = CSVDataParser<Int64Array, int64>;
using UInt64Parser = CSVDataParser<UInt64Array, uint64>;

using Float32Parser = CSVDataParser<Float32Array, float32>;
using Float64Parser = CSVDataParser<Float64Array, float64>;

using BoolParser = CSVDataParser<BoolArray, bool>;

// using StringParser = Parser<StringArray, std::string>;
