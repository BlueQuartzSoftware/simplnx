#pragma once

#include "ComplexCore/ComplexCore_export.hpp"
#include "ComplexCore/utils/UnaryOperator.hpp"

namespace complex
{
class CalculatorNumber;

class COMPLEXCORE_EXPORT ABSOperator : public UnaryOperator
{
public:
  using Self = ABSOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new ABSOperator());
  }

  ~ABSOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  ABSOperator();

public:
  ABSOperator(const ABSOperator&) = delete;            // Copy Constructor Not Implemented
  ABSOperator(ABSOperator&&) = delete;                 // Move Constructor Not Implemented
  ABSOperator& operator=(const ABSOperator&) = delete; // Copy Assignment Not Implemented
  ABSOperator& operator=(ABSOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace complex
