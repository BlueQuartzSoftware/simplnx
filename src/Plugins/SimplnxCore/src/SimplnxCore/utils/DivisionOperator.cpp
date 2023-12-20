#include "SimplnxCore/utils/DivisionOperator.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DivisionOperator::DivisionOperator()
{
  setPrecedence(B_Precedence);
  setInfixToken("/");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DivisionOperator::~DivisionOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DivisionOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTwoArguments(dataStructure, units, calculatedArrayPath, executionStack, [](double num1, double num2) -> double { return num1 / num2; });
}

// -----------------------------------------------------------------------------
DivisionOperator::Pointer DivisionOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
