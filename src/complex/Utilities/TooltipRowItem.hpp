#pragma once

#include <string>

namespace complex
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
   * @param  title
   */
  TooltipRowItem(const std::string& title);

  /**
   * @brief
   * @param title
   * @param value
   */
  TooltipRowItem(const std::string& title, const std::string& value);

  /**
   * @brief
   * @param other
   */
  TooltipRowItem(const TooltipRowItem& other);

  /**
   * @brief
   * @param other
   */
  TooltipRowItem(TooltipRowItem&& other) noexcept;

  virtual ~TooltipRowItem();

  /**
   * @brief
   * @return SIMPL::TooltipRowType
   */
  TooltipRowType getType() const;

  /**
   * @brief
   * @return std::string
   */
  std::string getTitle() const;

  /**
   * @brief
   * @return std::string
   */
  std::string getValue() const;

protected:
private:
  TooltipRowType Type = TooltipRowType::Spacer;
  std::string Title = "";
  std::string Value = "";
};
} // namespace complex
