#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/BinaryOperator.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT MultiplicationOperator : public BinaryOperator
{
public:
  using Self = MultiplicationOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new MultiplicationOperator());
  }

  ~MultiplicationOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  MultiplicationOperator();

public:
  MultiplicationOperator(const MultiplicationOperator&) = delete;            // Copy Constructor Not Implemented
  MultiplicationOperator(MultiplicationOperator&&) = delete;                 // Move Constructor Not Implemented
  MultiplicationOperator& operator=(const MultiplicationOperator&) = delete; // Copy Assignment Not Implemented
  MultiplicationOperator& operator=(MultiplicationOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace nx::core
