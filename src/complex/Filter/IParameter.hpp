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
/**
 * @brief IParameter
 */
class COMPLEX_EXPORT IParameter
{
public:
  using AcceptedTypes = std::vector<std::type_index>;
  using UniquePointer = std::unique_ptr<IParameter>;

  enum class Type : u8
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
   * @brief
   * @return
   */
  [[nodiscard]] virtual Uuid uuid() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::string name() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::string humanName() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::string helpText() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::any defaultValue() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual Type type() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual AcceptedTypes acceptedTypes() const = 0;

  /**
   * @brief
   * @param value
   */
  [[nodiscard]] virtual nlohmann::json toJson(const std::any& value) const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual Result<std::any> fromJson(const nlohmann::json& json) const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual UniquePointer clone() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::any construct(const Arguments& args) const;

protected:
  IParameter() = default;
};
} // namespace complex
