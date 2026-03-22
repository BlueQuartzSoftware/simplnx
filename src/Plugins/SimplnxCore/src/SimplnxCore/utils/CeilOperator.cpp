#include "SimplnxCore/utils/CeilOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

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
