#pragma once

#include <memory>
#include <stack>

#include "Core/Core_export.hpp"

#include "Core/Utilities/BinaryOperator.h"

namespace complex
{
class CORE_EXPORT SubtractionOperator : public BinaryOperator
{
public:
  using Self = SubtractionOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new SubtractionOperator());
  }

  ~SubtractionOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  SubtractionOperator();

public:
  SubtractionOperator(const SubtractionOperator&) = delete;            // Copy Constructor Not Implemented
  SubtractionOperator(SubtractionOperator&&) = delete;                 // Move Constructor Not Implemented
  SubtractionOperator& operator=(const SubtractionOperator&) = delete; // Copy Assignment Not Implemented
  SubtractionOperator& operator=(SubtractionOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace complex
