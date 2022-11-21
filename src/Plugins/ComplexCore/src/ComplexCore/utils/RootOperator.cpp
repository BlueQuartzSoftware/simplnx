#include "Core/Utilities/RootOperator.h"

#include <cmath>

#include "Core/Utilities/CalculatorArray.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
RootOperator::RootOperator()
{
  setNumberOfArguments(2);
  setInfixToken("root");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
RootOperator::~RootOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RootOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTwoArguments(dataStructure, units, calculatedArrayPath, executionStack, [this](double num1, double num2) -> double { return root(num1, num2); });
}

// -----------------------------------------------------------------------------
RootOperator::Pointer RootOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
