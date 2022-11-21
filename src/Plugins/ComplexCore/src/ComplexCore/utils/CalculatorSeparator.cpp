#include "ComplexCore/utils/CalculatorSeparator.hpp"

using namespace complex;

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
