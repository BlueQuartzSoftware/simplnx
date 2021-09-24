#pragma once

#include "complex/Filter/ParameterTraits.hpp"
#include "complex/Filter/ValueParameter.hpp"
#include "complex/complex_export.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

namespace complex
{
class COMPLEX_EXPORT GeneratedFileListParameter : public ValueParameter
{
public:
  enum class Ordering : uint32_t
  {
    LowToHigh = 0,
    HighToLow = 1
  };
  /**
   * @brief This struct holds all of the data necessary to generate a list of file paths.
   */
  struct ValueType
  {
    int32_t startIndex = 0;
    int32_t endIndex = 0;
    int32_t incrementIndex = 1;
    uint32_t paddingDigits = 3;
    Ordering ordering = Ordering::LowToHigh;
    std::string inputPath;
    std::string filePrefix;
    std::string fileSuffix;
    std::string fileExtension;
  };

  GeneratedFileListParameter() = delete;
  /**
   *
   * @param name
   * @param humanName
   * @param helpText
   * @param defaultValue
   */
  GeneratedFileListParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue);

  ~GeneratedFileListParameter() override = default;

  GeneratedFileListParameter(const GeneratedFileListParameter&) = delete;
  GeneratedFileListParameter(GeneratedFileListParameter&&) noexcept = delete;

  GeneratedFileListParameter& operator=(const GeneratedFileListParameter&) = delete;
  GeneratedFileListParameter& operator=(GeneratedFileListParameter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] AcceptedTypes acceptedTypes() const override;

  /**
   * @brief
   * @param value
   */
  [[nodiscard]] nlohmann::json toJson(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Result<std::any> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] UniquePointer clone() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::any defaultValue() const override;

  /**
   * @brief
   * @param value
   * @return
   */
  [[nodiscard]] Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace complex

COMPLEX_DEF_PARAMETER_TRAITS(complex::GeneratedFileListParameter, "aac15aa6-b367-508e-bf73-94ab6be0058b");
