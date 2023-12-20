#include "SimplnxCore/utils/CalculatorOperator.hpp"

#include "simplnx/Common/Numbers.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorOperator::CalculatorOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorOperator::~CalculatorOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool CalculatorOperator::hasHigherPrecedence(const CalculatorOperator::Pointer other)
{
  return m_Precedence > other->m_Precedence;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorOperator::Precedence CalculatorOperator::getPrecedence()
{
  return m_Precedence;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void CalculatorOperator::setPrecedence(Precedence precedence)
{
  m_Precedence = precedence;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorOperator::OperatorType CalculatorOperator::getOperatorType()
{
  return m_OperatorType;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void CalculatorOperator::setOperatorType(OperatorType type)
{
  m_OperatorType = type;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
double CalculatorOperator::toDegrees(double radians)
{
  return radians * (180.0 / numbers::pi);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
double CalculatorOperator::toRadians(double degrees)
{
  return degrees * (numbers::pi / 180.0);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
double CalculatorOperator::root(double base, double root)
{
  if(root == 0)
  {
    return std::numeric_limits<double>::infinity();
  }

  return pow(base, 1 / root);
}

// -----------------------------------------------------------------------------
CalculatorOperator::Pointer CalculatorOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
void CalculatorOperator::CreateNewArrayTwoArguments(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath,
                                                    std::stack<ICalculatorArray::Pointer>& executionStack, std::function<double(double, double)> op)
{
  ICalculatorArray::Pointer array1 = executionStack.top();
  if(executionStack.size() >= 2 && nullptr != array1)
  {
    executionStack.pop();
    ICalculatorArray::Pointer array2 = executionStack.top();
    executionStack.pop();

    calculatedArrayPath = GetUniquePathName(dataStructure, calculatedArrayPath);

    Float64Array* newArray = nullptr;
    if(array1->getType() == ICalculatorArray::Array)
    {
      newArray = Float64Array::CreateWithStore<Float64DataStore>(dataStructure, calculatedArrayPath.getTargetName(), array1->getArray()->getTupleShape(), array1->getArray()->getComponentShape());
    }
    else
    {
      newArray = Float64Array::CreateWithStore<Float64DataStore>(dataStructure, calculatedArrayPath.getTargetName(), array2->getArray()->getTupleShape(), array2->getArray()->getComponentShape());
    }

    int numComps = newArray->getNumberOfComponents();
    for(int i = 0; i < static_cast<int>(newArray->getNumberOfTuples()); i++)
    {
      for(int c = 0; c < newArray->getNumberOfComponents(); c++)
      {
        int index = numComps * i + c;
        double num1 = array1->getValue(index);
        double num2 = array2->getValue(index);
        (*newArray)[index] = op(num2, num1);
      }
    }

    if(array1->getType() == ICalculatorArray::Array || array2->getType() == ICalculatorArray::Array)
    {
      executionStack.push(CalculatorArray<double>::New(dataStructure, newArray, ICalculatorArray::Array, true));
    }
    else
    {
      executionStack.push(CalculatorArray<double>::New(dataStructure, newArray, ICalculatorArray::Number, true));
    }
    return;
  }
}
