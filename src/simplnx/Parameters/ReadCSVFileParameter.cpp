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

#include "ReadCSVFileParameter.hpp"

#include "simplnx/Common/TypesUtility.hpp"

namespace nx::core
{
// -----------------------------------------------------------------------------
ReadCSVFileParameter::ReadCSVFileParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

// -----------------------------------------------------------------------------
Uuid ReadCSVFileParameter::uuid() const
{
  return ParameterTraits<ReadCSVFileParameter>::uuid;
}

// -----------------------------------------------------------------------------
IParameter::AcceptedTypes ReadCSVFileParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

// -----------------------------------------------------------------------------
nlohmann::json ReadCSVFileParameter::toJson(const std::any& value) const
{
  const auto& ReadCSVData = GetAnyRef<ValueType>(value);
  nlohmann::json json = ReadCSVData.writeJson();
  return json;
}

// -----------------------------------------------------------------------------
Result<std::any> ReadCSVFileParameter::fromJson(const nlohmann::json& json) const
{
  return {ConvertResultTo<std::any>(ReadCSVData::ReadJson(json))};
}

// -----------------------------------------------------------------------------
IParameter::UniquePointer ReadCSVFileParameter::clone() const
{
  return std::make_unique<ReadCSVFileParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

// -----------------------------------------------------------------------------
std::any ReadCSVFileParameter::defaultValue() const
{
  return m_DefaultValue;
}

// -----------------------------------------------------------------------------
Result<> ReadCSVFileParameter::validate(const std::any& value) const
{
  [[maybe_unused]] auto data = std::any_cast<ReadCSVData>(value);
  return {};
}

namespace SIMPLConversion
{
namespace
{
constexpr StringLiteral k_InputFilePathKey = "Wizard_InputFilePath";
constexpr StringLiteral k_DataHeadersKey = "Wizard_DataHeaders";
constexpr StringLiteral k_BeginIndexKey = "Wizard_BeginIndex";
constexpr StringLiteral k_NumberOfLinesKey = "Wizard_NumberOfLines";
constexpr StringLiteral k_DataTypesKey = "Wizard_DataTypes";
constexpr StringLiteral k_DelimitersKey = "Wizard_Delimiters";
constexpr StringLiteral k_HeaderLineKey = "Wizard_HeaderLine";
constexpr StringLiteral k_HeaderIsCustomKey = "Wizard_HeaderIsCustom";
constexpr StringLiteral k_HeaderUsesDefaultsKey = "Wizard_HeaderUseDefaults";
constexpr StringLiteral k_ConsecutiveDelimitersKey = "Wizard_ConsecutiveDelimiters";

std::vector<DataType> ConvertDataTypeStrings(const std::vector<std::string>& dataTypes)
{
  std::vector<DataType> output;

  for(usize i = 0; i < dataTypes.size(); i++)
  {
    try
    {
      output.push_back(nx::core::StringToDataType(dataTypes[i]));
    } catch(const std::exception& e)
    {
    }
  }

  return output;
}

std::vector<char> ConvertToChars(const std::string& string)
{
  return std::vector<char>(string.begin(), string.end());
}
} // namespace

Result<ReadASCIIWizardDataFilterParameterConverter::ValueType> ReadASCIIWizardDataFilterParameterConverter::convert(const nlohmann::json& json)
{
  std::vector<std::string> dataTypeStrings = json[k_DataTypesKey].get<std::vector<std::string>>();
  for(auto& dataTypeStr : dataTypeStrings)
  {
    if(dataTypeStr == "double")
    {
      dataTypeStr = "float64";
    }
    else if(dataTypeStr == "float")
    {
      dataTypeStr = "float32";
    }
  }

  ValueType value;
  value.inputFilePath = json[k_InputFilePathKey].get<std::string>();
  value.customHeaders = json[k_DataHeadersKey].get<std::vector<std::string>>();
  value.startImportRow = json[k_BeginIndexKey].get<int32>();
  value.dataTypes = ConvertDataTypeStrings(dataTypeStrings);
  value.delimiters = ConvertToChars(json[k_DelimitersKey].get<std::string>());
  value.headersLine = json[k_HeaderLineKey].get<int32>();

  bool headerIsCustom = json[k_HeaderIsCustomKey].get<bool>();
  bool headerUsesDefaults = json[k_HeaderUsesDefaultsKey].get<bool>();

  if(headerIsCustom)
  {
    value.headerMode = ValueType::HeaderMode::CUSTOM;
  }
  else
  {
    value.headerMode = ValueType::HeaderMode::LINE;
  }

  value.consecutiveDelimiters = static_cast<bool>(json[k_ConsecutiveDelimitersKey].get<int32>());

  return {std::move(value)};
}
} // namespace SIMPLConversion
} // namespace nx::core
