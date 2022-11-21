#pragma once

#include <memory>

#include <stack>

#include "Core/Core_export.hpp"

#include "Core/Utilities/BinaryOperator.h"

namespace complex
{
class CalculatorNumber;

class CORE_EXPORT PowOperator : public BinaryOperator
{
public:
  using Self = PowOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new PowOperator());
  }

  ~PowOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  PowOperator();

public:
  PowOperator(const PowOperator&) = delete;            // Copy Constructor Not Implemented
  PowOperator(PowOperator&&) = delete;                 // Move Constructor Not Implemented
  PowOperator& operator=(const PowOperator&) = delete; // Copy Assignment Not Implemented
  PowOperator& operator=(PowOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace complex
