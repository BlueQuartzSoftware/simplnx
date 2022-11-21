#include "Core/Utilities/ATanOperator.h"

#include <cmath>

#include "Core/Utilities/CalculatorArray.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ATanOperator::ATanOperator()
{
  setNumberOfArguments(1);
  setInfixToken("atan");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ATanOperator::~ATanOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ATanOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayArcTrig(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return atan(num); });
}

// -----------------------------------------------------------------------------
ATanOperator::Pointer ATanOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
