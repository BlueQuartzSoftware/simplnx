#pragma once

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "complex/Common/Result.hpp"
#include "complex/Common/Types.hpp"
#include "complex/Common/Uuid.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/complex_export.hpp"

namespace complex
{

namespace FilterParameter::Constants
{
/**
 * @brief These constants can be used as standard error codes when parsing JSON
 */
constexpr const int32_t k_Json_Missing_Entry = -100;
constexpr const int32_t k_Json_Value_Not_Object = -101;
constexpr const int32_t k_Json_Value_Not_String = -102;
constexpr const int32_t k_Json_Value_Not_Integer = -103;
constexpr const int32_t k_Json_Value_Not_Unsigned = -104;
constexpr const int32_t k_Json_Value_Not_Enumeration = -105;

} // namespace FilterParameter::Constants

/**
 * @brief IParameter provides an interface for filter parameters including metadata (e.g. name, uuid, etc.)
 * and how it interacts with its inputs. May be either a ValueParameter or a DataParameter.
 */
class COMPLEX_EXPORT IParameter
{
public:
  using AcceptedTypes = std::vector<std::type_index>;
  using UniquePointer = std::unique_ptr<IParameter>;

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
   * @return
   */
  [[nodiscard]] virtual Uuid uuid() const = 0;

  /**
   * @brief Returns the user defined name.
   * @return
   */
  [[nodiscard]] virtual std::string name() const = 0;

  /**
   * @brief Returns the user defined human readable name.
   * @return
   */
  [[nodiscard]] virtual std::string humanName() const = 0;

  /**
   * @brief Returns the user defined help text.
   * @return
   */
  [[nodiscard]] virtual std::string helpText() const = 0;

  /**
   * @brief Returns the user defined default value.
   * @return
   */
  [[nodiscard]] virtual std::any defaultValue() const = 0;

  /**
   * @brief Returns whether the parameter is a ValueParameter or DataParameter.
   * @return
   */
  [[nodiscard]] virtual Type type() const = 0;

  /**
   * @brief Returns a list of accpeted input types.
   * @return
   */
  [[nodiscard]] virtual AcceptedTypes acceptedTypes() const = 0;

  /**
   * @brief Converts the given value to JSON.
   * Throws if value is not an accepted type.
   * @param value
   */
  [[nodiscard]] virtual nlohmann::json toJson(const std::any& value) const = 0;

  /**
   * @brief Converts the given JSON to a std::any containing the appropriate input type.
   * Returns any warnings/errors.
   * @return
   */
  [[nodiscard]] virtual Result<std::any> fromJson(const nlohmann::json& json) const = 0;

  /**
   * @brief Creates a copy of the parameter.
   * @return
   */
  [[nodiscard]] virtual UniquePointer clone() const = 0;

  /**
   * @brief Constructs an input value from the given arguments.
   * By default, accesses a singular value by key and returns that.
   * May be overriden by subclasses that depend on other parameters.
   * @return
   */
  [[nodiscard]] virtual std::any construct(const Arguments& args) const;

protected:
  IParameter() = default;
};
} // namespace complex
