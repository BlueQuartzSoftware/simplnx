#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <list>
#include <memory>

namespace nx::core
{
class SIMPLNX_EXPORT ReadHDF5DataStackParameter : public ValueParameter
{
public:
  struct SIMPLNX_EXPORT ValueType
  {
    std::string inputPath;
    std::string pathPrefix;
    std::string pathSuffix;
    uint8 paddingDigits;
    uint32 startIndex;
    uint32 endIndex;

    static inline constexpr StringLiteral k_FilePath_Key = "file_path";
    static inline constexpr StringLiteral k_PathPrefix_Key = "path_prefix";
    static inline constexpr StringLiteral k_PathSuffix_Key = "path_suffix";
    static inline constexpr StringLiteral k_PaddingDigits_Key = "padding_digits";
    static inline constexpr StringLiteral k_StartIndex_Key = "start_index";
    static inline constexpr StringLiteral k_EndIndex_Key = "end_index";
  };

  ReadHDF5DataStackParameter() = delete;
  ReadHDF5DataStackParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~ReadHDF5DataStackParameter() override = default;

  ReadHDF5DataStackParameter(const ReadHDF5DataStackParameter&) = delete;
  ReadHDF5DataStackParameter(ReadHDF5DataStackParameter&&) noexcept = delete;

  ReadHDF5DataStackParameter& operator=(const ReadHDF5DataStackParameter&) = delete;
  ReadHDF5DataStackParameter& operator=(ReadHDF5DataStackParameter&&) noexcept = delete;

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
   * @brief Returns version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the json format changes.
   * @return uint64
   */
  VersionType getVersion() const override;

  /**
   * @brief Constructs an input value from the given arguments.
   * By default, accesses a singular value by key and returns that.
   * May be overriden by subclasses that depend on other parameters.
   * @param args
   * @param executionContext
   * @return
   */
  std::any construct(const Arguments& args, const ExecutionContext& executionContext) const override;

  /**
   * @brief Validates the given value. Returns warnings/errors.
   * @param value
   * @return Result<>
   */
  Result<> validate(const std::any& value) const override;

protected:
  /**
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value
   * @return nlohmann::json
   */
  nlohmann::json toJsonImpl(const std::any& value) const override;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @return Result<std::any>
   */
  Result<std::any> fromJsonImpl(const nlohmann::json& json, VersionType version) const override;

private:
  ValueType m_DefaultValue = {};
};

namespace SIMPLConversion
{
struct SIMPLNX_EXPORT ReadHDF5DataStackFilterParameterConverter
{
  using ParameterType = ReadHDF5DataStackParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2, const nlohmann::json& json3);
};
} // namespace SIMPLConversion
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::ReadHDF5DataStackParameter, "fb56a7a3-b8cf-48a3-92f7-8aa892741c25");
