#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{

class CalculatorNumber;

class SIMPLNXCORE_EXPORT ATanOperator : public UnaryOperator
{
public:
  using Self = ATanOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new ATanOperator());
  }

  ~ATanOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  ATanOperator();

public:
  ATanOperator(const ATanOperator&) = delete;            // Copy Constructor Not Implemented
  ATanOperator(ATanOperator&&) = delete;                 // Move Constructor Not Implemented
  ATanOperator& operator=(const ATanOperator&) = delete; // Copy Assignment Not Implemented
  ATanOperator& operator=(ATanOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace nx::core