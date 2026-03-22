#include "SimplnxCore/utils/Log10Operator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Log10Operator::Log10Operator()
{
  setNumberOfArguments(1);
  setInfixToken("log10");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Log10Operator::~Log10Operator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void Log10Operator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayStandardUnary(dataStructure, units, calculatedArrayPath, executionStack, [](double num) -> double { return log10(num); });
}

// -----------------------------------------------------------------------------
Log10Operator::Pointer Log10Operator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
