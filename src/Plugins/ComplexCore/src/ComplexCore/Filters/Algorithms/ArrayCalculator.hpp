#pragma once

#include "Core/Core_export.hpp"
#include "Core/Utilities/CalculatorItem.h"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/CalculatorParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

namespace complex
{

struct CORE_EXPORT ArrayCalculatorInputValues
{
  DataPath SelectedGroup;
  std::string InfixEquation;
  CalculatorParameter::AngleUnits Units;
  NumericType ScalarType;
  DataPath CalculatedArray;
};

class CORE_EXPORT ArrayCalculatorParser
{
public:
  using ParsedEquation = std::vector<CalculatorItem::Pointer>;

  ArrayCalculatorParser(const DataStructure& dataStruct, const DataPath& selectedGroupPath, const std::string& infixEquation, bool isPreflight)
  : m_DataStructure(dataStruct)
  , m_SelectedGroupPath(selectedGroupPath)
  , m_InfixEquation(infixEquation)
  , m_IsPreflight(isPreflight)
  {
    createSymbolMap();
  }

  Result<> parseInfixEquation(ParsedEquation& parsedInfix);

  static Result<ArrayCalculatorParser::ParsedEquation> ToRPN(const std::string& unparsedInfixExpression, std::vector<CalculatorItem::Pointer> infixEquation);

protected:
  std::vector<std::string> getRegularExpressionMatches();
  Result<> parseNumericValue(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix, double number);
  Result<> parseMinusSign(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix, int loopIdx);
  Result<> parseIndexOperator(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix);
  Result<> parseCommaOperator(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix);
  Result<> parseArray(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix);
  Result<> checkForAmbiguousArrayName(std::string strItem, std::string warningMsg);

private:
  const DataStructure& m_DataStructure;
  DataStructure m_TemporaryDataStructure; // data structure for holding the temporary calculator array items
  DataPath m_SelectedGroupPath;
  std::string m_InfixEquation;
  bool m_IsPreflight;

  std::map<std::string, std::shared_ptr<CalculatorItem>> m_SymbolMap;

  void createSymbolMap();
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ArrayCalculator
{
public:
  ArrayCalculator(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ArrayCalculatorInputValues* inputValues);
  ~ArrayCalculator() noexcept;

  ArrayCalculator(const ArrayCalculator&) = delete;
  ArrayCalculator(ArrayCalculator&&) noexcept = delete;
  ArrayCalculator& operator=(const ArrayCalculator&) = delete;
  ArrayCalculator& operator=(ArrayCalculator&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ArrayCalculatorInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
