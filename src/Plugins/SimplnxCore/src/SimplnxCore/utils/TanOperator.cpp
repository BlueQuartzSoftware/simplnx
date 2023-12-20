#include "SimplnxCore/utils/TanOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
TanOperator::TanOperator()
{
  setNumberOfArguments(1);
  setInfixToken("tan");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
TanOperator::~TanOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void TanOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTrig(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return tan(num); });
}

// -----------------------------------------------------------------------------
TanOperator::Pointer TanOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
