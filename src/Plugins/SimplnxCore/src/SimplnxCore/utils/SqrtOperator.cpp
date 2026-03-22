#include "SimplnxCore/utils/SqrtOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SqrtOperator::SqrtOperator()
{
  setNumberOfArguments(1);
  setInfixToken("sqrt");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SqrtOperator::~SqrtOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SqrtOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayStandardUnary(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return sqrt(num); });
}

// -----------------------------------------------------------------------------
SqrtOperator::Pointer SqrtOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
