#include "SimplnxCore/utils/ASinOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ASinOperator::ASinOperator()
{
  setNumberOfArguments(1);
  setInfixToken("asin");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ASinOperator::~ASinOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ASinOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayArcTrig(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return asin(num); });
}

// -----------------------------------------------------------------------------
ASinOperator::Pointer ASinOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
