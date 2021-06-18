#pragma once

#include <string>

#include "complex/complex_export.hpp"

namespace complex
{

/**
 * class TooltipRowItem
 *
 */

class COMPLEX_EXPORT TooltipRowItem
{
public:
  enum class TooltipRowType
  {
    Title,
    Value,
    Spacer
  };

  /**
   * @brief Constructs a spacer row without a title or value.
   */
  TooltipRowItem();

  /**
   * @brief Constructs a Title row with the specified string
   * @param title
   */
  TooltipRowItem(const std::string& title);

  /**
   * @brief Constructs a Value row with the specified name and value.
   * @param title
   * @param value
   */
  TooltipRowItem(const std::string& title, const std::string& value);

  /**
   * @brief Copy constructor
   * @param other
   */
  TooltipRowItem(const TooltipRowItem& other);

  /**
   * @brief Move constructor
   * @param other
   */
  TooltipRowItem(TooltipRowItem&& other) noexcept;

  virtual ~TooltipRowItem();

  /**
   * @brief Gets the row's TooltipRowType.
   * @return complex::TooltipRowType
   */
  TooltipRowType getType() const;

  /**
   * @brief Returns the row's name. For Title rows, this returns the title name.
   * For Value rows, this returns the value name.
   * @return std::string
   */
  std::string getTitle() const;

  /**
   * @brief Returns the row value. This returns an empty string for Title rows.
   * @return std::string
   */
  std::string getValue() const;

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return TooltipRowItem&
   */
  TooltipRowItem& operator=(const TooltipRowItem& rhs);

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return TooltipRowItem&
   */
  TooltipRowItem& operator=(TooltipRowItem&& rhs) noexcept;

  /**
   * @brief Equality operator
   * @param rhs
   * @return bool
   */
  bool operator==(const TooltipRowItem& rhs) const;

  /**
   * @brief Inequality operator
   * @param rhs
   * @return bool
   */
  bool operator!=(const TooltipRowItem& rhs) const;

protected:
private:
  TooltipRowType m_Type = TooltipRowType::Spacer;
  std::string m_Title = "";
  std::string m_Value = "";
};
} // namespace complex
