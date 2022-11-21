#include "Core/Utilities/CommaSeparator.h"

#include "Core/Utilities/ICalculatorArray.h"
#include "Core/Utilities/UnaryOperator.h"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CommaSeparator::CommaSeparator()
{
  setInfixToken(",");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CommaSeparator::~CommaSeparator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorItem::ErrorCode CommaSeparator::checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& errMsg)
{
  // Make sure that this comma has a valid 2-argument unary operator before it
  bool foundUnaryOperator = false;
  for(int i = currentIndex - 1; i >= 0; i--)
  {
    CalculatorItem::Pointer item = infixVector[i];
    UnaryOperator::Pointer unary = std::dynamic_pointer_cast<UnaryOperator>(item);
    if(unary != UnaryOperator::NullPointer() && unary->getNumberOfArguments() == 2)
    {
      foundUnaryOperator = true;
    }
  }

  if(!foundUnaryOperator)
  {
    errMsg = "A comma in the expression does not have a corresponding operator preceding it.";
    return CalculatorItem::ErrorCode::NO_PRECEDING_UNARY_OPERATOR;
  }

  return CalculatorItem::ErrorCode::SUCCESS;
}

// -----------------------------------------------------------------------------
CommaSeparator::Pointer CommaSeparator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
