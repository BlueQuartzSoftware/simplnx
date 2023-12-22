#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/BinaryOperator.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT DivisionOperator : public BinaryOperator
{
public:
  using Self = DivisionOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new DivisionOperator());
  }

  ~DivisionOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  DivisionOperator();

public:
  DivisionOperator(const DivisionOperator&) = delete;            // Copy Constructor Not Implemented
  DivisionOperator(DivisionOperator&&) = delete;                 // Move Constructor Not Implemented
  DivisionOperator& operator=(const DivisionOperator&) = delete; // Copy Assignment Not Implemented
  DivisionOperator& operator=(DivisionOperator&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace nx::core
