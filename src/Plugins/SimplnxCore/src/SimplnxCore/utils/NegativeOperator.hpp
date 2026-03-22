#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/UnaryOperator.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT NegativeOperator : public CalculatorOperator
{
public:
  using Self = NegativeOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new NegativeOperator());
  }

  ~NegativeOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

  CalculatorItem::ErrorCode checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& errMsg) final;

protected:
  NegativeOperator();

public:
  NegativeOperator(const NegativeOperator&) = delete;            // Copy Constructor Not Implemented
  NegativeOperator(NegativeOperator&&) = delete;                 // Move Constructor Not Implemented
  NegativeOperator& operator=(const NegativeOperator&) = delete; // Copy Assignment Not Implemented
  NegativeOperator& operator=(NegativeOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace nx::core
