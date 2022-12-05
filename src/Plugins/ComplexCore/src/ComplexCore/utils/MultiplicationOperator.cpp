#include "Core/Utilities/MultiplicationOperator.h"

#include <cmath>

#include "Core/Utilities/CalculatorArray.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
MultiplicationOperator::MultiplicationOperator()
{
  setPrecedence(B_Precedence);
  setInfixToken("*");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
MultiplicationOperator::~MultiplicationOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void MultiplicationOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTwoArguments(dataStructure, units, calculatedArrayPath, executionStack, [](double num1, double num2) -> double { return num1 * num2; });
}

// -----------------------------------------------------------------------------
MultiplicationOperator::Pointer MultiplicationOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
