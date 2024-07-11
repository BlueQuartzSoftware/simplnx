#include "ArrayCalculator.hpp"

#include "SimplnxCore/utils/ABSOperator.hpp"
#include "SimplnxCore/utils/ACosOperator.hpp"
#include "SimplnxCore/utils/ASinOperator.hpp"
#include "SimplnxCore/utils/ATanOperator.hpp"
#include "SimplnxCore/utils/AdditionOperator.hpp"
#include "SimplnxCore/utils/CalculatorArray.hpp"
#include "SimplnxCore/utils/CalculatorOperator.hpp"
#include "SimplnxCore/utils/CeilOperator.hpp"
#include "SimplnxCore/utils/CommaSeparator.hpp"
#include "SimplnxCore/utils/CosOperator.hpp"
#include "SimplnxCore/utils/DivisionOperator.hpp"
#include "SimplnxCore/utils/ExpOperator.hpp"
#include "SimplnxCore/utils/FloorOperator.hpp"
#include "SimplnxCore/utils/LeftParenthesisItem.hpp"
#include "SimplnxCore/utils/LnOperator.hpp"
#include "SimplnxCore/utils/Log10Operator.hpp"
#include "SimplnxCore/utils/LogOperator.hpp"
#include "SimplnxCore/utils/MultiplicationOperator.hpp"
#include "SimplnxCore/utils/NegativeOperator.hpp"
#include "SimplnxCore/utils/PowOperator.hpp"
#include "SimplnxCore/utils/RightParenthesisItem.hpp"
#include "SimplnxCore/utils/RootOperator.hpp"
#include "SimplnxCore/utils/SinOperator.hpp"
#include "SimplnxCore/utils/SqrtOperator.hpp"
#include "SimplnxCore/utils/SubtractionOperator.hpp"
#include "SimplnxCore/utils/TanOperator.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <regex>

using namespace nx::core;

namespace
{
struct CreateCalculatorArrayFunctor
{
  template <typename T>
  CalculatorItem::Pointer operator()(DataStructure& dataStructure, bool allocate, const IDataArray* iDataArrayPtr)
  {
    const auto* inputDataArray = dynamic_cast<const DataArray<T>*>(iDataArrayPtr);
    CalculatorItem::Pointer itemPtr = CalculatorArray<T>::New(dataStructure, inputDataArray, ICalculatorArray::Array, allocate);
    return itemPtr;
  }
};

struct CopyArrayFunctor
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& calculatedArrayPath, const Float64Array* inputArray)
  {
    const auto& inputDataStore = inputArray->getDataStoreRef();
    if(nullptr != inputArray)
    {
      auto& convertedDataStore = dataStructure.getDataAsUnsafe<DataArray<T>>(calculatedArrayPath)->getDataStoreRef();

      const usize count = inputDataStore.getSize();
      for(usize i = 0; i < count; i++)
      {
        if constexpr(std::is_same_v<float64, T>)
        {
          convertedDataStore[i] = inputDataStore[i];
        }
        else
        {
          convertedDataStore[i] = static_cast<T>(inputDataStore[i]);
        }
      }
    }
  }
};

struct InitializeArrayFunctor
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& calculatedArrayPath, const Float64Array* inputArray)
  {
    auto& convertedDataStore = dataStructure.getDataAsUnsafe<DataArray<T>>(calculatedArrayPath)->getDataStoreRef();
    if(nullptr != inputArray && inputArray->getSize() == 1)
    {
      if constexpr(std::is_same_v<float64, T>)
      {
        convertedDataStore.fill(inputArray->at(0));
      }
      else
      {
        convertedDataStore.fill(static_cast<T>(inputArray->at(0)));
      }
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
ArrayCalculator::ArrayCalculator(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ArrayCalculatorInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ArrayCalculator::~ArrayCalculator() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ArrayCalculator::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ArrayCalculator::operator()()
{
  Result<> results;

  // Parse the infix expression from the user interface
  ArrayCalculatorParser parser(m_DataStructure, m_InputValues->SelectedGroup, m_InputValues->InfixEquation, false);
  std::vector<CalculatorItem::Pointer> parsedInfix;
  Result<> parsedEquationResults = parser.parseInfixEquation(parsedInfix);
  results.warnings() = parsedEquationResults.warnings();
  if(parsedEquationResults.invalid())
  {
    results.errors() = parsedEquationResults.errors();
    return results;
  }
  if(parsedInfix.empty())
  {
    results.errors().push_back(Error{-6550, "Error while parsing infix expression."});
    return results;
  }

  // Convert the parsed infix expression into RPN
  Result<ArrayCalculatorParser::ParsedEquation> rpnResults = ArrayCalculatorParser::ToRPN(m_InputValues->InfixEquation, parsedInfix);
  std::vector<CalculatorItem::Pointer> rpn = rpnResults.value();
  if(rpnResults.invalid() || rpn.empty())
  {
    results.errors().push_back(Error{-6551, "Error while converting parsed infix expression to postfix notation"});
    return results;
  }

  // Execute the RPN expression
  DataPath temporaryCalculatedArrayPath = DataPath({m_InputValues->CalculatedArray.getTargetName() + "_TEMPORARY"});
  std::stack<ICalculatorArray::Pointer> executionStack;
  int totalItems = rpn.size();
  for(int rpnCount = 0; rpnCount < totalItems; rpnCount++)
  {
    m_MessageHandler({IFilter::Message::Type::Info, "Computing Operator " + StringUtilities::number(rpnCount + 1) + "/" + StringUtilities::number(totalItems)});

    CalculatorItem::Pointer rpnItem = rpn[rpnCount];
    ICalculatorArray::Pointer calcArray = std::dynamic_pointer_cast<ICalculatorArray>(rpnItem);
    if(nullptr != calcArray)
    {
      // This is an array
      executionStack.push(calcArray);
    }
    else
    {
      // This is an operator
      CalculatorOperator::Pointer rpnOperator = std::dynamic_pointer_cast<CalculatorOperator>(rpnItem);

      rpnOperator->calculate(parser.m_TemporaryDataStructure, m_InputValues->Units, temporaryCalculatedArrayPath, executionStack);
    }

    if(getCancel())
    {
      return results;
    }
  }

  // Grab the result from the stack
  ICalculatorArray::Pointer arrayItem = ICalculatorArray::NullPointer();
  if(executionStack.size() != 1)
  {
    results.errors().push_back(Error{static_cast<int>(CalculatorItem::ErrorCode::InvalidEquation), "The chosen infix equation is not a valid equation."});
    return results;
  }
  if(!executionStack.empty())
  {
    arrayItem = executionStack.top();
    executionStack.pop();
  }

  if(arrayItem != ICalculatorArray::NullPointer())
  {
    const Float64Array* resultArray = arrayItem->getArray();
    if(arrayItem->isNumber() && m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->CalculatedArray.getParent()) != nullptr)
    {
      ExecuteDataFunction(InitializeArrayFunctor{}, ConvertNumericTypeToDataType(m_InputValues->ScalarType), m_DataStructure, m_InputValues->CalculatedArray, resultArray);
    }
    else
    {
      ExecuteDataFunction(CopyArrayFunctor{}, ConvertNumericTypeToDataType(m_InputValues->ScalarType), m_DataStructure, m_InputValues->CalculatedArray, resultArray);
    }
  }
  else
  {
    results.errors().push_back(Error{static_cast<int>(CalculatorItem::ErrorCode::UnexpectedOutput), "Unexpected output item from chosen infix expression; the output item must be an array\n"
                                                                                                    "Please contact the DREAM.3D developers for more information"});
    return results;
  }

  return {};
}

ArrayCalculatorParser::ArrayCalculatorParser(const DataStructure& dataStruct, const DataPath& selectedGroupPath, const std::string& infixEquation, bool isPreflight)
: m_DataStructure(dataStruct)
, m_SelectedGroupPath(selectedGroupPath)
, m_InfixEquation(infixEquation)
, m_IsPreflight(isPreflight)
{
  createSymbolMap();
}

// -----------------------------------------------------------------------------
Result<> ArrayCalculatorParser::parseInfixEquation(ParsedEquation& parsedInfix)
{
  parsedInfix.clear();
  Result<> results = {};
  std::vector<std::string> itemList = getRegularExpressionMatches();

  // Iterate through the QStringList and create the proper CalculatorItems
  for(int i = 0; i < itemList.size(); i++)
  {
    std::string strItem = itemList[i];
    CalculatorItem::Pointer itemPtr = nullptr;

    bool ok = true;
    double num;
    try
    {
      num = std::stod(strItem);
    } catch(std::exception& e)
    {
      ok = false;
    }
    if(ok)
    {
      // This is a numeric value
      auto parsedNumericResults = parseNumericValue(strItem, parsedInfix, num);
      results = MergeResults(results, parsedNumericResults);
      if(results.invalid())
      {
        parsedInfix.clear();
        return results;
      }
    }
    else if(strItem == "-")
    {
      // This is a minus sign
      auto parsedMinusResults = parseMinusSign(strItem, parsedInfix, i);
      results = MergeResults(results, parsedMinusResults);
      if(results.invalid())
      {
        parsedInfix.clear();
        return results;
      }
    }
    else if(StringUtilities::contains(strItem, "[") && StringUtilities::contains(strItem, "]"))
    {
      // This is an index operator
      auto parsedIndexResults = parseIndexOperator(strItem, parsedInfix);
      results = MergeResults(results, parsedIndexResults);
      if(results.invalid())
      {
        parsedInfix.clear();
        return results;
      }
    }
    else
    {
      if(m_SymbolMap.find(strItem) != m_SymbolMap.end())
      {
        itemPtr = m_SymbolMap[strItem];
      }

      if(nullptr != std::dynamic_pointer_cast<CommaSeparator>(itemPtr))
      {
        // This is a comma operator
        auto parsedCommaResults = parseCommaOperator(strItem, parsedInfix);
        results = MergeResults(results, parsedCommaResults);
        if(results.invalid())
        {
          parsedInfix.clear();
          return results;
        }
      }
      else if(nullptr != itemPtr)
      {
        // This is another type of operator
        std::string ss = fmt::format("Item '{}' in the infix expression is the name of an array in the selected Attribute Matrix, but it is currently being used as a mathematical operator", strItem);
        auto checkNameResults = checkForAmbiguousArrayName(strItem, ss);
        results.warnings() = checkNameResults.warnings();

        parsedInfix.push_back(itemPtr);
      }
      // It doesn't matter which path we use for the selected attribute matrix since we are only checking the target names
      else if(ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, strItem) || (!strItem.empty() && strItem[0] == '\"' && strItem[strItem.size() - 1] == '\"'))
      {
        auto parsedArrayResults = parseArray(strItem, parsedInfix);
        results = MergeResults(results, parsedArrayResults);
        if(results.invalid())
        {
          parsedInfix.clear();
          return results;
        }
      }
      else
      {
        parsedInfix.clear();
        std::string ss = fmt::format("An unrecognized or invalid item '{}' was found in the chosen infix expression", strItem);
        return MakeErrorResult(static_cast<int>(CalculatorItem::ErrorCode::UnrecognizedItem), ss);
      }
    }
  }

  // Return the parsed infix expression as a vector of CalculatorItems
  return results;
}

// -----------------------------------------------------------------------------
std::vector<std::string> ArrayCalculatorParser::getRegularExpressionMatches()
{
  // Parse all the items into a QVector of strings using a regular expression
  std::vector<std::string> itemList;
  // Match all array names that start with two alphabetical characters and have spaces.  Match all numbers, decimal or integer.
  // Match one letter array names.  Match all special character operators.
  std::regex regExp(R"lit(("((\[)?\d+(\.\d+)?(\])?|(\[)?\.\d+(\])?|\w{1,1}((\w|\s|\d)*(\w|\d){1,1})?|\S)")|(((\[)?\d+(\.\d+)?(\])?|(\[)?\.\d+(\])?|\w{1,1}((\w|\s|\d)*(\w|\d){1,1})?|\S)))lit");

  auto regExpMatchBegin = std::sregex_iterator(m_InfixEquation.begin(), m_InfixEquation.end(), regExp);
  auto regExpMatchEnd = std::sregex_iterator();
  for(std::sregex_iterator i = regExpMatchBegin; i != regExpMatchEnd; ++i)
  {
    const std::smatch& match = *i;
    itemList.push_back(match.str());
  }

  return itemList;
}

// -----------------------------------------------------------------------------
Result<> ArrayCalculatorParser::parseNumericValue(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix, double number)
{
  // This is a number, so create an array with numOfTuples equal to 1 and set the value into it
  Float64Array* ptr = Float64Array::CreateWithStore<Float64DataStore>(m_TemporaryDataStructure, "INTERNAL_USE_ONLY_NumberArray" + StringUtilities::number(m_TemporaryDataStructure.getSize()),
                                                                      std::vector<size_t>{1}, std::vector<size_t>{1});
  (*ptr)[0] = number;
  CalculatorItem::Pointer itemPtr = CalculatorArray<float64>::New(m_TemporaryDataStructure, ptr, ICalculatorArray::Number, !m_IsPreflight);
  parsedInfix.push_back(itemPtr);

  std::string ss = fmt::format("Item '{}' in the infix expression is the name of an array in the selected Attribute Matrix, but it is currently being used as a number", token);
  return checkForAmbiguousArrayName(token, ss);
}

// -----------------------------------------------------------------------------
Result<> ArrayCalculatorParser::parseMinusSign(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix, int loopIdx)
{
  CalculatorItem::Pointer itemPtr = nullptr;

  // This could be either a negative sign or subtraction sign, so we need to figure out which one it is
  if(loopIdx == 0 || (((nullptr != std::dynamic_pointer_cast<CalculatorOperator>(parsedInfix.back()) &&
                        std::dynamic_pointer_cast<CalculatorOperator>(parsedInfix.back())->getOperatorType() == CalculatorOperator::Binary) ||
                       nullptr != std::dynamic_pointer_cast<LeftParenthesisItem>(parsedInfix.back())) &&
                      nullptr == std::dynamic_pointer_cast<RightParenthesisItem>(parsedInfix.back())))
  {
    // By context, this is a negative sign
    itemPtr = NegativeOperator::New();
    parsedInfix.push_back(itemPtr);
  }
  else
  {
    // By context, this is a subtraction sign
    if(m_SymbolMap.find(token) != m_SymbolMap.end())
    {
      itemPtr = m_SymbolMap[token];
    }
    parsedInfix.push_back(itemPtr);
  }

  std::string ss = fmt::format("Item '{}' in the infix expression is the name of an array in the selected attribute matrix, but it is currently being used as a mathematical operator", token);
  return checkForAmbiguousArrayName(token, ss);
}

// -----------------------------------------------------------------------------
Result<> ArrayCalculatorParser::parseIndexOperator(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix)
{
  int idx = parsedInfix.size() - 1;

  std::string errorMsg = fmt::format("Index operator '{}' is not paired with a valid array name.", token);
  int errCode = static_cast<int>(CalculatorItem::ErrorCode::OrphanedComponent);
  if(idx < 0)
  {
    return MakeErrorResult(errCode, errorMsg);
  }
  if(!parsedInfix[idx]->isICalculatorArray())
  {
    return MakeErrorResult(errCode, errorMsg);
  }
  if(parsedInfix[idx]->isNumber())
  {
    return MakeErrorResult(errCode, errorMsg);
  }

  token = StringUtilities::replace(token, "[", "");
  token = StringUtilities::replace(token, "]", "");

  int index = -1;
  try
  {
    index = std::stoi(token);
  } catch(std::exception& e)
  {
    std::string ss = "The chosen infix expression is not a valid expression";
    return MakeErrorResult(static_cast<int>(CalculatorItem::ErrorCode::InvalidComponent), ss);
  }

  ICalculatorArray::Pointer calcArray = std::dynamic_pointer_cast<ICalculatorArray>(parsedInfix.back());
  if(nullptr != calcArray && index >= calcArray->getArray()->getNumberOfComponents())
  {
    std::string ss = fmt::format("'{}' has an component index that is out of range", calcArray->getArray()->getName());
    return MakeErrorResult(static_cast<int>(CalculatorItem::ErrorCode::ComponentOutOfRange), ss);
  }

  parsedInfix.pop_back();

  Float64Array* reducedArray = calcArray->reduceToOneComponent(index, !m_IsPreflight);
  CalculatorItem::Pointer itemPtr = CalculatorArray<float64>::New(m_TemporaryDataStructure, reducedArray, ICalculatorArray::Array, !m_IsPreflight);
  parsedInfix.push_back(itemPtr);

  std::string ss = fmt::format("Item '{}' in the infix expression is the name of an array in the selected Attribute Matrix, but it is currently being used as an indexing operator", token);
  return checkForAmbiguousArrayName(token, ss);
}

// -----------------------------------------------------------------------------
Result<> ArrayCalculatorParser::parseCommaOperator(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix)
{
  std::string ss =
      fmt::format("Item '{}' in the infix expression is the name of an array in the selected Attribute Matrix, but it is currently being detected as a comma in a mathematical operator", token);
  Result<> results = checkForAmbiguousArrayName(token, ss);

  // Put parentheses around the entire term so that the RPN parser knows to evaluate the entire expression placed here
  // For example, if we have root( 4*4, 2*3 ), then we need it to be root( (4*4), (2*3) )
  parsedInfix.push_back(RightParenthesisItem::New());

  auto iter = parsedInfix.end();
  iter--;
  while(iter != parsedInfix.begin())
  {
    if(nullptr != std::dynamic_pointer_cast<CommaSeparator>(*iter) || nullptr != std::dynamic_pointer_cast<LeftParenthesisItem>(*iter))
    {
      iter++;
      parsedInfix.insert(iter, LeftParenthesisItem::New());
      break;
    }

    iter--;
  }

  CalculatorItem::Pointer itemPtr = nullptr;
  if(m_SymbolMap.find(token) != m_SymbolMap.end())
  {
    itemPtr = m_SymbolMap[token];
  }
  parsedInfix.push_back(itemPtr);
  return results;
}

// -----------------------------------------------------------------------------
Result<> ArrayCalculatorParser::parseArray(std::string token, std::vector<CalculatorItem::Pointer>& parsedInfix)
{
  int firstArray_NumTuples = -1;
  std::string firstArray_Name = "";

  token = StringUtilities::replace(token, "\"", "");
  if(!ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, token))
  {
    std::string ss = fmt::format("The item '{}' is not the name of any valid array in the selected Attribute Matrix", token);
    return MakeErrorResult(static_cast<int>(CalculatorItem::ErrorCode::InvalidArrayName), ss);
  }

  DataPath tokenArrayPath = m_SelectedGroupPath.empty() ? DataPath({token}) : m_SelectedGroupPath.createChildPath(token);
  const auto* dataArray = m_DataStructure.getDataAs<IDataArray>(tokenArrayPath);
  if(firstArray_NumTuples < 0 && firstArray_Name.empty())
  {
    firstArray_NumTuples = dataArray->getNumberOfTuples();
    firstArray_Name = dataArray->getName();
  }
  else if(dataArray->getNumberOfTuples() != firstArray_NumTuples)
  {
    std::string ss = fmt::format("Arrays '{}' and '{}' in the infix expression have an inconsistent number of tuples", firstArray_Name, dataArray->getName());
    return MakeErrorResult(static_cast<int>(CalculatorItem::ErrorCode::InconsistentTuples), ss);
  }

  CalculatorItem::Pointer itemPtr = ExecuteDataFunction(CreateCalculatorArrayFunctor{}, dataArray->getDataType(), m_TemporaryDataStructure, !m_IsPreflight, dataArray);
  parsedInfix.push_back(itemPtr);
  return {};
}

// -----------------------------------------------------------------------------
Result<> ArrayCalculatorParser::checkForAmbiguousArrayName(const std::string& strItem, std::string warningMsg)
{
  if(m_IsPreflight && ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, strItem))
  {
    warningMsg.append("\nTo treat this item as an array name, please add double quotes around the item (i.e. \"" + strItem + "\").");
    return MakeWarningVoidResult(static_cast<int>(CalculatorItem::WarningCode::AmbiguousNameWarning), warningMsg);
  }
  return {};
}

// -----------------------------------------------------------------------------
void ArrayCalculatorParser::createSymbolMap()
{
  // Insert all items into the symbol map to use during expression parsing
  {
    LeftParenthesisItem::Pointer symbol = LeftParenthesisItem::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    RightParenthesisItem::Pointer symbol = RightParenthesisItem::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    CommaSeparator::Pointer symbol = CommaSeparator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    AdditionOperator::Pointer symbol = AdditionOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    SubtractionOperator::Pointer symbol = SubtractionOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    MultiplicationOperator::Pointer symbol = MultiplicationOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    DivisionOperator::Pointer symbol = DivisionOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    PowOperator::Pointer symbol = PowOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    ABSOperator::Pointer symbol = ABSOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    SinOperator::Pointer symbol = SinOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    CosOperator::Pointer symbol = CosOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    TanOperator::Pointer symbol = TanOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    ASinOperator::Pointer symbol = ASinOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    ACosOperator::Pointer symbol = ACosOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    ATanOperator::Pointer symbol = ATanOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    SqrtOperator::Pointer symbol = SqrtOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    RootOperator::Pointer symbol = RootOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    Log10Operator::Pointer symbol = Log10Operator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    LogOperator::Pointer symbol = LogOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    ExpOperator::Pointer symbol = ExpOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    LnOperator::Pointer symbol = LnOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    FloorOperator::Pointer symbol = FloorOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
  {
    CeilOperator::Pointer symbol = CeilOperator::New();
    m_SymbolMap[symbol->getInfixToken()] = symbol;
  }
}

// -----------------------------------------------------------------------------
Result<ArrayCalculatorParser::ParsedEquation> ArrayCalculatorParser::ToRPN(const std::string& unparsedInfixExpression, std::vector<CalculatorItem::Pointer> infixEquation)
{
  std::stack<CalculatorItem::Pointer> itemStack;
  std::vector<CalculatorItem::Pointer> rpnEquation;

  bool* oneComponent = nullptr;

  // Iterate through the infix expression items
  for(int i = 0; i < infixEquation.size(); i++)
  {
    CalculatorItem::Pointer calcItem = infixEquation[i];
    if(nullptr != std::dynamic_pointer_cast<ICalculatorArray>(calcItem))
    {
      ICalculatorArray::Pointer arrayItem = std::dynamic_pointer_cast<ICalculatorArray>(calcItem);

      // This is a number or array, so push it onto the rpn expression output
      rpnEquation.push_back(arrayItem);
    }
    else if(nullptr != std::dynamic_pointer_cast<LeftParenthesisItem>(calcItem))
    {
      // This is a left parenthesis, so push it onto the item stack
      itemStack.push(calcItem);
    }
    else if(nullptr != std::dynamic_pointer_cast<RightParenthesisItem>(calcItem))
    {
      // This is a right parenthesis, so push operators from the item stack onto the rpn expression output until we get to the left parenthesis
      while(!itemStack.empty() && nullptr == std::dynamic_pointer_cast<LeftParenthesisItem>(itemStack.top()))
      {
        rpnEquation.push_back(itemStack.top());
        itemStack.pop();
      }

      // Discard the left parenthesis that we found
      itemStack.pop();
    }
    else if(nullptr != std::dynamic_pointer_cast<CalculatorSeparator>(calcItem))
    {
      // This is a comma, so we want to continue without adding it to anything
      continue;
    }
    else
    {
      // This is an operator
      CalculatorOperator::Pointer incomingOperator = std::dynamic_pointer_cast<CalculatorOperator>(calcItem);

      if(!itemStack.empty())
      {
        /* If the operator's precedence is lower than the precedence of the operator on top of the item stack, push the operator at the top
           of the item stack onto the rpn expression output.  Keeping doing this until there isn't another operator at the top of the item
           stack or the operator has a higher precedence than the one currently on top of the stack */
        CalculatorOperator::Pointer topOperator = std::dynamic_pointer_cast<CalculatorOperator>(itemStack.top());
        while(nullptr != topOperator && !incomingOperator->hasHigherPrecedence(topOperator))
        {
          rpnEquation.push_back(itemStack.top());
          itemStack.pop();
          if(!itemStack.empty())
          {
            topOperator = std::dynamic_pointer_cast<CalculatorOperator>(itemStack.top());
          }
          else
          {
            topOperator = nullptr;
          }
        }
      }

      // Push the operator onto the rpn expression output.
      itemStack.push(calcItem);
    }
  }

  /* After we are done iterating through the infix expression items, keep transferring items from the item stack to the
     rpn expression output until the stack is empty. */
  while(!itemStack.empty())
  {
    CalculatorItem::Pointer item = itemStack.top();
    itemStack.pop();
    if(nullptr != std::dynamic_pointer_cast<LeftParenthesisItem>(item))
    {
      std::string ss = fmt::format("One or more parentheses are mismatched in the chosen infix expression '{}'", unparsedInfixExpression);
      return MakeErrorResult<ParsedEquation>(static_cast<int>(CalculatorItem::ErrorCode::MismatchedParentheses), ss);
    }

    rpnEquation.push_back(item);
  }

  delete oneComponent;

  return {std::move(rpnEquation)};
}
