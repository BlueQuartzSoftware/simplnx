#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class CalculatorNumber;

class SIMPLNXCORE_EXPORT CeilOperator : public UnaryOperator
{
public:
  using Self = CeilOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new CeilOperator());
  }

  ~CeilOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  CeilOperator();

public:
  CeilOperator(const CeilOperator&) = delete;            // Copy Constructor Not Implemented
  CeilOperator(CeilOperator&&) = delete;                 // Move Constructor Not Implemented
  CeilOperator& operator=(const CeilOperator&) = delete; // Copy Assignment Not Implemented
  CeilOperator& operator=(CeilOperator&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace nx::core
