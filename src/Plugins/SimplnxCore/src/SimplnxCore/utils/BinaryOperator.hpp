#pragma once

#include "SimplnxCore/utils/CalculatorOperator.hpp"

#include "simplnx/DataStructure/DataPath.hpp"

#include <memory>
#include <stack>

namespace nx::core
{

class SIMPLNXCORE_EXPORT BinaryOperator : public CalculatorOperator
{
public:
  using Self = BinaryOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new BinaryOperator());
  }

  ~BinaryOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

  CalculatorItem::ErrorCode checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& msg) final;

protected:
  BinaryOperator();

public:
  BinaryOperator(const BinaryOperator&) = delete;            // Copy Constructor Not Implemented
  BinaryOperator(BinaryOperator&&) = delete;                 // Move Constructor Not Implemented
  BinaryOperator& operator=(const BinaryOperator&) = delete; // Copy Assignment Not Implemented
  BinaryOperator& operator=(BinaryOperator&&) = delete;      // Move Assignment Not Implemented
};

} // namespace nx::core
