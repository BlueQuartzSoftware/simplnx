#include "SimplnxCore/utils/SinOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

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
