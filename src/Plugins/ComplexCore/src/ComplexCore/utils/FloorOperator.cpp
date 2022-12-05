#include "Core/Utilities/FloorOperator.h"

#include <cmath>

#include "Core/Utilities/CalculatorArray.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FloorOperator::FloorOperator()
{
  setNumberOfArguments(1);
  setInfixToken("floor");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FloorOperator::~FloorOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FloorOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayStandardUnary(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return floor(num); });
}

// -----------------------------------------------------------------------------
FloorOperator::Pointer FloorOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
