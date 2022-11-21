#pragma once

#include <memory>

#include "Core/Core_export.hpp"

#include "Core/Utilities/CalculatorItem.h"

namespace complex
{
class CORE_EXPORT CalculatorSeparator : public CalculatorItem
{
public:
  using Self = CalculatorSeparator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  ~CalculatorSeparator() override;

protected:
  CalculatorSeparator();

public:
  CalculatorSeparator(const CalculatorSeparator&) = delete;            // Copy Constructor Not Implemented
  CalculatorSeparator(CalculatorSeparator&&) = delete;                 // Move Constructor Not Implemented
  CalculatorSeparator& operator=(const CalculatorSeparator&) = delete; // Copy Assignment Not Implemented
  CalculatorSeparator& operator=(CalculatorSeparator&&) = delete;      // Move Assignment Not Implemented

private:
};

} // namespace complex
