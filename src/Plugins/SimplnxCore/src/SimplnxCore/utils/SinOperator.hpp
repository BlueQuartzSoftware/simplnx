#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class CalculatorNumber;

class SIMPLNXCORE_EXPORT SinOperator : public UnaryOperator
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

} // namespace nx::core
