#include "SimplnxCore/utils/BinaryOperator.hpp"
#include "SimplnxCore/utils/LeftParenthesisItem.hpp"
#include "SimplnxCore/utils/RightParenthesisItem.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
BinaryOperator::BinaryOperator()
{
  setOperatorType(Binary);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
BinaryOperator::~BinaryOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void BinaryOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  // This should never be executed
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorItem::ErrorCode BinaryOperator::checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& errMsg)
{
  int leftValue = currentIndex - 1;
  int rightValue = currentIndex + 1;

  // Check that there is a valid value to the left of the operator
  if(leftValue < 0 ||
     (nullptr != std::dynamic_pointer_cast<CalculatorOperator>(infixVector[leftValue]) && std::dynamic_pointer_cast<CalculatorOperator>(infixVector[leftValue])->getOperatorType() == Binary) ||
     nullptr != std::dynamic_pointer_cast<LeftParenthesisItem>(infixVector[leftValue]))
  {
    errMsg = fmt::format("The operator '{}' does not have a valid 'left' value.", getInfixToken());
    return CalculatorItem::ErrorCode::OperatorNoLeftValue;
  }

  // Check that there is a valid value to the right of the operator
  if(rightValue > infixVector.size() - 1 ||
     (nullptr != std::dynamic_pointer_cast<CalculatorOperator>(infixVector[rightValue]) && std::dynamic_pointer_cast<CalculatorOperator>(infixVector[rightValue])->getOperatorType() == Binary) ||
     nullptr != std::dynamic_pointer_cast<RightParenthesisItem>(infixVector[rightValue]))
  {
    errMsg = fmt::format("The operator '{}' does not have a valid 'right' value.", getInfixToken());
    return CalculatorItem::ErrorCode::OperatorNoRightValue;
  }

  return CalculatorItem::ErrorCode::Success;
}

// -----------------------------------------------------------------------------
BinaryOperator::Pointer BinaryOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
