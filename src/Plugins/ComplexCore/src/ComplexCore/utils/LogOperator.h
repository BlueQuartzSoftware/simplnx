#pragma once

#include <memory>
#include <stack>

#include "Core/Core_export.hpp"

#include "Core/Utilities/UnaryOperator.h"

namespace complex
{
class CalculatorNumber;

class CORE_EXPORT LogOperator : public UnaryOperator
{
public:
  using Self = LogOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new LogOperator());
  }

  ~LogOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

protected:
  LogOperator();

private:
  double log_arbitrary_base(double base, double value);

public:
  LogOperator(const LogOperator&) = delete;            // Copy Constructor Not Implemented
  LogOperator(LogOperator&&) = delete;                 // Move Constructor Not Implemented
  LogOperator& operator=(const LogOperator&) = delete; // Copy Assignment Not Implemented
  LogOperator& operator=(LogOperator&&) = delete;      // Move Assignment Not Implemented
};

} // namespace complex
