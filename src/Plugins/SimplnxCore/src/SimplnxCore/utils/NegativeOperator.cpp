#include "SimplnxCore/utils/NegativeOperator.hpp"
#include "SimplnxCore/utils/BinaryOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"
#include "SimplnxCore/utils/LeftParenthesisItem.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
NegativeOperator::NegativeOperator()
{
  setOperatorType(Unary);
  setPrecedence(D_Precedence);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
NegativeOperator::~NegativeOperator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void NegativeOperator::calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack)
{
  if(!executionStack.empty())
  {
    ICalculatorArray::Pointer arrayPtr = executionStack.top();
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
        (*newArray)[index] = -1 * num;
      }
    }

    executionStack.push(CalculatorArray<double>::New(dataStructure, newArray, arrayPtr->getType(), true));
    return;
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorItem::ErrorCode NegativeOperator::checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& errMsg)
{
  if(currentIndex - 1 >= 0 && (std::dynamic_pointer_cast<BinaryOperator>(infixVector[currentIndex - 1]) == BinaryOperator::NullPointer() &&
                               std::dynamic_pointer_cast<LeftParenthesisItem>(infixVector[currentIndex - 1]) == LeftParenthesisItem::NullPointer()))
  {
    // The symbol to the left of the negative sign is not a binary operator or left parenthesis
    errMsg = fmt::format("The negative operator '{}' does not have a valid 'left' value.", getInfixToken());
    return NegativeOperator::ErrorCode::OperatorNoLeftValue;
  }
  if(currentIndex + 1 >= infixVector.size() || (currentIndex + 1 < infixVector.size() && (nullptr == std::dynamic_pointer_cast<ICalculatorArray>(infixVector[currentIndex + 1]) &&
                                                                                          nullptr == std::dynamic_pointer_cast<LeftParenthesisItem>(infixVector[currentIndex + 1]) &&
                                                                                          nullptr == std::dynamic_pointer_cast<UnaryOperator>(infixVector[currentIndex + 1]))))
  {
    // The symbol to the right of the negative sign is not an array, left parenthesis, or unary operator
    errMsg = fmt::format("The negative operator '{}' does not have a valid 'right' value.", getInfixToken());
    return NegativeOperator::ErrorCode::OperatorNoRightValue;
  }

  return NegativeOperator::ErrorCode::Success;
}

// -----------------------------------------------------------------------------
NegativeOperator::Pointer NegativeOperator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
