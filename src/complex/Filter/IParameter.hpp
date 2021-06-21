#pragma once

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <typeinfo>

#include <nlohmann/json.hpp>

#include "complex/Filter/Arguments.hpp"
#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"

namespace complex
{
/**
 * @brief IParameter
 */
class COMPLEX_EXPORT IParameter
{
public:
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
  [[nodiscard]] virtual std::optional<std::any> defaultValue() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual Type type() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual const std::type_info& valueType() const = 0;

  /**
   * @brief
   * @param value
   */
  [[nodiscard]] virtual nlohmann::json toJson(const std::any& value) const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::any fromJson(const nlohmann::json& json) const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::unique_ptr<IParameter> clone() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::any construct(const Arguments& args) const;

protected:
  IParameter() = default;
};

class COMPLEX_EXPORT AbstractParameter : public IParameter
{
public:
  ~AbstractParameter() noexcept override = default;

  AbstractParameter(const AbstractParameter& other) = delete;
  AbstractParameter(AbstractParameter&& other) noexcept = delete;

  AbstractParameter& operator=(const AbstractParameter& other) = delete;
  AbstractParameter& operator=(AbstractParameter&&) noexcept = delete;

  [[nodiscard]] std::string name() const final;

  [[nodiscard]] std::string humanName() const final;

  [[nodiscard]] std::string helpText() const final;

  [[nodiscard]] std::optional<std::any> defaultValue() const final;

protected:
  AbstractParameter() = delete;

  AbstractParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const std::optional<std::any>& defaultValue = {});

private:
  std::string m_Name;
  std::string m_HumanName;
  std::string m_HelpText;
  std::optional<std::any> m_DefaultValue;
};

class COMPLEX_EXPORT ValueParameter : public AbstractParameter
{
public:
  ~ValueParameter() noexcept override = default;

  ValueParameter(const ValueParameter&) = delete;
  ValueParameter(ValueParameter&&) noexcept = delete;

  ValueParameter& operator=(const ValueParameter&) = delete;
  ValueParameter& operator=(ValueParameter&&) noexcept = delete;

  [[nodiscard]] Type type() const final;

  [[nodiscard]] virtual bool validate(const std::any& value) const = 0;

protected:
  ValueParameter() = delete;
  using AbstractParameter::AbstractParameter;
};

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

  [[nodiscard]] virtual bool validate(const DataStructure& dataStructure, const std::any& value) const = 0;

  [[nodiscard]] virtual Mutability mutability() const = 0;

protected:
  DataParameter() = delete;
  DataParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const std::optional<std::any>& default, Category category);

private:
  Category m_Category;
};

class COMPLEX_EXPORT ConstDataParameter : public DataParameter
{
public:
  ~ConstDataParameter() noexcept override = default;

  ConstDataParameter(const ConstDataParameter&) = delete;
  ConstDataParameter(ConstDataParameter&&) noexcept = delete;

  ConstDataParameter& operator=(const ConstDataParameter&) = delete;
  ConstDataParameter& operator=(ConstDataParameter&&) noexcept = delete;

  [[nodiscard]] Mutability mutability() const final;

  [[nodiscard]] virtual std::any resolve(const DataStructure& dataStructure, const std::any& value) const = 0;

protected:
  ConstDataParameter() = delete;
  using DataParameter::DataParameter;
};

class COMPLEX_EXPORT MutableDataParameter : public DataParameter
{
public:
  ~MutableDataParameter() noexcept override = default;

  MutableDataParameter(const MutableDataParameter&) = delete;
  MutableDataParameter(MutableDataParameter&&) noexcept = delete;

  MutableDataParameter& operator=(const MutableDataParameter&) = delete;
  MutableDataParameter& operator=(MutableDataParameter&&) noexcept = delete;

  [[nodiscard]] Mutability mutability() const final;

  [[nodiscard]] virtual std::any resolve(DataStructure& dataStructure, const std::any& value) const = 0;

protected:
  MutableDataParameter() = delete;
  using DataParameter::DataParameter;
};
} // namespace complex
