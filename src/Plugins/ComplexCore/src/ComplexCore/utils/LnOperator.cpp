#include "Core/Utilities/LnOperator.h"

#include <cmath>

#include "Core/Utilities/CalculatorArray.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
LnOperator::LnOperator()
{
  setNumberOfArguments(1);
  setInfixToken("ln");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
LnOperator::~LnOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void LnOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayStandardUnary(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return log(num); });
}

// -----------------------------------------------------------------------------
LnOperator::Pointer LnOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
