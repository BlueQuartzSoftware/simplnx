#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class CalculatorNumber;

class SIMPLNXCORE_EXPORT MaxOperator : public UnaryOperator
{
public:
  using Self = MaxOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new MaxOperator());
  }

  ~MaxOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  MaxOperator();

public:
  MaxOperator(const MaxOperator&) = delete;            // Copy Constructor Not Implemented
  MaxOperator(MaxOperator&&) = delete;                 // Move Constructor Not Implemented
  MaxOperator& operator=(const MaxOperator&) = delete; // Copy Assignment Not Implemented
  MaxOperator& operator=(MaxOperator&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace nx::core
