#pragma once

#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{

/**
 * @class TooltipRowItem
 * @brief The TooltipRowItem class is a utility class for use in a
 * TooltipGenerator. Each instance of TooltipRowItem stores information
 * required to generate a single table row. There are three types of rows
 * that can be generated: Title, Value, and Spacer.
 * Title rows contain a single string serving as a centered title value and is
 * formatted accordingly.
 * Value rows contain two strings, a name and a value. Both strings are
 * formatted according to their purpose and are left-aligned into their own
 * columns.
 * Spacer rows do not contain any values but serve as a blank row for
 * formatting purposes.
 */
class SIMPLNX_EXPORT TooltipRowItem
{
public:
  enum class Type
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

  ~TooltipRowItem();

  /**
   * @brief Gets the row's Type.
   * @return nx::core::Type
   */
  Type getType() const;

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
  Type m_Type = Type::Spacer;
  std::string m_Title = "";
  std::string m_Value = "";
};
} // namespace nx::core
