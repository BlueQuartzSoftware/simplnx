#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/CalculatorItem.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

#include <memory>

namespace nx::core
{
class SIMPLNXCORE_EXPORT ICalculatorArray : public CalculatorItem
{
public:
  enum ValueType
  {
    Number,
    Array,
    Unknown
  };

  using Self = ICalculatorArray;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  ~ICalculatorArray() override;

  virtual Float64Array* getArray() = 0;
  virtual double getValue(int i) = 0;
  virtual void setValue(int i, double value) = 0;
  virtual ValueType getType() = 0;

  virtual Float64Array* reduceToOneComponent(int c, bool allocate = true) = 0;

protected:
  ICalculatorArray();

public:
  ICalculatorArray(const ICalculatorArray&) = delete;            // Copy Constructor Not Implemented
  ICalculatorArray(ICalculatorArray&&) = delete;                 // Move Constructor Not Implemented
  ICalculatorArray& operator=(const ICalculatorArray&) = delete; // Copy Assignment Not Implemented
  ICalculatorArray& operator=(ICalculatorArray&&) = delete;      // Move Assignment Not Implemented

private:
};
} // namespace nx::core
