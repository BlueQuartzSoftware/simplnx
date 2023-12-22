#include "SimplnxCore/utils/ACosOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ACosOperator::ACosOperator()
{
  setNumberOfArguments(1);
  setInfixToken("acos");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ACosOperator::~ACosOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ACosOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayArcTrig(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return acos(num); });
}

// -----------------------------------------------------------------------------
ACosOperator::Pointer ACosOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
