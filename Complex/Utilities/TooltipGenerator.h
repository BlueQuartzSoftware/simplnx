#pragma once

#include <vector>

#include "Complex/Utilities/TooltipRowItem.h"

namespace Complex
{

/**
 * class TooltipGenerator
 *
 */

class TooltipGenerator
{
public:
  /**
   * @brief Creates an empty TooltipGenerator.
   */
  TooltipGenerator();

  virtual ~TooltipGenerator();

  /**
   * @brief Adds a title-type row with the specified name to the end of the TooltipGenerator.
   * @param title
   */
  void addTitle(const std::string& title);

  /**
   * @brief Adds a value-type row with the target name and value to the end of the TooltipGenerator.
   * @param name
   * @param value
   */
  void addValue(const std::string& name, const std::string& value);

  /**
   * @brief Adds an empty spacer-type row to the end of the TooltipGenerator.
   */
  void addSpacer();

  /**
   * @brief Appends the provided TooltipGenerator's rows to the end of the TooltipGenerator.
   * @param ToolTipGenerator
   */
  void append(const TooltipGenerator& ToolTipGenerator);

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
  static std::string rowToHTML(const TooltipRowItem& row);

private:
  std::vector<TooltipRowItem> m_Rows;
};
} // namespace Complex
