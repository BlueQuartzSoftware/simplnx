#include "ComplexCore/utils/SubtractionOperator.hpp"
#include "ComplexCore/utils/CalculatorArray.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SubtractionOperator::SubtractionOperator()
{
  setPrecedence(A_Precedence);
  setInfixToken("-");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SubtractionOperator::~SubtractionOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SubtractionOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTwoArguments(dataStructure, units, calculatedArrayPath, executionStack, [](double num1, double num2) -> double { return num1 - num2; });
}

// -----------------------------------------------------------------------------
SubtractionOperator::Pointer SubtractionOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
