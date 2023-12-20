#pragma once

#include "SimplnxCore/utils/CalculatorOperator.hpp"
#include "simplnx/DataStructure/DataPath.hpp"

#include <memory>
#include <stack>

namespace nx::core
{
class SIMPLNXCORE_EXPORT UnaryOperator : public CalculatorOperator
{
public:
  using Self = UnaryOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static Pointer New()
  {
    return Pointer(new UnaryOperator());
  }

  ~UnaryOperator() override;

  void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) override;

  CalculatorItem::ErrorCode checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& msg) final;

  int getNumberOfArguments();

protected:
  UnaryOperator();

  void setNumberOfArguments(int numOfArguments);

private:
  int m_NumOfArguments;

public:
  UnaryOperator(const UnaryOperator&) = delete;            // Copy Constructor Not Implemented
  UnaryOperator(UnaryOperator&&) = delete;                 // Move Constructor Not Implemented
  UnaryOperator& operator=(const UnaryOperator&) = delete; // Copy Assignment Not Implemented
  UnaryOperator& operator=(UnaryOperator&&) = delete;      // Move Assignment Not Implemented

  static void CreateNewArrayStandardUnary(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack,
                                          std::function<double(double)> op);
  static void CreateNewArrayTrig(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack,
                                 std::function<double(double)> op);
  static void CreateNewArrayArcTrig(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack,
                                    std::function<double(double)> op);
};

} // namespace nx::core
