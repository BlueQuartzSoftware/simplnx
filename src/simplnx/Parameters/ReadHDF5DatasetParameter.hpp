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
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <list>
#include <memory>

namespace nx::core
{
class SIMPLNX_EXPORT ReadHDF5DatasetParameter : public ValueParameter
{
public:
  struct DatasetImportInfo
  {
    std::string dataSetPath;
    std::string componentDimensions;
    std::string tupleDimensions;

    static inline constexpr StringLiteral k_DatasetPath_Key = "dataset_path";
    static inline constexpr StringLiteral k_ComponentDimensions_Key = "component_dimensions";
    static inline constexpr StringLiteral k_TupleDimensions_Key = "tuple_dimensions";

    static Result<DatasetImportInfo> ReadJson(const nlohmann::json& json)
    {
      DatasetImportInfo data;
      if(!json.contains(k_DatasetPath_Key.view()))
      {
        return MakeErrorResult<DatasetImportInfo>(-500, fmt::format("ImportHDF5DatasetParameter ValueType: Cannot find the key \"{}\" in the ValueType json object.", k_DatasetPath_Key));
      }
      else if(!json[k_DatasetPath_Key.str()].is_string())
      {
        return MakeErrorResult<DatasetImportInfo>(-501,
                                                  fmt::format("ImportHDF5DatasetParameter ValueType: 'Dataset Path' value is of type {} and is not a string.", json[k_DatasetPath_Key].type_name()));
      }
      data.dataSetPath = json[k_DatasetPath_Key.str()];

      if(!json.contains(k_ComponentDimensions_Key.view()))
      {
        return MakeErrorResult<DatasetImportInfo>(-502, fmt::format("ImportHDF5DatasetParameter ValueType: Cannot find the key \"{}\" in the ValueType json object.", k_ComponentDimensions_Key));
      }
      else if(!json[k_ComponentDimensions_Key.str()].is_string())
      {
        return MakeErrorResult<DatasetImportInfo>(
            -503, fmt::format("ImportHDF5DatasetParameter ValueType: 'Component Dimensions' value is of type {} and is not a string.", json[k_ComponentDimensions_Key].type_name()));
      }
      data.componentDimensions = json[k_ComponentDimensions_Key.str()];

      if(!json.contains(k_TupleDimensions_Key.view()))
      {
        return MakeErrorResult<DatasetImportInfo>(-502, fmt::format("ImportHDF5DatasetParameter ValueType: Cannot find the key \"{}\" in the ValueType json object.", k_TupleDimensions_Key));
      }
      else if(!json[k_TupleDimensions_Key.str()].is_string())
      {
        return MakeErrorResult<DatasetImportInfo>(
            -503, fmt::format("ImportHDF5DatasetParameter ValueType: 'Tuple Dimensions' value is of type {} and is not a string.", json[k_TupleDimensions_Key].type_name()));
      }
      data.tupleDimensions = json[k_TupleDimensions_Key.str()];

      return {data};
    }

    nlohmann::json writeJson() const
    {
      nlohmann::json json;
      json[k_DatasetPath_Key.str()] = dataSetPath;
      json[k_ComponentDimensions_Key.str()] = componentDimensions;
      json[k_TupleDimensions_Key.str()] = tupleDimensions;
      return json;
    }
  };

  using InputFile = std::string;
  struct ValueType
  {
    std::optional<DataPath> parent;
    InputFile inputFile;
    std::list<DatasetImportInfo> datasets;
  };

  ReadHDF5DatasetParameter() = delete;
  ReadHDF5DatasetParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~ReadHDF5DatasetParameter() override = default;

  ReadHDF5DatasetParameter(const ReadHDF5DatasetParameter&) = delete;
  ReadHDF5DatasetParameter(ReadHDF5DatasetParameter&&) noexcept = delete;

  ReadHDF5DatasetParameter& operator=(const ReadHDF5DatasetParameter&) = delete;
  ReadHDF5DatasetParameter& operator=(ReadHDF5DatasetParameter&&) noexcept = delete;

  /**
   * @brief Returns the parameter's uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns a list of accpeted input types.
   * @return AcceptedTypes
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value
   * @return nlohmann::json
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @return Result<std::any>
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief Creates a copy of the parameter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns the user defined default value.
   * @return std::any
   */
  std::any defaultValue() const override;

  /**
   * @brief Validates the given value. Returns warnings/errors.
   * @param value
   * @return Result<>
   */
  Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT ImportHDF5DatasetFilterParameterConverter
{
  using ParameterType = ReadHDF5DatasetParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2, const nlohmann::json& json3);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::ReadHDF5DatasetParameter, "32e83e13-ee4c-494e-8bab-4e699df74a5a");
