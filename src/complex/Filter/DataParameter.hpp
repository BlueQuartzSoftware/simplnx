#pragma once

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/AbstractParameter.hpp"

namespace complex
{
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

  [[nodiscard]] Type type() const final;

  [[nodiscard]] Category category() const;

  [[nodiscard]] virtual Result<> validate(const DataStructure& dataStructure, const std::any& value) const = 0;

  [[nodiscard]] virtual Mutability mutability() const = 0;

protected:
  DataParameter() = delete;
  DataParameter(const std::string& name, const std::string& humanName, const std::string& helpText, Category category);

private:
  Category m_Category;
};
} // namespace complex
