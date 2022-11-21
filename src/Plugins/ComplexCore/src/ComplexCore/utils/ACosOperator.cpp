#include "Core/Utilities/ACosOperator.h"

#include <cmath>

#include "Core/Utilities/CalculatorArray.hpp"

using namespace complex;

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
