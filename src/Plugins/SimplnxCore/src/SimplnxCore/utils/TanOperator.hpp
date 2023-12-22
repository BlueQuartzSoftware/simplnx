#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class CalculatorNumber;

class SIMPLNXCORE_EXPORT TanOperator : public UnaryOperator
{
public:
  using Self = TanOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new TanOperator());
  }

  ~TanOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  TanOperator();

public:
  TanOperator(const TanOperator&) = delete;            // Copy Constructor Not Implemented
  TanOperator(TanOperator&&) = delete;                 // Move Constructor Not Implemented
  TanOperator& operator=(const TanOperator&) = delete; // Copy Assignment Not Implemented
  TanOperator& operator=(TanOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace nx::core
