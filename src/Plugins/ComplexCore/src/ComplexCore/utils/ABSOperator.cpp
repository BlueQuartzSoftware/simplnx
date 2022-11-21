#include "Core/Utilities/ABSOperator.h"

#include <cmath>

#include "Core/Utilities/CalculatorArray.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ABSOperator::ABSOperator()
{
  setNumberOfArguments(1);
  setInfixToken("abs");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ABSOperator::~ABSOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ABSOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayStandardUnary(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return fabs(num); });
}

// -----------------------------------------------------------------------------
ABSOperator::Pointer ABSOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
