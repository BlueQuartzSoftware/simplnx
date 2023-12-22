#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/CalculatorItem.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT LeftParenthesisItem : public CalculatorItem
{
public:
  using Self = LeftParenthesisItem;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new LeftParenthesisItem());
  }

  ~LeftParenthesisItem() override;

  /**
   * @brief checkValidity
   * @param infixVector
   * @param currentIndex
   * @param msg
   * @return
   */
  CalculatorItem::ErrorCode checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& msg) override;

protected:
  LeftParenthesisItem();

public:
  LeftParenthesisItem(const LeftParenthesisItem&) = delete;            // Copy Constructor Not Implemented
  LeftParenthesisItem(LeftParenthesisItem&&) = delete;                 // Move Constructor Not Implemented
  LeftParenthesisItem& operator=(const LeftParenthesisItem&) = delete; // Copy Assignment Not Implemented
  LeftParenthesisItem& operator=(LeftParenthesisItem&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace nx::core
