#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class CalculatorNumber;

class SIMPLNXCORE_EXPORT FloorOperator : public UnaryOperator
{
public:
  using Self = FloorOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new FloorOperator());
  }

  ~FloorOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  FloorOperator();

public:
  FloorOperator(const FloorOperator&) = delete;            // Copy Constructor Not Implemented
  FloorOperator(FloorOperator&&) = delete;                 // Move Constructor Not Implemented
  FloorOperator& operator=(const FloorOperator&) = delete; // Copy Assignment Not Implemented
  FloorOperator& operator=(FloorOperator&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace nx::core
