#include "Core/Utilities/RightParenthesisItem.h"

#include "Core/Utilities/LeftParenthesisItem.h"

using namespace complex;

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
        return RightParenthesisItem::ErrorCode::SUCCESS;
      }
    }
  }

  return RightParenthesisItem::ErrorCode::MISMATCHED_PARENTHESES;
}

// -----------------------------------------------------------------------------
RightParenthesisItem::Pointer RightParenthesisItem::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
