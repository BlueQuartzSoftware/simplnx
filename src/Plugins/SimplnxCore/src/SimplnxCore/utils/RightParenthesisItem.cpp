#include "SimplnxCore/utils/RightParenthesisItem.hpp"
#include "SimplnxCore/utils/LeftParenthesisItem.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
RightParenthesisItem::RightParenthesisItem()
{
  setInfixToken(")");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
RightParenthesisItem::~RightParenthesisItem() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorItem::ErrorCode RightParenthesisItem::checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& errMsg)
{
  // Check for an opening parenthesis, ignoring any other internal sets of parentheses
  int rightParenthesisCount = 0;
  for(int i = currentIndex - 1; i >= 0; i--)
  {
    CalculatorItem::Pointer item = infixVector[i];
    if(std::dynamic_pointer_cast<RightParenthesisItem>(item) != RightParenthesisItem::NullPointer())
    {
      rightParenthesisCount++;
    }
    else if(std::dynamic_pointer_cast<LeftParenthesisItem>(item) != LeftParenthesisItem::NullPointer())
    {
      if(rightParenthesisCount > 0)
      {
        rightParenthesisCount--;
      }
      else
      {
        return RightParenthesisItem::ErrorCode::Success;
      }
    }
  }

  return RightParenthesisItem::ErrorCode::MismatchedParentheses;
}

// -----------------------------------------------------------------------------
RightParenthesisItem::Pointer RightParenthesisItem::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
