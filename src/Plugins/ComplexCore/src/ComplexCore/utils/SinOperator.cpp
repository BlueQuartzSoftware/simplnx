#include "Core/Utilities/SinOperator.h"

#include <cmath>

#include "Core/Utilities/CalculatorArray.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SinOperator::SinOperator()
{
  setNumberOfArguments(1);
  setInfixToken("sin");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SinOperator::~SinOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SinOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTrig(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return sin(num); });
}

// -----------------------------------------------------------------------------
SinOperator::Pointer SinOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
