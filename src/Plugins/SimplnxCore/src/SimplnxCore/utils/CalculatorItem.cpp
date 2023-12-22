#include "SimplnxCore/utils/CalculatorItem.hpp"
#include "SimplnxCore/utils/ICalculatorArray.hpp"

#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorItem::CalculatorItem() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
CalculatorItem::~CalculatorItem() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
std::string CalculatorItem::getInfixToken()
{
  return m_InfixToken;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void CalculatorItem::setInfixToken(const std::string& token)
{
  m_InfixToken = token;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool CalculatorItem::isICalculatorArray()
{
  return (nullptr != dynamic_cast<ICalculatorArray*>(this));
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool CalculatorItem::isArray()
{
  ICalculatorArray* calculatorArray = dynamic_cast<ICalculatorArray*>(this);
  if(calculatorArray != nullptr)
  {
    return (calculatorArray->getType() == ICalculatorArray::ValueType::Array);
  }

  return false;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool CalculatorItem::isNumber()
{
  ICalculatorArray* calculatorArray = dynamic_cast<ICalculatorArray*>(this);
  if(calculatorArray != nullptr)
  {
    return (calculatorArray->getType() == ICalculatorArray::ValueType::Number);
  }

  return false;
}

// -----------------------------------------------------------------------------
CalculatorItem::Pointer CalculatorItem::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
DataPath CalculatorItem::GetUniquePathName(const DataStructure& dataStructure, DataPath path)
{
  if(dataStructure.containsData(path))
  {
    auto calculatedPathVector = path.getPathVector();
    auto& targetName = calculatedPathVector.back();
    targetName = targetName + StringUtilities::number(dataStructure.getSize());
    path = DataPath(calculatedPathVector);
  }
  return path;
}
