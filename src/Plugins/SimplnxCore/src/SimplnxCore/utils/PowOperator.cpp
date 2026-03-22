#include "SimplnxCore/utils/PowOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
PowOperator::PowOperator()
{
  setPrecedence(C_Precedence);
  setInfixToken("^");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
PowOperator::~PowOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void PowOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTwoArguments(dataStructure, units, calculatedArrayPath, executionStack, [](double num1, double num2) -> double { return pow(num1, num2); });
}

// -----------------------------------------------------------------------------
PowOperator::Pointer PowOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
