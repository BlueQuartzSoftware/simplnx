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
   * @return Type The parameter type
   */
  Type type() const final;

  /**
   * @brief Returns whether the parameter refers to a required or created DataObject.
   * @return Category The parameter category
   */
  Category category() const;

  /**
   * @brief Returns whether the parameter needs const or non-const access to the DataStructure.
   * @return Mutability The mutability requirement
   */
  virtual Mutability mutability() const = 0;

  /**
   * @brief Validates the given value against the given DataStructure. Returns warnings/errors.
   * @param dataStructure The active DataStructure to use during validation
   * @param value The value to validate
   * @return Result<> Result with any errors or warnings
   */
  virtual Result<> validate(const DataStructure& dataStructure, const std::any& value) const = 0;

protected:
  DataParameter() = delete;

  /**
   * @brief Protected constructor for creating a DataParameter.
   * @param name The parameter name
   * @param humanName The human-readable name
   * @param helpText The help text
   * @param category The parameter category
   */
  DataParameter(const std::string& name, const std::string& humanName, const std::string& helpText, Category category);

private:
  Category m_Category;
};
} // namespace nx::core
