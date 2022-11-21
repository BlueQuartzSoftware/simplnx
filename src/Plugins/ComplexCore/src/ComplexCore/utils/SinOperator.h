#pragma once

#include <memory>

#include <stack>

#include "Core/Core_export.hpp"

#include "Core/Utilities/UnaryOperator.h"

namespace complex
{
class CalculatorNumber;

class CORE_EXPORT SinOperator : public UnaryOperator
{
public:
  using Self = SinOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new SinOperator());
  }

  ~SinOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  SinOperator();

public:
  SinOperator(const SinOperator&) = delete;            // Copy Constructor Not Implemented
  SinOperator(SinOperator&&) = delete;                 // Move Constructor Not Implemented
  SinOperator& operator=(const SinOperator&) = delete; // Copy Assignment Not Implemented
  SinOperator& operator=(SinOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace complex
