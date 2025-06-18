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

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <optional>

namespace nx::core
{
enum class CSVType : uint8
{
  int8,
  uint8,
  int16,
  uint16,
  int32,
  uint32,
  int64,
  uint64,
  float32,
  float64,
  boolean,
  string
};

inline constexpr StringLiteral CSVTypeToHumanString(CSVType csvType)
{
  switch(csvType)
  {
  case CSVType::int8: {
    return "signed int 8 bit";
  }
  case CSVType::uint8: {
    return "unsigned int 8 bit";
  }
  case CSVType::int16: {
    return "signed int 16 bit";
  }
  case CSVType::uint16: {
    return "unsigned int 16 bit";
  }
  case CSVType::int32: {
    return "signed int 32 bit";
  }
  case CSVType::uint32: {
    return "unsigned int 32 bit";
  }
  case CSVType::int64: {
    return "signed int 64 bit";
  }
  case CSVType::uint64: {
    return "unsigned int 64 bit";
  }
  case CSVType::float32: {
    return "float 32";
  }
  case CSVType::float64: {
    return "double 64";
  }
  case CSVType::boolean: {
    return "boolean";
  }
  case CSVType::string: {
    return "string";
  }
  default:
    throw std::runtime_error("nx::core::CSVTypeToHumanString: Unknown CSVType");
  }
}

/**
 *
 * @param humanReadable Strings that would be good for a User interface
 * @return
 */
inline const std::vector<std::string>& GetAllCSVTypesAsHumanStrings()
{
  static const std::vector<std::string> dataTypes = {
      CSVTypeToHumanString(nx::core::CSVType::int8),    CSVTypeToHumanString(nx::core::CSVType::uint8),   CSVTypeToHumanString(nx::core::CSVType::int16),
      CSVTypeToHumanString(nx::core::CSVType::uint16),  CSVTypeToHumanString(nx::core::CSVType::int32),   CSVTypeToHumanString(nx::core::CSVType::uint32),
      CSVTypeToHumanString(nx::core::CSVType::int64),   CSVTypeToHumanString(nx::core::CSVType::uint64),  CSVTypeToHumanString(nx::core::CSVType::float32),
      CSVTypeToHumanString(nx::core::CSVType::float64), CSVTypeToHumanString(nx::core::CSVType::boolean), CSVTypeToHumanString(nx::core::CSVType::string)};
  return dataTypes;
}

/**
 * @brief Returns a CSVType for the passed in index
 * @param index
 * @return
 */
inline std::optional<CSVType> IndexToCSVType(usize index)
{
  switch(index)
  {
  case static_cast<int>(CSVType::int8):
  case static_cast<int>(CSVType::uint8):
  case static_cast<int>(CSVType::int16):
  case static_cast<int>(CSVType::uint16):
  case static_cast<int>(CSVType::int32):
  case static_cast<int>(CSVType::uint32):
  case static_cast<int>(CSVType::int64):
  case static_cast<int>(CSVType::uint64):
  case static_cast<int>(CSVType::float32):
  case static_cast<int>(CSVType::float64):
  case static_cast<int>(CSVType::boolean):
  case static_cast<int>(CSVType::string):
    return static_cast<CSVType>(index);
  default:
    return {};
  }
}

/**
 * @brief Converts CSVType to DataType. Fails on CSVType::string and CSVType::error.
 * @param numericType
 * @return
 */
inline constexpr DataType ConvertCSVTypeToDataType(CSVType csvType)
{
  switch(csvType)
  {
  case CSVType::int8: {
    return DataType::int8;
  }
  case CSVType::int16: {
    return DataType::int16;
  }
  case CSVType::int32: {
    return DataType::int32;
  }
  case CSVType::int64: {
    return DataType::int64;
  }
  case CSVType::uint8: {
    return DataType::uint8;
  }
  case CSVType::uint16: {
    return DataType::uint16;
  }
  case CSVType::uint32: {
    return DataType::uint32;
  }
  case CSVType::uint64: {
    return DataType::uint64;
  }
  case CSVType::float32: {
    return DataType::float32;
  }
  case CSVType::float64: {
    return DataType::float64;
  }
  case CSVType::boolean: {
    return DataType::boolean;
  }
  default: {
    throw std::runtime_error("nx::core::ConvertCSVTypeToDataType: Invalid CSVType");
  }
  }
}

/**
 * @brief Returns the CSVType associated with T.
 * @tparam T
 * @return
 */
template <class T>
constexpr CSVType GetCSVType() noexcept
{
  if constexpr(std::is_same_v<T, int8>)
  {
    return CSVType::int8;
  }
  else if constexpr(std::is_same_v<T, uint8>)
  {
    return CSVType::uint8;
  }
  else if constexpr(std::is_same_v<T, int16>)
  {
    return CSVType::int16;
  }
  else if constexpr(std::is_same_v<T, uint16>)
  {
    return CSVType::uint16;
  }
  else if constexpr(std::is_same_v<T, int32>)
  {
    return CSVType::int32;
  }
  else if constexpr(std::is_same_v<T, uint32>)
  {
    return CSVType::uint32;
  }
  else if constexpr(std::is_same_v<T, int64>)
  {
    return CSVType::int64;
  }
  else if constexpr(std::is_same_v<T, uint64>)
  {
    return CSVType::uint64;
  }
  else if constexpr(std::is_same_v<T, float32>)
  {
    return CSVType::float32;
  }
  else if constexpr(std::is_same_v<T, float64>)
  {
    return CSVType::float64;
  }
  else if constexpr(std::is_same_v<T, bool>)
  {
    return CSVType::boolean;
  }
  else if constexpr(std::is_same_v<T, std::string>)
  {
    return CSVType::string;
  }
  else
  {
    static_assert(dependent_false<T>, "nx::core::GetCSVType: Unsupported type");
  }
}

/**
 * @brief Returns a string representation of the passed in DataType
 * @param dataType
 * @return
 */
inline constexpr StringLiteral CSVTypeToString(CSVType dataType)
{
  switch(dataType)
  {
  case CSVType::int8: {
    return "int8";
  }
  case CSVType::uint8: {
    return "uint8";
  }
  case CSVType::int16: {
    return "int16";
  }
  case CSVType::uint16: {
    return "uint16";
  }
  case CSVType::int32: {
    return "int32";
  }
  case CSVType::uint32: {
    return "uint32";
  }
  case CSVType::int64: {
    return "int64";
  }
  case CSVType::uint64: {
    return "uint64";
  }
  case CSVType::float32: {
    return "float32";
  }
  case CSVType::float64: {
    return "float64";
  }
  case CSVType::boolean: {
    return "boolean";
  }
  case CSVType::string: {
    return "string";
  }
  default:
    throw std::runtime_error("nx::core::CSVTypeToString: Unknown CSVType");
  }
}

/**
 * @brief Returns a CSVType for the passed in string representation
 * @param csvTypeString
 * @return
 */
inline constexpr CSVType StringToCSVType(std::string_view csvTypeString)
{
  if(csvTypeString == CSVTypeToString(CSVType::int8).view())
  {
    return CSVType::int8;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::uint8).view())
  {
    return CSVType::uint8;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::int16).view())
  {
    return CSVType::int16;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::uint16).view())
  {
    return CSVType::uint16;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::int32).view())
  {
    return CSVType::int32;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::uint32).view())
  {
    return CSVType::uint32;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::int64).view())
  {
    return CSVType::int64;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::uint64).view())
  {
    return CSVType::uint64;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::float32).view())
  {
    return CSVType::float32;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::float64).view())
  {
    return CSVType::float64;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::boolean).view())
  {
    return CSVType::boolean;
  }
  else if(csvTypeString == CSVTypeToString(CSVType::string).view())
  {
    return CSVType::string;
  }
  else
  {
    throw std::runtime_error("nx::core::StringToCSVType: No known CSVType matches the given string value.");
  }
}

struct SIMPLNX_EXPORT ReadCSVData
{
public:
  enum class HeaderMode
  {
    LINE,
    CUSTOM
  };

  // Json Reader and Writer
  nlohmann::json writeJson() const;
  static Result<ReadCSVData> ReadJson(const nlohmann::json& json);

  std::string inputFilePath;
  std::vector<std::string> customHeaders;
  usize startImportRow = 1;
  std::vector<CSVType> dataTypes;
  std::vector<bool> skippedArrayMask;
  usize headersLine = 1;
  HeaderMode headerMode = HeaderMode::CUSTOM;
  std::vector<usize> tupleDims = {1};
  std::vector<char> delimiters = {};
  bool consecutiveDelimiters = false;
};
} // namespace nx::core
