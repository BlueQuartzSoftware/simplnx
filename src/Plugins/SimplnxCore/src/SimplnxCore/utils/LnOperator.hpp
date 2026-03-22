#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class CalculatorNumber;

class SIMPLNXCORE_EXPORT LnOperator : public UnaryOperator
{
public:
  using Self = LnOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new LnOperator());
  }

  ~LnOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  LnOperator();

public:
  LnOperator(const LnOperator&) = delete;            // Copy Constructor Not Implemented
  LnOperator(LnOperator&&) = delete;                 // Move Constructor Not Implemented
  LnOperator& operator=(const LnOperator&) = delete; // Copy Assignment Not Implemented
  LnOperator& operator=(LnOperator&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace nx::core
