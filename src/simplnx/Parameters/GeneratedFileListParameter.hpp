#pragma once

#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/Utilities/FilePathGenerator.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{
class SIMPLNX_EXPORT GeneratedFileListParameter : public ValueParameter
{
public:
  using Ordering = FilePathGenerator::Ordering;

  /**
   * @brief This struct holds all of the data necessary to generate a list of file paths.
   */
  struct ValueType
  {
    int32 startIndex = 0;
    int32 endIndex = 0;
    int32 incrementIndex = 1;
    uint32 paddingDigits = 3;
    Ordering ordering = Ordering::LowToHigh;
    std::string inputPath;
    std::string filePrefix;
    std::string fileSuffix;
    std::string fileExtension;

    std::vector<std::string> generate() const
    {
      return FilePathGenerator::GenerateFileList(startIndex, endIndex, incrementIndex, ordering, inputPath, filePrefix, fileSuffix, fileExtension, paddingDigits);
    }
    std::pair<std::vector<std::string>, bool> generateAndValidate(bool failFast) const
    {
      return FilePathGenerator::GenerateAndValidateFileList(startIndex, endIndex, incrementIndex, ordering, inputPath, filePrefix, fileSuffix, fileExtension, paddingDigits, failFast);
    }
  };

  GeneratedFileListParameter() = delete;

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
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::GeneratedFileListParameter, "aac15aa6-b367-508e-bf73-94ab6be0058b");
