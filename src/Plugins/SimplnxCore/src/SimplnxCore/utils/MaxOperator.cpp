#include "SimplnxCore/utils/MaxOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;
using namespace std;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
MaxOperator::MaxOperator()
{
  setNumberOfArguments(2);
  setInfixToken("max");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
MaxOperator::~MaxOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void MaxOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTwoArguments(dataStructure, units, calculatedArrayPath, executionStack, [](double num1, double num2) -> double { return max(num1, num2); });
}

// -----------------------------------------------------------------------------
MaxOperator::Pointer MaxOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
