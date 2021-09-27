#pragma once

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/AbstractParameter.hpp"

namespace complex
{
/**
 * @brief DataParameter provides an interface for parameters that need access to DataStructure.
 */
class COMPLEX_EXPORT DataParameter : public AbstractParameter
{
public:
  enum class Category : u8
  {
    Required = 0,
    Created
  };

  enum class Mutability : u8
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
  [[nodiscard]] Type type() const final;

  /**
   * @brief Returns whether the parameter refers to a required or created DataObject.
   * @return
   */
  [[nodiscard]] Category category() const;

  /**
   * @brief Returns whether the parameter needs const or non-const access to the DataStructure.
   * @return
   */
  [[nodiscard]] virtual Mutability mutability() const = 0;

  /**
   * @brief Validates the given value against the given DataStructure. Returns warnings/errors.
   * @param dataStructure
   * @param value
   * @return
   */
  [[nodiscard]] virtual Result<> validate(const DataStructure& dataStructure, const std::any& value) const = 0;

protected:
  DataParameter() = delete;
  DataParameter(const std::string& name, const std::string& humanName, const std::string& helpText, Category category);

private:
  Category m_Category;
};
} // namespace complex
