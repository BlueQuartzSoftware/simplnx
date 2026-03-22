#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class CalculatorNumber;

class SIMPLNXCORE_EXPORT CosOperator : public UnaryOperator
{
public:
  using Self = CosOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new CosOperator());
  }

  ~CosOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  CosOperator();

public:
  CosOperator(const CosOperator&) = delete;            // Copy Constructor Not Implemented
  CosOperator(CosOperator&&) = delete;                 // Move Constructor Not Implemented
  CosOperator& operator=(const CosOperator&) = delete; // Copy Assignment Not Implemented
  CosOperator& operator=(CosOperator&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace nx::core
