#pragma once

#include <memory>
#include <stack>

#include "Core/Core_export.hpp"

#include "Core/Utilities/UnaryOperator.h"

namespace complex
{
class CalculatorNumber;

class CORE_EXPORT ACosOperator : public UnaryOperator
{
public:
  using Self = ACosOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new ACosOperator());
  }

  ~ACosOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  ACosOperator();

public:
  ACosOperator(const ACosOperator&) = delete;            // Copy Constructor Not Implemented
  ACosOperator(ACosOperator&&) = delete;                 // Move Constructor Not Implemented
  ACosOperator& operator=(const ACosOperator&) = delete; // Copy Assignment Not Implemented
  ACosOperator& operator=(ACosOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace complex