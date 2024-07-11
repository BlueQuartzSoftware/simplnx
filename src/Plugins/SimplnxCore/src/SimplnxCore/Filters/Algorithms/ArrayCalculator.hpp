#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/CalculatorItem.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/CalculatorParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ArrayCalculatorInputValues
{
  DataPath SelectedGroup;
  std::string InfixEquation;
  CalculatorParameter::AngleUnits Units;
  NumericType ScalarType;
  DataPath CalculatedArray;
};

class SIMPLNXCORE_EXPORT ArrayCalculatorParser
{
public:
  using ParsedEquation = std::vector<CalculatorItem::Pointer>;

  ArrayCalculatorParser(const DataStructure& dataStruct, const DataPath& selectedGroupPath, const std::string& infixEquation, bool isPreflight);

  Result<> parseInfixEquation(ParsedEquation& parsedInfix);

  static Result<ArrayCalculatorParser::ParsedEquation> ToRPN(const std::string& unparsedInfixExpression, std::vector<CalculatorItem::Pointer> infixEquation);

  friend class ArrayCalculator;

protected:
  std::vector<std::string> getRegularExpressionMatches();
  Result<> parseNumericValue(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix, double number);
  Result<> parseMinusSign(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix, int loopIdx);
  Result<> parseIndexOperator(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix);
  Result<> parseCommaOperator(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix);
  Result<> parseArray(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix);
  Result<> checkForAmbiguousArrayName(const std::string& strItem, std::string warningMsg);

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
 * @class
 */
class SIMPLNXCORE_EXPORT ArrayCalculator
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

} // namespace nx::core
