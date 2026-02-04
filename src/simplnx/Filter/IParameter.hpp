#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Filter/AnyCloneable.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/ExecutionContext.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json_fwd.hpp>

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace nx::core
{
/**
 * @brief These constants can be used as standard error codes when parsing JSON
 */
namespace FilterParameter::Constants
{
inline constexpr int32 k_Json_Missing_Entry = -100;
inline constexpr int32 k_Json_Value_Not_Object = -101;
inline constexpr int32 k_Json_Value_Not_String = -102;
inline constexpr int32 k_Json_Value_Not_Integer = -103;
inline constexpr int32 k_Json_Value_Not_Unsigned = -104;
inline constexpr int32 k_Json_Value_Not_Enumeration = -105;
inline constexpr int32 k_Json_Value_Not_Value_Type = -106;

inline constexpr int32 k_Validate_Empty_Value = -201;
inline constexpr int32 k_Validate_ExistingValue = -202;
inline constexpr int32 k_Validate_DuplicateValue = -203;
inline constexpr int32 k_Validate_Does_Not_Exist = -204;
inline constexpr int32 k_Validate_Type_Error = -205;
inline constexpr int32 k_Validate_OutOfRange_Error = -206;
inline constexpr int32 k_Validate_AllowedType_Error = -207;
inline constexpr int32 k_Validate_TupleShapeValue = -208;
inline constexpr int32 k_Validate_DataLocation_Error = -209;
inline constexpr int32 k_Validate_InvalidDataObjectName = -210;
} // namespace FilterParameter::Constants

/**
 * @brief IParameter provides an interface for filter parameters including metadata (e.g. name, uuid, etc.)
 * and how it interacts with its inputs. May be either a ValueParameter or a DataParameter.
 */
class SIMPLNX_EXPORT IParameter
{
public:
  using AcceptedTypes = std::vector<std::type_index>;
  using UniquePointer = std::unique_ptr<IParameter>;
  using VersionType = uint64;

  enum class Type : uint8
  {
    Value = 0,
    Data
  };

  virtual ~IParameter() noexcept = default;

  IParameter(const IParameter&) = delete;
  IParameter(IParameter&&) noexcept = delete;

  IParameter& operator=(const IParameter&) = delete;
  IParameter& operator=(IParameter&&) noexcept = delete;

  /**
   * @brief Returns the parameter's uuid.
   * @return Uuid The parameter's unique identifier
   */
  virtual Uuid uuid() const = 0;

  /**
   * @brief Returns the user defined name.
   * @return std::string The parameter name
   */
  virtual std::string name() const = 0;

  /**
   * @brief Returns the user defined human readable name.
   * @return std::string The human-readable parameter name
   */
  virtual std::string humanName() const = 0;

  /**
   * @brief Returns the user defined help text.
   * @return std::string The help text describing this parameter
   */
  virtual std::string helpText() const = 0;

  /**
   * @brief Returns the user defined default value.
   * @return std::any The default value for this parameter
   */
  virtual std::any defaultValue() const = 0;

  /**
   * @brief Returns whether the parameter is a ValueParameter or DataParameter.
   * @return Type The parameter type enumeration
   */
  virtual Type type() const = 0;

  /**
   * @brief Returns a list of accepted input types.
   * @return AcceptedTypes Vector of accepted type indices
   */
  virtual AcceptedTypes acceptedTypes() const = 0;

  /**
   * @brief Returns version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the json format changes.
   * @return VersionType The version number
   */
  virtual VersionType getVersion() const = 0;

  /**
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value The value to convert
   * @return nlohmann::json The JSON representation
   */
  nlohmann::json toJson(const std::any& value) const;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @param json The JSON to convert
   * @return Result<std::any> The converted value or errors
   */
  Result<std::any> fromJson(const nlohmann::json& json) const;

  /**
   * @brief Creates a copy of the parameter.
   * @return UniquePointer A unique pointer to the cloned parameter
   */
  virtual UniquePointer clone() const = 0;

  /**
   * @brief Constructs an input value from the given arguments.
   * By default, accesses a singular value by key and returns that.
   * May be overriden by subclasses that depend on other parameters.
   * @param args The arguments to construct from
   * @param executionContext The execution context
   * @return std::any The constructed value
   */
  virtual std::any construct(const Arguments& args, const ExecutionContext& executionContext) const;

protected:
  /**
   * @brief Protected default constructor.
   */
  IParameter() = default;

  /**
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value The value to convert
   * @return nlohmann::json The JSON representation
   */
  virtual nlohmann::json toJsonImpl(const std::any& value) const = 0;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @param json The JSON to convert
   * @param version The parameter version to use for conversion
   * @return Result<std::any> The converted value or errors
   */
  virtual Result<std::any> fromJsonImpl(const nlohmann::json& json, uint64 version) const = 0;
};

using AnyParameter = AnyCloneable<IParameter>;
} // namespace nx::core
