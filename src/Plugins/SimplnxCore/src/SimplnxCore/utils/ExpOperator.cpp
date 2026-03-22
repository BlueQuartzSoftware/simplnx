#include "SimplnxCore/utils/ExpOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ExpOperator::ExpOperator()
{
  setNumberOfArguments(1);
  setInfixToken("exp");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ExpOperator::~ExpOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ExpOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayStandardUnary(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return exp(num); });
}

// -----------------------------------------------------------------------------
ExpOperator::Pointer ExpOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
