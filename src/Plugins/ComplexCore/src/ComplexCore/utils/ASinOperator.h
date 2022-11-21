#pragma once

#include <memory>
#include <stack>

#include "Core/Core_export.hpp"

#include "Core/Utilities/UnaryOperator.h"

namespace complex
{

class CalculatorNumber;

class CORE_EXPORT ASinOperator : public UnaryOperator
{
public:
  using Self = ASinOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new ASinOperator());
  }

  ~ASinOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  ASinOperator();

public:
  ASinOperator(const ASinOperator&) = delete;            // Copy Constructor Not Implemented
  ASinOperator(ASinOperator&&) = delete;                 // Move Constructor Not Implemented
  ASinOperator& operator=(const ASinOperator&) = delete; // Copy Assignment Not Implemented
  ASinOperator& operator=(ASinOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace complex