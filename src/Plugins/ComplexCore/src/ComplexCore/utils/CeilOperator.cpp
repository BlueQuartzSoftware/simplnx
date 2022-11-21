#include "Core/Utilities/CeilOperator.h"

#include <cmath>

#include "Core/Utilities/CalculatorArray.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CeilOperator::CeilOperator()
{
  setNumberOfArguments(1);
  setInfixToken("ceil");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CeilOperator::~CeilOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void CeilOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayStandardUnary(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return ceil(num); });
}

// -----------------------------------------------------------------------------
CeilOperator::Pointer CeilOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
