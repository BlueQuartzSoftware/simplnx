#include "SimplnxCore/utils/MinOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;
using namespace std;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
MinOperator::MinOperator()
{
  setNumberOfArguments(2);
  setInfixToken("min");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
MinOperator::~MinOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void MinOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTwoArguments(dataStructure, units, calculatedArrayPath, executionStack, [](double num1, double num2) -> double { return min(num1, num2); });
}

// -----------------------------------------------------------------------------
MinOperator::Pointer MinOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
