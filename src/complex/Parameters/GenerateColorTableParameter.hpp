#pragma once

#include <string>

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

#include "nlohmann/json.hpp"

namespace complex
{
/**
 * @brief This FilterParameter represents a color preset, and works specifically
 * with the GenerateColorTable filter. The data is held in an nlohmann::json object.
 */
class COMPLEX_EXPORT GenerateColorTableParameter : public ValueParameter
{
public:
  using ValueType = nlohmann::json;

  GenerateColorTableParameter() = delete;
  GenerateColorTableParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);
  ~GenerateColorTableParameter() override = default;

  GenerateColorTableParameter(const GenerateColorTableParameter&) = delete;
  GenerateColorTableParameter(GenerateColorTableParameter&&) noexcept = delete;

  GenerateColorTableParameter& operator=(const GenerateColorTableParameter&) = delete;
  GenerateColorTableParameter& operator=(GenerateColorTableParameter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  AcceptedTypes acceptedTypes() const override;

  /**
   * @brief
   * @param value
   */
  nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief
   * @return
   */
  std::any defaultValue() const override;

  /**
   * @brief
   * @return
   */
  ValueType defaultJson() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::GenerateColorTableParameter, "7b0e5b25-564e-4797-b154-4324ef276bf0");
