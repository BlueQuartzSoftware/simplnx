#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/AbstractParameter.hpp"

namespace nx::core
{
/**
 * @brief DataParameter provides an interface for parameters that need access to DataStructure.
 */
class SIMPLNX_EXPORT DataParameter : public AbstractParameter
{
public:
  enum class Category : uint8
  {
    Required = 0,
    Created
  };

  enum class Mutability : uint8
  {
    Const = 0,
    Mutable
  };

  ~DataParameter() noexcept override = default;

  DataParameter(const DataParameter&) = delete;
  DataParameter(DataParameter&&) noexcept = delete;

  DataParameter& operator=(const DataParameter&) = delete;
  DataParameter& operator=(DataParameter&&) noexcept = delete;

  /**
   * @brief Returns whether the parameter is a ValueParameter or DataParameter.
   * @return
   */
  Type type() const final;

  /**
   * @brief Returns whether the parameter refers to a required or created DataObject.
   * @return
   */
  Category category() const;

  /**
   * @brief Returns whether the parameter needs const or non-const access to the DataStructure.
   * @return
   */
  virtual Mutability mutability() const = 0;

  /**
   * @brief Validates the given value against the given DataStructure. Returns warnings/errors.
   * @param dataStructure The active DataStructure to use during validation
   * @param value The value to validate
   * @return
   */
  virtual Result<> validate(const DataStructure& dataStructure, const std::any& value) const = 0;

protected:
  DataParameter() = delete;
  DataParameter(const std::string& name, const std::string& humanName, const std::string& helpText, Category category);

private:
  Category m_Category;
};
} // namespace nx::core
