#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/CalculatorItem.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT RightParenthesisItem : public CalculatorItem
{
public:
  using Self = RightParenthesisItem;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new RightParenthesisItem());
  }

  ~RightParenthesisItem() override;

  /**
   * @brief checkValidity
   * @param infixVector
   * @param currentIndex
   * @param msg
   * @return
   */
  CalculatorItem::ErrorCode checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& msg) override;

protected:
  RightParenthesisItem();

public:
  RightParenthesisItem(const RightParenthesisItem&) = delete;            // Copy Constructor Not Implemented
  RightParenthesisItem(RightParenthesisItem&&) = delete;                 // Move Constructor Not Implemented
  RightParenthesisItem& operator=(const RightParenthesisItem&) = delete; // Copy Assignment Not Implemented
  RightParenthesisItem& operator=(RightParenthesisItem&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace nx::core
