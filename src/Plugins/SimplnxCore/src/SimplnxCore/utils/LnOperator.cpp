#include "SimplnxCore/utils/LnOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
LnOperator::LnOperator()
{
  setNumberOfArguments(1);
  setInfixToken("ln");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
LnOperator::~LnOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void LnOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayStandardUnary(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return log(num); });
}

// -----------------------------------------------------------------------------
LnOperator::Pointer LnOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
