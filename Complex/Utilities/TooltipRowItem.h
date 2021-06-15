#pragma once

#include <string>

namespace Complex
{

/**
 * class TooltipRowItem
 *
 */

class TooltipRowItem
{
public:
  enum class TooltipRowType
  {
    Title,
    Value,
    Spacer
  };

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
   * @return Complex::TooltipRowType
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

protected:
private:
  TooltipRowType Type = TooltipRowType::Spacer;
  std::string Title = "";
  std::string Value = "";
};
} // namespace Complex
