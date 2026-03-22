#include "SimplnxCore/utils/LogOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
LogOperator::LogOperator()
{
  setNumberOfArguments(2);
  setInfixToken("log");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
LogOperator::~LogOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void LogOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTwoArguments(dataStructure, units, calculatedArrayPath, executionStack, [this](double num1, double num2) -> double { return log_arbitrary_base(num1, num2); });
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
double LogOperator::log_arbitrary_base(double base, double value)
{
  return log(value) / log(base);
}

// -----------------------------------------------------------------------------
LogOperator::Pointer LogOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
