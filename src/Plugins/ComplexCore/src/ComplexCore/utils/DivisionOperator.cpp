#include "Core/Utilities/DivisionOperator.h"

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Eigen>

#include "Core/Utilities/LeftParenthesisItem.h"
#include "Core/Utilities/RightParenthesisItem.h"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DivisionOperator::DivisionOperator()
{
  setPrecedence(B_Precedence);
  setInfixToken("/");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DivisionOperator::~DivisionOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DivisionOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  CreateNewArrayTwoArguments(dataStructure, units, calculatedArrayPath, executionStack, [](double num1, double num2) -> double { return num1 / num2; });
}

// -----------------------------------------------------------------------------
DivisionOperator::Pointer DivisionOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
