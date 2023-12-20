#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/CalculatorParameter.hpp"

#include <memory>
#include <stack>

namespace nx::core
{

class SIMPLNXCORE_EXPORT CalculatorOperator : public CalculatorItem
{
public:
  enum OperatorType
  {
    Unary,
    Binary
  };

  using Self = CalculatorOperator;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static double toDegrees(double radians);
  static double toRadians(double degrees);

  ~CalculatorOperator() override;

  bool hasHigherPrecedence(CalculatorOperator::Pointer other);

  virtual void calculate(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack) = 0;

  OperatorType getOperatorType();

protected:
  CalculatorOperator();

  enum Precedence
  {
    Unknown_Precedence,
    A_Precedence,
    B_Precedence,
    C_Precedence,
    D_Precedence,
    E_Precedence
  };

  double root(double base, double root);

  Precedence getPrecedence();
  void setPrecedence(Precedence precedence);

  void setOperatorType(OperatorType type);

private:
  Precedence m_Precedence = {Unknown_Precedence};
  OperatorType m_OperatorType;

public:
  CalculatorOperator(const CalculatorOperator&) = delete;            // Copy Constructor Not Implemented
  CalculatorOperator(CalculatorOperator&&) = delete;                 // Move Constructor Not Implemented
  CalculatorOperator& operator=(const CalculatorOperator&) = delete; // Copy Assignment Not Implemented
  CalculatorOperator& operator=(CalculatorOperator&&) = delete;      // Move Assignment Not Implemented

  static void CreateNewArrayTwoArguments(DataStructure& dataStructure, CalculatorParameter::AngleUnits units, DataPath calculatedArrayPath, std::stack<ICalculatorArray::Pointer>& executionStack,
                                         std::function<double(double, double)> op);
};

} // namespace nx::core
