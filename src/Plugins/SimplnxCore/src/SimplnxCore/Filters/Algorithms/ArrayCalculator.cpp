#include "ArrayCalculator.hpp"

#include "simplnx/Common/Result.hpp"

using namespace nx::core;

// ---------------------------------------------------------------------------
// getOperatorRegistry  (stub -- returns empty table)
// ---------------------------------------------------------------------------
const std::vector<OperatorDef>& nx::core::getOperatorRegistry()
{
  static const std::vector<OperatorDef> s_Registry;
  return s_Registry;
}

// ---------------------------------------------------------------------------
// ArrayCalculatorParser
// ---------------------------------------------------------------------------
ArrayCalculatorParser::ArrayCalculatorParser(const DataStructure& dataStructure, const DataPath& selectedGroupPath, const std::string& infixEquation, bool isPreflight)
: m_DataStructure(dataStructure)
, m_SelectedGroupPath(selectedGroupPath)
, m_InfixEquation(infixEquation)
, m_IsPreflight(isPreflight)
{
}

// ---------------------------------------------------------------------------
std::vector<Token> ArrayCalculatorParser::tokenize(const std::string& equation)
{
  std::vector<Token> tokens;
  const size_t len = equation.size();
  size_t i = 0;

  while(i < len)
  {
    const char c = equation[i];

    // 1. Skip whitespace
    if(std::isspace(static_cast<unsigned char>(c)))
    {
      ++i;
      continue;
    }

    // 2. Numbers: starts with digit, or dot followed by a digit
    if(std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && i + 1 < len && std::isdigit(static_cast<unsigned char>(equation[i + 1]))))
    {
      size_t start = i;
      bool hasDot = false;
      while(i < len && (std::isdigit(static_cast<unsigned char>(equation[i])) || equation[i] == '.'))
      {
        if(equation[i] == '.')
        {
          if(hasDot)
          {
            break;
          }
          hasDot = true;
        }
        ++i;
      }
      tokens.push_back({TokenType::Number, equation.substr(start, i - start), start});
      continue;
    }

    // 3. Identifiers: start with letter or underscore
    if(std::isalpha(static_cast<unsigned char>(c)) || c == '_')
    {
      size_t start = i;
      while(i < len && (std::isalnum(static_cast<unsigned char>(equation[i])) || equation[i] == '_'))
      {
        ++i;
      }
      tokens.push_back({TokenType::Identifier, equation.substr(start, i - start), start});
      continue;
    }

    // 4. Quoted strings
    if(c == '"')
    {
      size_t start = i;
      ++i; // skip opening quote
      size_t contentStart = i;
      while(i < len && equation[i] != '"')
      {
        ++i;
      }
      std::string content = equation.substr(contentStart, i - contentStart);
      if(i < len)
      {
        ++i; // skip closing quote
      }
      tokens.push_back({TokenType::QuotedString, content, start});
      continue;
    }

    // 5. Single-character operators
    TokenType opType;
    bool isOperator = true;
    switch(c)
    {
    case '+':
      opType = TokenType::Plus;
      break;
    case '-':
      opType = TokenType::Minus;
      break;
    case '*':
      opType = TokenType::Star;
      break;
    case '/':
      opType = TokenType::Slash;
      break;
    case '^':
      opType = TokenType::Caret;
      break;
    case '%':
      opType = TokenType::Percent;
      break;
    case '(':
      opType = TokenType::LParen;
      break;
    case ')':
      opType = TokenType::RParen;
      break;
    case '[':
      opType = TokenType::LBracket;
      break;
    case ']':
      opType = TokenType::RBracket;
      break;
    case ',':
      opType = TokenType::Comma;
      break;
    default:
      isOperator = false;
      break;
    }

    if(isOperator)
    {
      tokens.push_back({opType, std::string(1, c), i});
      ++i;
      continue;
    }

    // 6. Unknown characters: produce an Identifier token
    tokens.push_back({TokenType::Identifier, std::string(1, c), i});
    ++i;
  }

  return tokens;
}

// ---------------------------------------------------------------------------
Result<> ArrayCalculatorParser::parseAndValidate(std::vector<usize>& /*outTupleShape*/, std::vector<usize>& /*outComponentShape*/)
{
  return MakeErrorResult(-1, "Not yet implemented");
}

// ---------------------------------------------------------------------------
Result<> ArrayCalculatorParser::evaluateInto(DataStructure& /*dataStructure*/, const DataPath& /*outputPath*/, NumericType /*scalarType*/, CalculatorParameter::AngleUnits /*units*/)
{
  return MakeErrorResult(-1, "Not yet implemented");
}

// ---------------------------------------------------------------------------
// ArrayCalculator
// ---------------------------------------------------------------------------
ArrayCalculator::ArrayCalculator(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ArrayCalculatorInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// ---------------------------------------------------------------------------
ArrayCalculator::~ArrayCalculator() noexcept = default;

// ---------------------------------------------------------------------------
const std::atomic_bool& ArrayCalculator::getCancel()
{
  return m_ShouldCancel;
}

// ---------------------------------------------------------------------------
Result<> ArrayCalculator::operator()()
{
  return MakeErrorResult(-1, "Not yet implemented");
}
