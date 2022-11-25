#pragma once

#include <memory>
#include <stack>

#include "ComplexCore/ComplexCore_export.hpp"
#include "ComplexCore/utils/BinaryOperator.hpp"

namespace complex
{

class CalculatorNumber;

class COMPLEXCORE_EXPORT AdditionOperator : public BinaryOperator
{
public:
  using Self = AdditionOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new AdditionOperator());
  }

  ~AdditionOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  AdditionOperator();

public:
  AdditionOperator(const AdditionOperator&) = delete;            // Copy Constructor Not Implemented
  AdditionOperator(AdditionOperator&&) = delete;                 // Move Constructor Not Implemented
  AdditionOperator& operator=(const AdditionOperator&) = delete; // Copy Assignment Not Implemented
  AdditionOperator& operator=(AdditionOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace complex
