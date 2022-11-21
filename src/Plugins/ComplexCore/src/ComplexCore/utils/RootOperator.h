#pragma once

#include <memory>
#include <stack>

#include "Core/Core_export.hpp"

#include "Core/Utilities/UnaryOperator.h"

namespace complex
{
class CalculatorNumber;

class CORE_EXPORT RootOperator : public UnaryOperator
{
public:
  using Self = RootOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new RootOperator());
  }

  ~RootOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  RootOperator();

public:
  RootOperator(const RootOperator&) = delete;            // Copy Constructor Not Implemented
  RootOperator(RootOperator&&) = delete;                 // Move Constructor Not Implemented
  RootOperator& operator=(const RootOperator&) = delete; // Copy Assignment Not Implemented
  RootOperator& operator=(RootOperator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace complex
