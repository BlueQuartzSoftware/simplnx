#include "SimplnxCore/utils/ICalculatorArray.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ICalculatorArray::ICalculatorArray() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ICalculatorArray::~ICalculatorArray() = default;

// -----------------------------------------------------------------------------
ICalculatorArray::Pointer ICalculatorArray::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}
