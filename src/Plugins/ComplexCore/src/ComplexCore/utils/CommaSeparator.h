#pragma once

#include <memory>

#include "ComplexCore/ComplexCore_export.hpp"

#include "ComplexCore/utils/CalculatorSeparator.h"

namespace complex
{
class COMPLEXCORE_EXPORT CommaSeparator : public CalculatorSeparator
{
public:
  using Self = CommaSeparator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new CommaSeparator());
  }

  ~CommaSeparator() override;

  CalculatorItem::ErrorCode checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& msg) override;

protected:
  CommaSeparator();

public:
  CommaSeparator(const CommaSeparator&) = delete;            // Copy Constructor Not Implemented
  CommaSeparator(CommaSeparator&&) = delete;                 // Move Constructor Not Implemented
  CommaSeparator& operator=(const CommaSeparator&) = delete; // Copy Assignment Not Implemented
  CommaSeparator& operator=(CommaSeparator&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace complex
