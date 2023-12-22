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

#include "ReadHDF5DatasetParameter.hpp"

using namespace nx::core;
namespace
{
constexpr StringLiteral k_DataPathsKey = "data_paths";
constexpr StringLiteral k_InputFileKey = "input_file";
constexpr StringLiteral k_ParentGroupKey = "parent_group";
} // namespace

namespace nx::core
{
// -----------------------------------------------------------------------------
ReadHDF5DatasetParameter::ReadHDF5DatasetParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

// -----------------------------------------------------------------------------
Uuid ReadHDF5DatasetParameter::uuid() const
{
  return ParameterTraits<ReadHDF5DatasetParameter>::uuid;
}

// -----------------------------------------------------------------------------
IParameter::AcceptedTypes ReadHDF5DatasetParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

// -----------------------------------------------------------------------------
nlohmann::json ReadHDF5DatasetParameter::toJson(const std::any& value) const
{
  const auto& datasetImportInfo = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  nlohmann::json dataPathsJson = nlohmann::json::array();
  for(const auto& importInfo : datasetImportInfo.datasets)
  {
    dataPathsJson.push_back(importInfo.writeJson());
  }
  json[k_InputFileKey.str()] = datasetImportInfo.inputFile;
  if(datasetImportInfo.parent.has_value())
  {
    json[k_ParentGroupKey.str()] = datasetImportInfo.parent.value().toString();
  }
  json[k_DataPathsKey.str()] = std::move(dataPathsJson);
  return json;
}

// -----------------------------------------------------------------------------
Result<std::any> ReadHDF5DatasetParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'ImportHDF5DatasetParameter' JSON Error: ";
  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(-780, fmt::format("{}JSON value for key '{}' is not an object", prefix, name()));
  }

  if(!json.contains(k_DataPathsKey.view()))
  {
    return MakeErrorResult<std::any>(-781, fmt::format("{}JSON does not contain key '{} / {}'", prefix, name(), k_DataPathsKey.view()));
  }

  if(!json.contains(k_InputFileKey.view()))
  {
    return MakeErrorResult<std::any>(-782, fmt::format("{}JSON does not contain key '{} / {}'", prefix, name(), k_InputFileKey.view()));
  }

  ValueType importData;
  importData.inputFile = json[k_InputFileKey.str()];
  if(json.contains(k_ParentGroupKey.view()))
  {
    importData.parent = DataPath::FromString(json[k_ParentGroupKey.str()].get<std::string>());
  }
  const auto& jsonDataPaths = json[k_DataPathsKey.str()];
  if(!jsonDataPaths.is_null())
  {
    if(!jsonDataPaths.is_array())
    {
      return MakeErrorResult<std::any>(-783, fmt::format("{}JSON value for key '{} / {}' is not an array", prefix, name()));
    }
    std::vector<Error> errors;
    for(const auto& jsonImportInfo : jsonDataPaths)
    {
      auto importInfo = DatasetImportInfo::ReadJson(jsonImportInfo);
      if(importInfo.invalid())
      {
        return {{nonstd::make_unexpected(std::move(importInfo.errors()))}};
      }
      importData.datasets.push_back(std::move(importInfo.value()));
    }
  }

  return {{std::move(importData)}};
}

// -----------------------------------------------------------------------------
IParameter::UniquePointer ReadHDF5DatasetParameter::clone() const
{
  return std::make_unique<ReadHDF5DatasetParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

// -----------------------------------------------------------------------------
std::any ReadHDF5DatasetParameter::defaultValue() const
{
  return m_DefaultValue;
}

// -----------------------------------------------------------------------------
Result<> ReadHDF5DatasetParameter::validate(const std::any& value) const
{
  [[maybe_unused]] auto data = std::any_cast<ValueType>(value);
  return {};
}
} // namespace nx::core
