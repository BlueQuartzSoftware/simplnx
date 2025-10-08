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
  ICalculatorArray::Pointer iArray1 = executionStack.top();
  if(executionStack.size() >= 2 && nullptr != iArray1)
  {
    executionStack.pop();
    ICalculatorArray::Pointer iArray2 = executionStack.top();
    executionStack.pop();

    calculatedArrayPath = GetUniquePathName(dataStructure, calculatedArrayPath);

    DataArray<float64>* array1 = iArray1->getArray();
    DataArray<float64>* array2 = iArray2->getArray();

    Float64Array* newArray = nullptr;
    if(iArray1->getType() == ICalculatorArray::Array)
    {
      newArray = Float64Array::CreateWithStore<Float64DataStore>(dataStructure, calculatedArrayPath.getTargetName(), array1->getTupleShape(), array1->getComponentShape());
    }
    else
    {
      newArray = Float64Array::CreateWithStore<Float64DataStore>(dataStructure, calculatedArrayPath.getTargetName(), array2->getTupleShape(), array2->getComponentShape());
    }

    usize numComps = newArray->getNumberOfComponents();
    for(usize i = 0; i < newArray->getNumberOfTuples(); i++)
    {
      for(usize c = 0; c < newArray->getNumberOfComponents(); c++)
      {
        usize index = numComps * i + c;
        float64 num1 = (iArray1->getType() == ICalculatorArray::Array) ? array1->getValue(index) : array1->getValue(0);
        float64 num2 = (iArray2->getType() == ICalculatorArray::Array) ? array2->getValue(index) : array2->getValue(0);
        (*newArray)[index] = op(num2, num1);
      }
    }

    if(iArray1->getType() == ICalculatorArray::Array || iArray2->getType() == ICalculatorArray::Array)
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
