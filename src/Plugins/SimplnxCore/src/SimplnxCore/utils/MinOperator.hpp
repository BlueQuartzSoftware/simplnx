#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class CalculatorNumber;

class SIMPLNXCORE_EXPORT MinOperator : public UnaryOperator
{
public:
  using Self = MinOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new MinOperator());
  }

  ~MinOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  MinOperator();

public:
  MinOperator(const MinOperator&) = delete;            // Copy Constructor Not Implemented
  MinOperator(MinOperator&&) = delete;                 // Move Constructor Not Implemented
  MinOperator& operator=(const MinOperator&) = delete; // Copy Assignment Not Implemented
  MinOperator& operator=(MinOperator&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace nx::core
