#include "SimplnxCore/utils/LeftParenthesisItem.hpp"
#include "SimplnxCore/utils/RightParenthesisItem.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
LeftParenthesisItem::LeftParenthesisItem()
{
  setInfixToken("(");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
LeftParenthesisItem::~LeftParenthesisItem() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorItem::ErrorCode LeftParenthesisItem::checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& errMsg)
{
  // Check for a closing parenthesis, ignoring any other internal sets of parentheses
  int leftParenthesisCount = 0;
  for(int i = currentIndex + 1; i < infixVector.size(); i++)
  {
    CalculatorItem::Pointer item = infixVector[i];
    if(std::dynamic_pointer_cast<LeftParenthesisItem>(item) != LeftParenthesisItem::NullPointer())
    {
      leftParenthesisCount++;
    }
    else if(std::dynamic_pointer_cast<RightParenthesisItem>(item) != RightParenthesisItem::NullPointer())
    {
      if(leftParenthesisCount > 0)
      {
        leftParenthesisCount--;
      }
      else
      {
        return LeftParenthesisItem::ErrorCode::Success;
      }
    }
  }

  return LeftParenthesisItem::ErrorCode::MismatchedParentheses;
}

// -----------------------------------------------------------------------------
LeftParenthesisItem::Pointer LeftParenthesisItem::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
