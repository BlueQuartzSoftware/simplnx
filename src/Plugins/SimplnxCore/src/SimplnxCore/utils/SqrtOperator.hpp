#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class CalculatorNumber;

class SIMPLNXCORE_EXPORT SqrtOperator : public UnaryOperator
{
public:
  using Self = SqrtOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new SqrtOperator());
  }

  ~SqrtOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  SqrtOperator();

public:
  SqrtOperator(const SqrtOperator&) = delete;            // Copy Constructor Not Implemented
  SqrtOperator(SqrtOperator&&) = delete;                 // Move Constructor Not Implemented
  SqrtOperator& operator=(const SqrtOperator&) = delete; // Copy Assignment Not Implemented
  SqrtOperator& operator=(SqrtOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace nx::core
