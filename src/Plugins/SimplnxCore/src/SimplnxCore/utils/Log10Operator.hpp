#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class CalculatorNumber;

class SIMPLNXCORE_EXPORT Log10Operator : public UnaryOperator
{
public:
  using Self = Log10Operator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new Log10Operator());
  }

  ~Log10Operator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  Log10Operator();

public:
  Log10Operator(const Log10Operator&) = delete;            // Copy Constructor Not Implemented
  Log10Operator(Log10Operator&&) = delete;                 // Move Constructor Not Implemented
  Log10Operator& operator=(const Log10Operator&) = delete; // Copy Assignment Not Implemented
  Log10Operator& operator=(Log10Operator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace nx::core
