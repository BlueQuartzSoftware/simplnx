#include "Core/Utilities/TanOperator.h"

#include <cmath>

#include "Core/Utilities/CalculatorArray.hpp"

using namespace complex;

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
