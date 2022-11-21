#pragma once

#include <memory>
#include <stack>

#include "Core/Core_export.hpp"

#include "Core/Utilities/UnaryOperator.h"

namespace complex
{
class CalculatorNumber;

class CORE_EXPORT ExpOperator : public UnaryOperator
{
public:
  using Self = ExpOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new ExpOperator());
  }

  ~ExpOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  ExpOperator();

public:
  ExpOperator(const ExpOperator&) = delete;            // Copy Constructor Not Implemented
  ExpOperator(ExpOperator&&) = delete;                 // Move Constructor Not Implemented
  ExpOperator& operator=(const ExpOperator&) = delete; // Copy Assignment Not Implemented
  ExpOperator& operator=(ExpOperator&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace complex
