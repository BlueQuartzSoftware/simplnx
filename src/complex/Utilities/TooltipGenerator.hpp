#pragma once

#include "complex/Utilities/TooltipRowItem.hpp"
#include "complex/complex_export.hpp"

#include <vector>

namespace complex
{

/**
 * @class TooltipGenerator
 * @brief The TooltipGenerator class creates a standardized HTML tooltip
 * using a simplified interface. Instead of hard-coding tables, the
 * TooltipGenerator creates tables using title, named value, and spacer rows.
 * HTML tables can be generated for a consistent appearance that can be easily
 * updated by changing how the HTML is generated in a single place without the
 * need to update HTML tables for every class that uses them.
 */
class COMPLEX_EXPORT TooltipGenerator
{
public:
  /**
   * @brief Creates an empty TooltipGenerator.
   */
  TooltipGenerator();

  /**
   * @brief Copy constructor
   * @param other
   */
  TooltipGenerator(const TooltipGenerator& other);

  /**
   * @brief Move constructor
   * @param other
   */
  TooltipGenerator(TooltipGenerator&& other) noexcept;

  ~TooltipGenerator();

  /**
   * @brief Adds a title-type row with the specified name to the end of the
   * TooltipGenerator.
   * @param title
   */
  void addTitle(const std::string& title);

  /**
   * @brief Adds a value-type row with the target name and value to the end
   * of the TooltipGenerator.
   * @param name
   * @param value
   */
  void addValue(const std::string& name, const std::string& value);

  /**
   * @brief Adds an empty spacer-type row to the end of the TooltipGenerator.
   */
  void addSpacer();

  /**
   * @brief Appends the provided TooltipGenerator's rows to the end of the
   * TooltipGenerator.
   * @param other
   */
  void append(const TooltipGenerator& other);

  /**
   * @brief Clears the TooltipGenerator.
   */
  void clear();

  /**
   * @brief Gets the current row color string.
   */
  std::string getRowColorStr() const;

  /**
   * @brief Sets the row color string.
   * @param colorStr
   */
  void setRowColorStr(const std::string& colorStr);

  /**
   * @brief Generates and returns the tooltip HTML.
   * @return std::string
   */
  std::string generateHTML() const;

protected:
  /**
   * @brief Converts the target TooltipRowItem to HTML.
   * @param row
   * @return std::string
   */
  std::string rowToHTML(const TooltipRowItem& row) const;

private:
  std::vector<TooltipRowItem> m_Rows;
  std::string m_ColorStr;
};
} // namespace complex
