#include "SimplnxCore/utils/AdditionOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AdditionOperator::AdditionOperator()
{
  setPrecedence(A_Precedence);
  setInfixToken("+");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AdditionOperator::~AdditionOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void AdditionOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTwoArguments(dataStructure, units, calculatedArrayPath, executionStack, [](double num1, double num2) -> double { return num1 + num2; });
}

// -----------------------------------------------------------------------------
AdditionOperator::Pointer AdditionOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
