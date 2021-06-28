#pragma once

#include <vector>

#include "complex/Utilities/TooltipRowItem.hpp"

namespace complex
{

/**
 * class TooltipGenerator
 *
 */

class TooltipGenerator
{
public:
  /**
   * Empty Constructor
   */
  TooltipGenerator();

  /**
   * Empty Destructor
   */
  virtual ~TooltipGenerator();

  /**
   * @brief
   * @param title
   */
  void addTitle(const std::string& title);

  /**
   * @brief
   * @param name
   * @param value
   */
  void addValue(const std::string& name, const std::string& value);

  /**
   * @brief
   */
  void addSpacer();

  /**
   * @brief
   * @param ToolTipGenerator
   */
  void append(const TooltipGenerator& ToolTipGenerator);

  /**
   * @brief
   */
  void clear();

  /**
   * @brief
   */
  std::string getRowColorStr() const;

  /**
   * @brief
   * @param colorStr
   */
  void setRowColorStr(const std::string& colorStr);

  /**
   * @brief
   * @return std::string
   */
  std::string generateHTML() const;

protected:
  /**
   * @param row
   * @return std::string
   */
  static std::string rowToHTML(const TooltipRowItem& row);

private:
  std::vector<TooltipRowItem> m_Rows;
};
} // namespace complex
