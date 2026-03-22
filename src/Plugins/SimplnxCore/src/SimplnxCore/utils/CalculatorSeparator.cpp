#include "SimplnxCore/utils/CalculatorSeparator.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorSeparator::CalculatorSeparator() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorSeparator::~CalculatorSeparator() = default;

// -----------------------------------------------------------------------------
CalculatorSeparator::Pointer CalculatorSeparator::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
