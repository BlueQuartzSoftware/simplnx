#include "UnaryOperator.hpp"

#include "SimplnxCore/Filters/ArrayCalculatorFilter.hpp"
#include "SimplnxCore/utils/BinaryOperator.hpp"
#include "SimplnxCore/utils/CommaSeparator.hpp"
#include "SimplnxCore/utils/LeftParenthesisItem.hpp"
#include "SimplnxCore/utils/NegativeOperator.hpp"
#include "SimplnxCore/utils/RightParenthesisItem.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
UnaryOperator::UnaryOperator()
: m_NumOfArguments(-1)
{
  setPrecedence(E_Precedence);
  setOperatorType(Unary);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
UnaryOperator::~UnaryOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void UnaryOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  // This should never be executed
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorItem::ErrorCode UnaryOperator::checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& errMsg)
{
  if(currentIndex - 1 >= 0)
  {
    // If the left value isn't a binary operator
    if(nullptr == std::dynamic_pointer_cast<BinaryOperator>(infixVector[currentIndex - 1]))
    {
      // If the left value isn't a left parenthesis
      if(nullptr == std::dynamic_pointer_cast<LeftParenthesisItem>(infixVector[currentIndex - 1]))
      {
        // If the left value isn't a negative operator
        if(nullptr == std::dynamic_pointer_cast<NegativeOperator>(infixVector[currentIndex - 1]))
        {
          errMsg = fmt::format("The operator '{}' does not have a valid 'left' value.", getInfixToken());
          return CalculatorItem::ErrorCode::OperatorNoLeftValue;
        }
      }
    }
  }

  int index = currentIndex + 1;
  int commaCount = 0;
  bool hasArray = false;
  if(index < infixVector.size() && nullptr != std::dynamic_pointer_cast<LeftParenthesisItem>(infixVector[index]))
  {
    index++;

    // Iterate through the vector to find the matching right parenthesis
    for(; index < infixVector.size(); index++)
    {
      if(nullptr != std::dynamic_pointer_cast<RightParenthesisItem>(infixVector[index]))
      {
        // We found the matching right parenthesis
        if(commaCount < m_NumOfArguments - 1)
        {
          errMsg = fmt::format("The operator '{}' needs {} arguments.  {} arguments were found.", getInfixToken(), m_NumOfArguments, commaCount + 1);
          return CalculatorItem::ErrorCode::NotEnoughArguments;
        }
        if(!hasArray)
        {
          errMsg = fmt::format("The operator '{}' does not have any arguments that simplify down to a number.", getInfixToken());
          return CalculatorItem::ErrorCode::NoNumericArguments;
        }

        return CalculatorItem::ErrorCode::Success;
      }
      if(nullptr != std::dynamic_pointer_cast<LeftParenthesisItem>(infixVector[index]))
      {
        /* We found another left parenthesis, but we don't care what's inside this set of parentheses
           (other operators' checkValidity functions will take care of these values), so just iterate
           until we find the matching closing parenthesis for this opening parenthesis */
        int extraLeftPCount = 0;
        index++;
        while(index < infixVector.size() && (nullptr == std::dynamic_pointer_cast<RightParenthesisItem>(infixVector[index]) || extraLeftPCount > 0))
        {
          if(nullptr != std::dynamic_pointer_cast<ICalculatorArray>(infixVector[index]))
          {
            hasArray = true;
          }
          else if(nullptr != std::dynamic_pointer_cast<LeftParenthesisItem>(infixVector[index]))
          {
            extraLeftPCount++;
          }
          else if(nullptr != std::dynamic_pointer_cast<RightParenthesisItem>(infixVector[index]))
          {
            extraLeftPCount--;
          }

          index++;
        }
      }
      else if(nullptr != std::dynamic_pointer_cast<CommaSeparator>(infixVector[index]))
      {
        // We found a comma, so increase the comma count
        commaCount++;
        if(commaCount > m_NumOfArguments - 1)
        {
          // We found too many commas (meaning that there are too many arguments), so return false
          errMsg = fmt::format("The operator '{}' needs {} arguments.  {} arguments were found.", getInfixToken(), m_NumOfArguments, commaCount + 1);
          return CalculatorItem::ErrorCode::TooManyArguments;
        }
      }
      else if(nullptr != std::dynamic_pointer_cast<ICalculatorArray>(infixVector[index]))
      {
        hasArray = true;
      }
    }
  }
  else
  {
    errMsg = fmt::format("The operator '{}' does not have an opening parenthesis.", getInfixToken());
    return CalculatorItem::ErrorCode::OperatorNoOpeningParen;
  }

  errMsg = fmt::format("The operator '{}' does not have a closing parenthesis.", getInfixToken());
  return CalculatorItem::ErrorCode::OperatorNoClosingParen;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int UnaryOperator::getNumberOfArguments()
{
  return m_NumOfArguments;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void UnaryOperator::setNumberOfArguments(int numOfArguments)
{
  m_NumOfArguments = numOfArguments;
}

// -----------------------------------------------------------------------------
UnaryOperator::Pointer UnaryOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
void UnaryOperator::CreateNewArrayStandardUnary(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath,
                                                std::stack<ICalculatorArray::Pointer>& executionStack, std::function<double(double)> op)
{
  ICalculatorArray::Pointer arrayPtr = executionStack.top();
  if(!executionStack.empty() && nullptr != arrayPtr)
  {
    executionStack.pop();
    calculatedArrayPath = GetUniquePathName(dataStructure, calculatedArrayPath);

    Float64Array* newArray =
        Float64Array::CreateWithStore<Float64DataStore>(dataStructure, calculatedArrayPath.getTargetName(), arrayPtr->getArray()->getTupleShape(), arrayPtr->getArray()->getComponentShape());

    int numComps = newArray->getNumberOfComponents();
    for(int i = 0; i < newArray->getNumberOfTuples(); i++)
    {
      for(int c = 0; c < newArray->getNumberOfComponents(); c++)
      {
        int index = numComps * i + c;
        double num = arrayPtr->getValue(index);
        (*newArray)[index] = op(num);
      }
    }

    executionStack.push(CalculatorArray<double>::New(dataStructure, newArray, arrayPtr->getType(), true));
    return;
  }
}

// -----------------------------------------------------------------------------
void UnaryOperator::CreateNewArrayTrig(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack,
                                       std::function<double(double)> op)
{
  ICalculatorArray::Pointer arrayPtr = executionStack.top();
  if(!executionStack.empty() && nullptr != arrayPtr)
  {
    executionStack.pop();
    calculatedArrayPath = GetUniquePathName(dataStructure, calculatedArrayPath);

    Float64Array* newArray =
        Float64Array::CreateWithStore<Float64DataStore>(dataStructure, calculatedArrayPath.getTargetName(), arrayPtr->getArray()->getTupleShape(), arrayPtr->getArray()->getComponentShape());

    int numComps = newArray->getNumberOfComponents();
    for(int i = 0; i < newArray->getNumberOfTuples(); i++)
    {
      for(int c = 0; c < newArray->getNumberOfComponents(); c++)
      {
        int index = numComps * i + c;
        double num = arrayPtr->getValue(index);

        if(units == CalculatorParameter::AngleUnits::Degrees)
        {
          (*newArray)[index] = op(toRadians(num));
        }
        else
        {
          (*newArray)[index] = op(num);
        }
      }
    }

    executionStack.push(CalculatorArray<double>::New(dataStructure, newArray, arrayPtr->getType(), true));
    return;
  }
}

// -----------------------------------------------------------------------------
void UnaryOperator::CreateNewArrayArcTrig(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack,
                                          std::function<double(double)> op)
{
  ICalculatorArray::Pointer arrayPtr = executionStack.top();
  if(!executionStack.empty() && nullptr != arrayPtr)
  {
    executionStack.pop();
    calculatedArrayPath = GetUniquePathName(dataStructure, calculatedArrayPath);

    Float64Array* newArray =
        Float64Array::CreateWithStore<Float64DataStore>(dataStructure, calculatedArrayPath.getTargetName(), arrayPtr->getArray()->getTupleShape(), arrayPtr->getArray()->getComponentShape());

    int numComps = newArray->getNumberOfComponents();
    for(int i = 0; i < newArray->getNumberOfTuples(); i++)
    {
      for(int c = 0; c < newArray->getNumberOfComponents(); c++)
      {
        int index = numComps * i + c;
        double num = arrayPtr->getValue(index);

        if(units == CalculatorParameter::AngleUnits::Degrees)
        {
          (*newArray)[index] = toDegrees(op(num));
        }
        else
        {
          (*newArray)[index] = op(num);
        }
      }
    }

    executionStack.push(CalculatorArray<double>::New(dataStructure, newArray, arrayPtr->getType(), true));
    return;
  }
}
