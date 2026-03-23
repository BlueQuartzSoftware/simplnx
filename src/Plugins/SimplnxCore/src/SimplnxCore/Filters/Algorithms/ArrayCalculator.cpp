#include "ArrayCalculator.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stack>

using namespace nx::core;

// ===========================================================================
// Anonymous namespace: ParsedItem, helper functions, functors
// ===========================================================================
namespace
{

// ---------------------------------------------------------------------------
// Intermediate representation used between parsing and shunting-yard.
// Includes parentheses and commas that the final RpnItem list does not.
// ---------------------------------------------------------------------------
struct ParsedItem
{
  enum class Kind
  {
    Value,
    Operator,
    LParen,
    RParen,
    Comma,
    ComponentExtract
  } kind;

  CalcValue value{CalcValue::Kind::Number, 0};
  const OperatorDef* op = nullptr;
  usize componentIndex = std::numeric_limits<usize>::max();
  bool isNegativePrefix = false;
};

// ---------------------------------------------------------------------------
// The static unary negative OperatorDef (not in the registry)
// ---------------------------------------------------------------------------
const OperatorDef& getUnaryNegativeOp()
{
  static const OperatorDef s_UnaryNeg = {"neg", OperatorDef::UnaryPrefix, 4, 1, OperatorDef::Right, OperatorDef::None, [](double x) { return -x; }, nullptr};
  return s_UnaryNeg;
}

// ---------------------------------------------------------------------------
// Look up an operator/function in the registry by exact token match
// ---------------------------------------------------------------------------
const OperatorDef* findOperatorByToken(const std::string& token)
{
  const auto& registry = getOperatorRegistry();
  for(const auto& opDef : registry)
  {
    if(opDef.token == token)
    {
      return &opDef;
    }
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// Map single-character TokenType to the operator registry token string
// ---------------------------------------------------------------------------
const OperatorDef* operatorDefForSymbolToken(TokenType type)
{
  switch(type)
  {
  case TokenType::Plus:
    return findOperatorByToken("+");
  case TokenType::Minus:
    return findOperatorByToken("-");
  case TokenType::Star:
    return findOperatorByToken("*");
  case TokenType::Slash:
    return findOperatorByToken("/");
  case TokenType::Caret:
    return findOperatorByToken("^");
  case TokenType::Percent:
    return findOperatorByToken("%");
  default:
    return nullptr;
  }
}

// ---------------------------------------------------------------------------
// Search the entire DataStructure for IDataArrays with a given name.
// Returns all DataPaths where a matching array is found.
// ---------------------------------------------------------------------------
std::vector<DataPath> findArraysByName(const DataStructure& ds, const std::string& name)
{
  std::vector<DataPath> results;

  // Search recursively from the root
  auto allPaths = GetAllChildDataPathsRecursive(ds, DataPath{}, DataObject::Type::DataArray);
  if(allPaths.has_value())
  {
    for(const auto& path : allPaths.value())
    {
      if(path.getTargetName() == name)
      {
        results.push_back(path);
      }
    }
  }

  return results;
}

// ---------------------------------------------------------------------------
// Functor to copy an IDataArray of any numeric type into a Float64Array
// ---------------------------------------------------------------------------
struct CopyToFloat64Functor
{
  template <typename T>
  void operator()(const IDataArray& sourceArray, Float64Array& destArray)
  {
    const auto& typedSource = dynamic_cast<const DataArray<T>&>(sourceArray);
    const usize totalElements = typedSource.getSize();
    for(usize i = 0; i < totalElements; i++)
    {
      destArray[i] = static_cast<double>(typedSource.at(i));
    }
  }
};

// ---------------------------------------------------------------------------
// Check whether the previous ParsedItem is a binary operator
// ---------------------------------------------------------------------------
bool isBinaryOp(const ParsedItem& item)
{
  return item.kind == ParsedItem::Kind::Operator && item.op != nullptr && item.op->kind == OperatorDef::BinaryInfix && !item.isNegativePrefix;
}

// ---------------------------------------------------------------------------
// WrapFunctionArguments: for each function call in the parsed item list,
// wrap each comma-separated argument in extra parentheses so that the
// shunting-yard algorithm processes them correctly.
//
// Example: sin(a + b) stays as sin((a + b))
//          log(a, b)  becomes log((a), (b))
// ---------------------------------------------------------------------------
void wrapFunctionArguments(std::vector<ParsedItem>& items)
{
  std::vector<ParsedItem> out;
  out.reserve(items.size() * 2);

  for(size_t i = 0; i < items.size(); ++i)
  {
    const auto& item = items[i];

    // Detect: Function operator followed by LParen
    if(item.kind == ParsedItem::Kind::Operator && item.op != nullptr && item.op->kind == OperatorDef::Function && i + 1 < items.size() && items[i + 1].kind == ParsedItem::Kind::LParen)
    {
      // Copy function and '('
      out.push_back(item);
      out.push_back(items[++i]);
      int depth = 1;
      size_t argStart = out.size();

      // Process until matching ')'
      for(++i; i < items.size() && depth > 0; ++i)
      {
        const auto& cur = items[i];
        if(cur.kind == ParsedItem::Kind::LParen)
        {
          ++depth;
          out.push_back(cur);
        }
        else if(cur.kind == ParsedItem::Kind::RParen)
        {
          --depth;
          if(depth == 0)
          {
            // Close last argument with wrapping parens
            ParsedItem lp;
            lp.kind = ParsedItem::Kind::LParen;
            out.insert(out.begin() + static_cast<std::ptrdiff_t>(argStart), lp);

            ParsedItem rp;
            rp.kind = ParsedItem::Kind::RParen;
            out.push_back(rp);
            break;
          }
          out.push_back(cur);
        }
        else if(cur.kind == ParsedItem::Kind::Comma && depth == 1)
        {
          // End this argument, copy comma, start next argument
          ParsedItem rp;
          rp.kind = ParsedItem::Kind::RParen;
          out.push_back(rp);

          out.push_back(cur);

          ParsedItem lp;
          lp.kind = ParsedItem::Kind::LParen;
          out.push_back(lp);
        }
        else
        {
          out.push_back(cur);
        }
      }
      --i; // we consumed the ')' in the inner loop
    }
    else
    {
      out.push_back(item);
    }
  }

  items.swap(out);
}

// ---------------------------------------------------------------------------
// Functor to copy a Float64Array result into the output DataArray of any
// numeric type, performing static_cast on each element.
// ---------------------------------------------------------------------------
struct CopyResultFunctor
{
  template <typename T>
  void operator()(DataStructure& ds, const DataPath& outputPath, const Float64Array* resultArray, bool isScalar)
  {
    auto& output = ds.getDataRefAs<DataArray<T>>(outputPath).getDataStoreRef();
    if(isScalar && resultArray->getSize() == 1)
    {
      output.fill(static_cast<T>(resultArray->at(0)));
    }
    else
    {
      for(usize i = 0; i < output.getSize(); i++)
      {
        output[i] = static_cast<T>(resultArray->at(i));
      }
    }
  }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// getOperatorRegistry
// ---------------------------------------------------------------------------
const std::vector<OperatorDef>& nx::core::getOperatorRegistry()
{
  static const std::vector<OperatorDef> s_Registry = []() {
    std::vector<OperatorDef> reg;
    reg.reserve(23);

    // ---- Binary infix operators ----
    reg.push_back({"+", OperatorDef::BinaryInfix, 1, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return a + b; }});
    reg.push_back({"-", OperatorDef::BinaryInfix, 1, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return a - b; }});
    reg.push_back({"*", OperatorDef::BinaryInfix, 2, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return a * b; }});
    reg.push_back({"/", OperatorDef::BinaryInfix, 2, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return a / b; }});
    reg.push_back({"%", OperatorDef::BinaryInfix, 2, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return std::fmod(a, b); }});
    reg.push_back({"^", OperatorDef::BinaryInfix, 3, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return std::pow(a, b); }});

    // ---- Unary functions (1-arg) ----
    reg.push_back({"abs", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::abs(x); }, nullptr});
    reg.push_back({"sqrt", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::sqrt(x); }, nullptr});
    reg.push_back({"ceil", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::ceil(x); }, nullptr});
    reg.push_back({"floor", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::floor(x); }, nullptr});
    reg.push_back({"exp", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::exp(x); }, nullptr});
    reg.push_back({"ln", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::log(x); }, nullptr});
    reg.push_back({"log10", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::log10(x); }, nullptr});

    // ---- Trig functions (1-arg, ForwardTrig) ----
    reg.push_back({"sin", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::ForwardTrig, [](double x) { return std::sin(x); }, nullptr});
    reg.push_back({"cos", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::ForwardTrig, [](double x) { return std::cos(x); }, nullptr});
    reg.push_back({"tan", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::ForwardTrig, [](double x) { return std::tan(x); }, nullptr});

    // ---- Inverse trig functions (1-arg, InverseTrig) ----
    reg.push_back({"asin", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::InverseTrig, [](double x) { return std::asin(x); }, nullptr});
    reg.push_back({"acos", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::InverseTrig, [](double x) { return std::acos(x); }, nullptr});
    reg.push_back({"atan", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::InverseTrig, [](double x) { return std::atan(x); }, nullptr});

    // ---- Binary functions (2-arg) ----
    // NOTE: log (2-arg) must come AFTER log10 so that prefix matching during
    //       identifier resolution checks "log10" before "log".
    reg.push_back({"log", OperatorDef::Function, 5, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double base, double val) { return std::log(val) / std::log(base); }});
    reg.push_back({"root", OperatorDef::Function, 5, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double val, double n) { return std::pow(val, 1.0 / n); }});
    reg.push_back({"min", OperatorDef::Function, 5, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return std::min(a, b); }});
    reg.push_back({"max", OperatorDef::Function, 5, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return std::max(a, b); }});

    return reg;
  }();

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
std::string ArrayCalculatorParser::nextScratchName()
{
  return "_calc_" + std::to_string(m_ScratchCounter++);
}

// ---------------------------------------------------------------------------
DataObject::IdType ArrayCalculatorParser::createScalarInTemp(double value)
{
  auto* arr = Float64Array::CreateWithStore<Float64DataStore>(m_TempDataStructure, nextScratchName(), std::vector<usize>{1}, std::vector<usize>{1});
  (*arr)[0] = value;
  return arr->getId();
}

// ---------------------------------------------------------------------------
DataObject::IdType ArrayCalculatorParser::copyArrayToTemp(const IDataArray& sourceArray)
{
  auto tupleShape = sourceArray.getTupleShape();
  auto compShape = sourceArray.getComponentShape();
  std::string scratchName = nextScratchName();

  auto* destArr = Float64Array::CreateWithStore<Float64DataStore>(m_TempDataStructure, scratchName, tupleShape, compShape);

  if(!m_IsPreflight)
  {
    ExecuteDataFunction(CopyToFloat64Functor{}, sourceArray.getDataType(), sourceArray, *destArr);
  }

  return destArr->getId();
}

// ---------------------------------------------------------------------------
// parse() -- the core parsing pipeline
// ---------------------------------------------------------------------------
Result<> ArrayCalculatorParser::parse()
{
  Result<> result;

  // === Step 1: Tokenize ===
  std::vector<Token> tokens = tokenize(m_InfixEquation);
  if(tokens.empty())
  {
    return MakeErrorResult(static_cast<int>(CalculatorErrorCode::EmptyEquation), "The infix expression is empty.");
  }

  // === Step 2: Multi-word identifier merging ===
  // Walk the token list. When consecutive Identifier tokens appear, try
  // merging them with spaces (greedy longest-first) and check if the
  // merged name matches an array name.
  {
    std::vector<Token> merged;
    merged.reserve(tokens.size());
    size_t i = 0;
    while(i < tokens.size())
    {
      if(tokens[i].type == TokenType::Identifier)
      {
        // Find the run of consecutive Identifier tokens
        size_t runStart = i;
        size_t runEnd = i + 1;
        while(runEnd < tokens.size() && tokens[runEnd].type == TokenType::Identifier)
        {
          ++runEnd;
        }
        size_t runLen = runEnd - runStart;

        if(runLen > 1)
        {
          // Greedy: try longest merge first
          bool foundMatch = false;
          for(size_t len = runLen; len >= 2; --len)
          {
            for(size_t start = runStart; start + len <= runEnd; ++start)
            {
              // Build the merged name
              std::string mergedName = tokens[start].text;
              for(size_t k = start + 1; k < start + len; ++k)
              {
                mergedName += " " + tokens[k].text;
              }

              // Check if this name matches an array
              bool found = false;
              if(!m_SelectedGroupPath.empty())
              {
                found = ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, mergedName);
              }
              if(!found)
              {
                auto paths = findArraysByName(m_DataStructure, mergedName);
                found = !paths.empty();
              }

              if(found)
              {
                // Add tokens before the match
                for(size_t k = runStart; k < start; ++k)
                {
                  merged.push_back(tokens[k]);
                }
                // Add the merged token
                merged.push_back({TokenType::Identifier, mergedName, tokens[start].position});
                // Add tokens after the match
                for(size_t k = start + len; k < runEnd; ++k)
                {
                  merged.push_back(tokens[k]);
                }
                i = runEnd;
                foundMatch = true;
                break;
              }
            }
            if(foundMatch)
            {
              break;
            }
          }
          if(!foundMatch)
          {
            // No merge found; copy all identifiers as-is
            for(size_t k = runStart; k < runEnd; ++k)
            {
              merged.push_back(tokens[k]);
            }
            i = runEnd;
          }
        }
        else
        {
          merged.push_back(tokens[i]);
          ++i;
        }
      }
      else
      {
        merged.push_back(tokens[i]);
        ++i;
      }
    }
    tokens = std::move(merged);
  }

  // === Steps 3+4: Identifier resolution, token conversion, and bracket indexing ===
  // These steps are combined so brackets can reference the array they follow.
  std::vector<ParsedItem> items;
  items.reserve(tokens.size());

  // Combined Step 3+4: resolve identifiers and handle brackets inline
  for(size_t i = 0; i < tokens.size(); ++i)
  {
    const Token& tok = tokens[i];

    // Check if this token starts a bracket expression [...]
    if(tok.type == TokenType::LBracket)
    {
      // Parse bracket contents: [Number] or [Number, Number]
      if(items.empty())
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OrphanedComponent), "Index operator '[' is not paired with a valid array name or closing parenthesis.");
      }

      // Collect tokens until matching ']'
      std::vector<std::string> bracketNumbers;
      size_t j = i + 1;
      while(j < tokens.size() && tokens[j].type != TokenType::RBracket)
      {
        if(tokens[j].type == TokenType::Number)
        {
          bracketNumbers.push_back(tokens[j].text);
        }
        else if(tokens[j].type == TokenType::Comma)
        {
          // skip comma
        }
        else
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidComponent), fmt::format("Invalid content inside bracket index: '{}'.", tokens[j].text));
        }
        ++j;
      }
      if(j >= tokens.size())
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::MismatchedParentheses), "Missing closing bracket ']'.");
      }
      // j now points to ']'

      ParsedItem& prevItem = items.back();

      if(prevItem.kind == ParsedItem::Kind::Value && prevItem.value.kind == CalcValue::Kind::Array)
      {
        // Case A: Array[C] or Array[T, C]
        auto* tempArr = m_TempDataStructure.getDataAs<Float64Array>(prevItem.value.arrayId);
        if(tempArr == nullptr)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), "Internal error: could not find temporary array for bracket indexing.");
        }

        usize numComponents = tempArr->getNumberOfComponents();
        usize numTuples = tempArr->getNumberOfTuples();

        if(bracketNumbers.size() == 1)
        {
          // [C]: component extraction
          usize compIdx = 0;
          try
          {
            auto parsed = std::stoull(bracketNumbers[0]);
            compIdx = static_cast<usize>(parsed);
          } catch(const std::exception&)
          {
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidComponent), fmt::format("Invalid component index '{}'.", bracketNumbers[0]));
          }

          if(compIdx >= numComponents)
          {
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::ComponentOutOfRange), fmt::format("Component index {} is out of range for array with {} components.", compIdx, numComponents));
          }

          if(numComponents > 1)
          {
            // Create new single-component array by extracting component C
            auto* newArr = Float64Array::CreateWithStore<Float64DataStore>(m_TempDataStructure, nextScratchName(), tempArr->getTupleShape(), std::vector<usize>{1});
            if(!m_IsPreflight)
            {
              for(usize t = 0; t < numTuples; ++t)
              {
                (*newArr)[t] = (*tempArr)[t * numComponents + compIdx];
              }
            }
            prevItem.value.arrayId = newArr->getId();
          }
          // If already single-component, leave as-is
        }
        else if(bracketNumbers.size() == 2)
        {
          // [T, C]: tuple+component extraction
          usize tupleIdx = 0;
          usize compIdx = 0;
          try
          {
            tupleIdx = static_cast<usize>(std::stoull(bracketNumbers[0]));
            compIdx = static_cast<usize>(std::stoull(bracketNumbers[1]));
          } catch(const std::exception&)
          {
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidComponent), fmt::format("Invalid tuple/component index in '[{}, {}]'.", bracketNumbers[0], bracketNumbers[1]));
          }

          if(tupleIdx >= numTuples)
          {
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::TupleOutOfRange), fmt::format("Tuple index {} is out of range for array with {} tuples.", tupleIdx, numTuples));
          }
          if(compIdx >= numComponents)
          {
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::ComponentOutOfRange), fmt::format("Component index {} is out of range for array with {} components.", compIdx, numComponents));
          }

          double extractedValue = 0.0;
          if(!m_IsPreflight)
          {
            extractedValue = (*tempArr)[tupleIdx * numComponents + compIdx];
          }
          DataObject::IdType id = createScalarInTemp(extractedValue);
          prevItem.value = CalcValue{CalcValue::Kind::Number, id};
        }
        else
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidComponent), "Bracket index must contain 1 or 2 numbers (e.g. [C] or [T, C]).");
        }
      }
      else if(prevItem.kind == ParsedItem::Kind::RParen)
      {
        // Case B: )[C] -- component extraction on sub-expression result
        if(bracketNumbers.size() != 1)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidComponent), "Component extraction on sub-expression must have exactly one index: [C].");
        }
        usize compIdx = 0;
        try
        {
          compIdx = static_cast<usize>(std::stoull(bracketNumbers[0]));
        } catch(const std::exception&)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidComponent), fmt::format("Invalid component index '{}'.", bracketNumbers[0]));
        }

        ParsedItem ce;
        ce.kind = ParsedItem::Kind::ComponentExtract;
        ce.componentIndex = compIdx;
        items.push_back(ce);
      }
      else
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OrphanedComponent), fmt::format("Index operator '{}' is not paired with a valid array name or closing parenthesis.", tok.text));
      }

      i = j; // skip past ']'
      continue;
    }

    // Normal token processing (same as step 3 above but now with brackets handled separately)
    switch(tok.type)
    {
    case TokenType::Number: {
      double numValue = 0.0;
      try
      {
        numValue = std::stod(tok.text);
      } catch(const std::exception&)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), fmt::format("Invalid numeric value '{}'.", tok.text));
      }
      DataObject::IdType id = createScalarInTemp(numValue);

      ParsedItem pi;
      pi.kind = ParsedItem::Kind::Value;
      pi.value = CalcValue{CalcValue::Kind::Number, id};
      items.push_back(pi);

      // Ambiguous name warning
      if(m_IsPreflight)
      {
        bool arrayExists = false;
        if(!m_SelectedGroupPath.empty())
        {
          arrayExists = ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, tok.text);
        }
        if(!arrayExists)
        {
          arrayExists = !findArraysByName(m_DataStructure, tok.text).empty();
        }
        if(arrayExists)
        {
          result.warnings().push_back(Warning{static_cast<int>(CalculatorWarningCode::AmbiguousNameWarning),
                                              fmt::format("Item '{}' in the infix expression is the name of an array, but it is currently being used as a number."
                                                          "\nTo treat this item as an array name, please add double quotes around the item (i.e. \"{}\").",
                                                          tok.text, tok.text)});
        }
      }
      break;
    }

    case TokenType::Identifier: {
      const OperatorDef* opDef = findOperatorByToken(tok.text);
      if(opDef != nullptr)
      {
        if(m_IsPreflight)
        {
          bool arrayExists = false;
          if(!m_SelectedGroupPath.empty())
          {
            arrayExists = ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, tok.text);
          }
          if(!arrayExists)
          {
            arrayExists = !findArraysByName(m_DataStructure, tok.text).empty();
          }
          if(arrayExists)
          {
            result.warnings().push_back(Warning{static_cast<int>(CalculatorWarningCode::AmbiguousNameWarning),
                                                fmt::format("Item '{}' in the infix expression is the name of an array, but it is currently being used as a mathematical operator."
                                                            "\nTo treat this item as an array name, please add double quotes around the item (i.e. \"{}\").",
                                                            tok.text, tok.text)});
          }
        }

        ParsedItem pi;
        pi.kind = ParsedItem::Kind::Operator;
        pi.op = opDef;
        items.push_back(pi);
      }
      else if(tok.text == "pi" || tok.text == "e")
      {
        double constValue = (tok.text == "pi") ? std::numbers::pi : std::numbers::e;
        DataObject::IdType id = createScalarInTemp(constValue);

        ParsedItem pi;
        pi.kind = ParsedItem::Kind::Value;
        pi.value = CalcValue{CalcValue::Kind::Number, id};
        items.push_back(pi);

        if(m_IsPreflight)
        {
          bool arrayExists = false;
          if(!m_SelectedGroupPath.empty())
          {
            arrayExists = ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, tok.text);
          }
          if(!arrayExists)
          {
            arrayExists = !findArraysByName(m_DataStructure, tok.text).empty();
          }
          if(arrayExists)
          {
            result.warnings().push_back(Warning{static_cast<int>(CalculatorWarningCode::AmbiguousNameWarning),
                                                fmt::format("Item '{}' in the infix expression is the name of an array, but it is currently being used as a built-in constant."
                                                            "\nTo treat this item as an array name, please add double quotes around the item (i.e. \"{}\").",
                                                            tok.text, tok.text)});
          }
        }
      }
      else
      {
        // Try as array name
        if(!m_SelectedGroupPath.empty() && ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, tok.text))
        {
          DataPath arrayPath = m_SelectedGroupPath.createChildPath(tok.text);
          const auto* dataArray = m_DataStructure.getDataAs<IDataArray>(arrayPath);
          if(dataArray == nullptr)
          {
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::UnrecognizedItem), fmt::format("Could not access array '{}' in selected group.", tok.text));
          }
          DataObject::IdType id = copyArrayToTemp(*dataArray);

          ParsedItem pi;
          pi.kind = ParsedItem::Kind::Value;
          pi.value = CalcValue{CalcValue::Kind::Array, id};
          items.push_back(pi);
        }
        else
        {
          auto foundPaths = findArraysByName(m_DataStructure, tok.text);
          if(foundPaths.empty())
          {
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::UnrecognizedItem), fmt::format("An unrecognized or invalid item '{}' was found in the chosen infix expression.", tok.text));
          }
          if(foundPaths.size() > 1)
          {
            std::string pathsList;
            for(const auto& p : foundPaths)
            {
              if(!pathsList.empty())
              {
                pathsList += ", ";
              }
              pathsList += p.toString();
            }
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::AmbiguousArrayName), fmt::format("Array name '{}' is ambiguous. Multiple arrays found: [{}]."
                                                                                                          "\nPlease use double quotes with the full path (e.g. \"Path/To/{}\") to disambiguate.",
                                                                                                          tok.text, pathsList, tok.text));
          }
          const auto* dataArray = m_DataStructure.getDataAs<IDataArray>(foundPaths[0]);
          if(dataArray == nullptr)
          {
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::UnrecognizedItem), fmt::format("Could not access array '{}'.", tok.text));
          }
          DataObject::IdType id = copyArrayToTemp(*dataArray);

          ParsedItem pi;
          pi.kind = ParsedItem::Kind::Value;
          pi.value = CalcValue{CalcValue::Kind::Array, id};
          items.push_back(pi);
        }
      }
      break;
    }

    case TokenType::QuotedString: {
      std::vector<std::string> pathComponents;
      {
        std::string component;
        for(char ch : tok.text)
        {
          if(ch == '/')
          {
            if(!component.empty())
            {
              pathComponents.push_back(component);
              component.clear();
            }
          }
          else
          {
            component += ch;
          }
        }
        if(!component.empty())
        {
          pathComponents.push_back(component);
        }
      }

      DataPath quotedPath(pathComponents);

      // If single component, try as child of selected group first
      if(pathComponents.size() == 1 && !m_SelectedGroupPath.empty())
      {
        DataPath childPath = m_SelectedGroupPath.createChildPath(pathComponents[0]);
        if(m_DataStructure.getDataAs<IDataArray>(childPath) != nullptr)
        {
          quotedPath = childPath;
        }
      }

      const auto* dataArray = m_DataStructure.getDataAs<IDataArray>(quotedPath);
      if(dataArray == nullptr)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidArrayName), fmt::format("The item '\"{}\"' is not a valid array path in the DataStructure.", tok.text));
      }
      DataObject::IdType id = copyArrayToTemp(*dataArray);

      ParsedItem pi;
      pi.kind = ParsedItem::Kind::Value;
      pi.value = CalcValue{CalcValue::Kind::Array, id};
      items.push_back(pi);
      break;
    }

    case TokenType::Plus:
    case TokenType::Minus:
    case TokenType::Star:
    case TokenType::Slash:
    case TokenType::Caret:
    case TokenType::Percent: {
      const OperatorDef* opDef = operatorDefForSymbolToken(tok.type);
      if(opDef == nullptr)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidSymbol), fmt::format("Unknown operator symbol '{}'.", tok.text));
      }

      // Check if the operator symbol is also the name of an array
      if(m_IsPreflight)
      {
        bool arrayExists = false;
        if(!m_SelectedGroupPath.empty())
        {
          arrayExists = ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, tok.text);
        }
        if(!arrayExists)
        {
          arrayExists = !findArraysByName(m_DataStructure, tok.text).empty();
        }
        if(arrayExists)
        {
          result.warnings().push_back(Warning{static_cast<int>(CalculatorWarningCode::AmbiguousNameWarning),
                                              fmt::format("Item '{}' in the infix expression is the name of an array, but it is currently being used as a mathematical operator."
                                                          "\nTo treat this item as an array name, please add double quotes around the item (i.e. \"{}\").",
                                                          tok.text, tok.text)});
        }
      }

      ParsedItem pi;
      pi.kind = ParsedItem::Kind::Operator;
      pi.op = opDef;
      items.push_back(pi);
      break;
    }

    case TokenType::LParen: {
      ParsedItem pi;
      pi.kind = ParsedItem::Kind::LParen;
      items.push_back(pi);
      break;
    }

    case TokenType::RParen: {
      ParsedItem pi;
      pi.kind = ParsedItem::Kind::RParen;
      items.push_back(pi);
      break;
    }

    case TokenType::Comma: {
      ParsedItem pi;
      pi.kind = ParsedItem::Kind::Comma;
      items.push_back(pi);
      break;
    }

    case TokenType::LBracket:
    case TokenType::RBracket: {
      // Should not reach here since brackets are handled at the top of the loop
      return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), "Unexpected bracket token encountered.");
    }

    } // end switch
  }

  // === Step 5: Minus sign disambiguation ===
  for(size_t i = 0; i < items.size(); ++i)
  {
    auto& item = items[i];
    if(item.kind != ParsedItem::Kind::Operator || item.op == nullptr)
    {
      continue;
    }
    if(item.op->token != "-")
    {
      continue;
    }

    // Determine if this is a unary negative
    bool isUnary = false;
    if(i == 0)
    {
      isUnary = true;
    }
    else
    {
      const auto& prev = items[i - 1];
      if(isBinaryOp(prev))
      {
        isUnary = true;
      }
      else if(prev.kind == ParsedItem::Kind::Operator && prev.op != nullptr && prev.op->kind == OperatorDef::UnaryPrefix)
      {
        isUnary = true;
      }
      else if(prev.kind == ParsedItem::Kind::LParen)
      {
        isUnary = true;
      }
      else if(prev.kind == ParsedItem::Kind::Comma)
      {
        isUnary = true;
      }
    }

    if(isUnary)
    {
      item.op = &getUnaryNegativeOp();
      item.isNegativePrefix = true;
    }
  }

  // === Step 6: WrapFunctionArguments ===
  wrapFunctionArguments(items);

  // === Step 7: Validation ===

  // 7a-1: Check for function/unary operators: opening/closing paren, argument count, empty args
  for(size_t i = 0; i < items.size(); ++i)
  {
    const auto& item = items[i];
    if(item.kind == ParsedItem::Kind::Operator && item.op != nullptr && item.op->kind == OperatorDef::Function)
    {
      // A function operator must be followed by LParen
      if(i + 1 >= items.size() || items[i + 1].kind != ParsedItem::Kind::LParen)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OperatorNoOpeningParen), fmt::format("The operator/function '{}' does not have a valid opening parenthesis.", item.op->token));
      }

      // Find the matching RParen and count commas/values at depth 1
      int depth = 0;
      bool foundClose = false;
      size_t closeIdx = 0;
      int commaCount = 0;
      bool hasValueInside = false;
      for(size_t j = i + 1; j < items.size(); ++j)
      {
        if(items[j].kind == ParsedItem::Kind::LParen)
        {
          ++depth;
        }
        else if(items[j].kind == ParsedItem::Kind::RParen)
        {
          --depth;
          if(depth == 0)
          {
            foundClose = true;
            closeIdx = j;
            break;
          }
        }
        else if(items[j].kind == ParsedItem::Kind::Comma && depth == 1)
        {
          ++commaCount;
        }
        else if(depth >= 1 && (items[j].kind == ParsedItem::Kind::Value || (items[j].kind == ParsedItem::Kind::Operator && items[j].op != nullptr)))
        {
          hasValueInside = true;
        }
      }
      if(!foundClose)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OperatorNoClosingParen), fmt::format("The operator/function '{}' does not have a valid closing parenthesis.", item.op->token));
      }

      // Check for empty function call: func() with no values or commas inside
      if(!hasValueInside && commaCount == 0)
      {
        // For 2-arg functions with empty parens: NotEnoughArguments
        if(item.op->numArgs == 2)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NotEnoughArguments),
                                 fmt::format("The function '{}' requires {} arguments, but none were provided.", item.op->token, item.op->numArgs));
        }
        // For 1-arg functions with empty parens: NoNumericArguments
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NoNumericArguments), fmt::format("The function '{}' does not have any arguments that simplify down to a number.", item.op->token));
      }

      // Check for commas in the empty-value case: func(,) -- commas but no real values
      if(!hasValueInside && commaCount > 0)
      {
        if(item.op->numArgs == 1)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::TooManyArguments),
                                 fmt::format("The function '{}' requires {} argument, but more were provided.", item.op->token, item.op->numArgs));
        }
        // For 2-arg functions: NoNumericArguments (commas but no values)
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NoNumericArguments), fmt::format("The function '{}' does not have any arguments that simplify down to a number.", item.op->token));
      }

      // Argument count: numArgs from OperatorDef, commaCount gives (numArgs-1)
      int providedArgs = commaCount + 1;
      if(item.op->numArgs == 1 && commaCount > 0)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::TooManyArguments),
                               fmt::format("The function '{}' requires {} argument, but {} were provided.", item.op->token, item.op->numArgs, providedArgs));
      }
      if(item.op->numArgs == 2 && commaCount < 1 && hasValueInside)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NotEnoughArguments),
                               fmt::format("The function '{}' requires {} arguments, but only {} was provided.", item.op->token, item.op->numArgs, providedArgs));
      }
    }
  }

  // 7a-1b: Check for commas inside non-function parentheses (NoPrecedingUnaryOperator)
  for(size_t i = 0; i < items.size(); ++i)
  {
    if(items[i].kind == ParsedItem::Kind::Comma)
    {
      // Walk backwards to find the opening paren at the same depth, and check if preceded by a function
      int depth = 0;
      bool foundFunction = false;
      for(int j = static_cast<int>(i) - 1; j >= 0; --j)
      {
        if(items[j].kind == ParsedItem::Kind::RParen)
        {
          ++depth;
        }
        else if(items[j].kind == ParsedItem::Kind::LParen)
        {
          if(depth == 0)
          {
            // Found the opening paren; check if preceded by a function
            if(j > 0 && items[j - 1].kind == ParsedItem::Kind::Operator && items[j - 1].op != nullptr && items[j - 1].op->kind == OperatorDef::Function)
            {
              foundFunction = true;
            }
            break;
          }
          --depth;
        }
      }
      if(!foundFunction)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NoPrecedingUnaryOperator), "A comma was found in parentheses without a preceding function operator.");
      }
    }
  }

  // 7a-2: Check for binary operators missing left or right operands
  for(size_t i = 0; i < items.size(); ++i)
  {
    const auto& item = items[i];
    if(!isBinaryOp(item))
    {
      continue;
    }
    // Check left: the item before must be a value or RParen (something that produces a value)
    bool hasLeft = false;
    if(i > 0)
    {
      const auto& prev = items[i - 1];
      if(prev.kind == ParsedItem::Kind::Value || prev.kind == ParsedItem::Kind::RParen)
      {
        hasLeft = true;
      }
    }
    if(!hasLeft)
    {
      return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OperatorNoLeftValue), fmt::format("The binary operator '{}' does not have a valid left-hand value.", item.op->token));
    }
    // Check right: the item after must be a value, LParen, or unary operator (something that produces a value)
    bool hasRight = false;
    if(i + 1 < items.size())
    {
      const auto& next = items[i + 1];
      if(next.kind == ParsedItem::Kind::Value || next.kind == ParsedItem::Kind::LParen)
      {
        hasRight = true;
      }
      else if(next.kind == ParsedItem::Kind::Operator && next.op != nullptr)
      {
        hasRight = true; // Could be a unary prefix or function
      }
    }
    if(!hasRight)
    {
      return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OperatorNoRightValue), fmt::format("The binary operator '{}' does not have a valid right-hand value.", item.op->token));
    }
  }

  // 7a-3: Check for unary negative with no right operand
  for(size_t i = 0; i < items.size(); ++i)
  {
    const auto& item = items[i];
    if(item.isNegativePrefix)
    {
      bool hasRight = false;
      if(i + 1 < items.size())
      {
        const auto& next = items[i + 1];
        if(next.kind == ParsedItem::Kind::Value || next.kind == ParsedItem::Kind::LParen)
        {
          hasRight = true;
        }
        else if(next.kind == ParsedItem::Kind::Operator && next.op != nullptr)
        {
          hasRight = true; // e.g. -sin(...)
        }
      }
      if(!hasRight)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OperatorNoRightValue), "The unary negative operator does not have a valid right-hand value.");
      }
    }
  }

  // 7a-4: Check matched parentheses (generic, after operator-specific checks)
  {
    int parenDepth = 0;
    for(const auto& item : items)
    {
      if(item.kind == ParsedItem::Kind::LParen)
      {
        ++parenDepth;
      }
      else if(item.kind == ParsedItem::Kind::RParen)
      {
        --parenDepth;
      }
      if(parenDepth < 0)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::MismatchedParentheses),
                               fmt::format("One or more parentheses are mismatched in the chosen infix expression '{}'.", m_InfixEquation));
      }
    }
    if(parenDepth != 0)
    {
      return MakeErrorResult(static_cast<int>(CalculatorErrorCode::MismatchedParentheses), fmt::format("One or more parentheses are mismatched in the chosen infix expression '{}'.", m_InfixEquation));
    }
  }

  // 7b: Collect array-type values and verify consistent tuple/component info
  std::vector<usize> arrayTupleShape;
  std::vector<usize> arrayCompShape;
  usize arrayNumTuples = 0;
  bool hasArray = false;
  bool hasNumericValue = false;
  bool tupleShapesMatch = true;

  for(const auto& item : items)
  {
    if(item.kind == ParsedItem::Kind::Value)
    {
      hasNumericValue = true;
      if(item.value.kind == CalcValue::Kind::Array)
      {
        auto* arr = m_TempDataStructure.getDataAs<Float64Array>(item.value.arrayId);
        if(arr != nullptr)
        {
          auto ts = arr->getTupleShape();
          auto cs = arr->getComponentShape();
          usize nt = arr->getNumberOfTuples();

          if(hasArray)
          {
            if(!arrayCompShape.empty() && arrayCompShape != cs)
            {
              return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InconsistentCompDims), "Attribute Array symbols in the infix expression have mismatching component dimensions.");
            }
            if(arrayNumTuples != 0 && nt != arrayNumTuples)
            {
              return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InconsistentTuples), "Attribute Array symbols in the infix expression have mismatching number of tuples.");
            }
            if(!arrayTupleShape.empty() && arrayTupleShape != ts)
            {
              tupleShapesMatch = false;
            }
          }

          hasArray = true;
          arrayTupleShape = ts;
          arrayCompShape = cs;
          arrayNumTuples = nt;
        }
      }
    }
  }

  // 7c: Ensure at least one numeric argument exists
  if(!hasNumericValue)
  {
    return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NoNumericArguments), "The expression does not have any arguments that simplify down to a number.");
  }

  // Check if there is a ComponentExtract item in the parsed list.
  // If so, the final output will be single-component regardless of the
  // input array's component shape.
  bool hasComponentExtract = false;
  for(const auto& item : items)
  {
    if(item.kind == ParsedItem::Kind::ComponentExtract)
    {
      hasComponentExtract = true;
      break;
    }
  }

  // Store the parsed shape info for use by parseAndValidate()
  if(hasArray)
  {
    if(tupleShapesMatch)
    {
      m_ParsedTupleShape = arrayTupleShape;
    }
    else
    {
      m_ParsedTupleShape = {arrayNumTuples};
    }
    m_ParsedComponentShape = hasComponentExtract ? std::vector<usize>{1} : arrayCompShape;
  }
  else
  {
    // All scalars: output is {1} tuples, {1} components
    m_ParsedTupleShape = {1};
    m_ParsedComponentShape = {1};
  }

  // === Convert ParsedItems to RPN using shunting-yard ===
  m_RpnItems.clear();
  std::vector<ParsedItem> opStack;

  for(const auto& item : items)
  {
    switch(item.kind)
    {
    case ParsedItem::Kind::Value: {
      m_RpnItems.push_back(RpnItem{RpnItem::Type::Value, item.value, nullptr, std::numeric_limits<usize>::max()});
      break;
    }

    case ParsedItem::Kind::LParen: {
      opStack.push_back(item);
      break;
    }

    case ParsedItem::Kind::RParen: {
      // Pop operators to output until we find the matching LParen
      while(!opStack.empty() && opStack.back().kind != ParsedItem::Kind::LParen)
      {
        const auto& top = opStack.back();
        m_RpnItems.push_back(RpnItem{RpnItem::Type::Operator, CalcValue{CalcValue::Kind::Number, 0}, top.op, std::numeric_limits<usize>::max()});
        opStack.pop_back();
      }
      if(opStack.empty())
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::MismatchedParentheses),
                               fmt::format("One or more parentheses are mismatched in the chosen infix expression '{}'.", m_InfixEquation));
      }
      // Discard the LParen
      opStack.pop_back();
      break;
    }

    case ParsedItem::Kind::Comma: {
      // Pop operators to output until we find the LParen (but don't discard it)
      while(!opStack.empty() && opStack.back().kind != ParsedItem::Kind::LParen)
      {
        const auto& top = opStack.back();
        m_RpnItems.push_back(RpnItem{RpnItem::Type::Operator, CalcValue{CalcValue::Kind::Number, 0}, top.op, std::numeric_limits<usize>::max()});
        opStack.pop_back();
      }
      break;
    }

    case ParsedItem::Kind::Operator: {
      const OperatorDef* incomingOp = item.isNegativePrefix ? &getUnaryNegativeOp() : item.op;
      int incomingPrec = incomingOp->precedence;
      bool isLeftAssoc = (incomingOp->associativity == OperatorDef::Left);

      while(!opStack.empty() && opStack.back().kind == ParsedItem::Kind::Operator)
      {
        const auto& topItem = opStack.back();
        const OperatorDef* topOp = topItem.isNegativePrefix ? &getUnaryNegativeOp() : topItem.op;
        int topPrec = topOp->precedence;

        if(topPrec > incomingPrec || (topPrec == incomingPrec && isLeftAssoc))
        {
          m_RpnItems.push_back(RpnItem{RpnItem::Type::Operator, CalcValue{CalcValue::Kind::Number, 0}, topOp, std::numeric_limits<usize>::max()});
          opStack.pop_back();
        }
        else
        {
          break;
        }
      }

      opStack.push_back(item);
      break;
    }

    case ParsedItem::Kind::ComponentExtract: {
      m_RpnItems.push_back(RpnItem{RpnItem::Type::ComponentExtract, CalcValue{CalcValue::Kind::Number, 0}, nullptr, item.componentIndex});
      break;
    }

    } // end switch
  }

  // Pop remaining operators from the stack
  while(!opStack.empty())
  {
    const auto& top = opStack.back();
    if(top.kind == ParsedItem::Kind::LParen)
    {
      return MakeErrorResult(static_cast<int>(CalculatorErrorCode::MismatchedParentheses), fmt::format("One or more parentheses are mismatched in the chosen infix expression '{}'.", m_InfixEquation));
    }
    const OperatorDef* topOp = top.isNegativePrefix ? &getUnaryNegativeOp() : top.op;
    m_RpnItems.push_back(RpnItem{RpnItem::Type::Operator, CalcValue{CalcValue::Kind::Number, 0}, topOp, std::numeric_limits<usize>::max()});
    opStack.pop_back();
  }

  return result;
}

// ---------------------------------------------------------------------------
Result<> ArrayCalculatorParser::parseAndValidate(std::vector<usize>& outTupleShape, std::vector<usize>& outComponentShape)
{
  Result<> parseResult = parse();
  if(parseResult.invalid())
  {
    return parseResult;
  }

  outTupleShape = m_ParsedTupleShape;
  outComponentShape = m_ParsedComponentShape;

  return parseResult;
}

// ---------------------------------------------------------------------------
Result<> ArrayCalculatorParser::evaluateInto(DataStructure& dataStructure, const DataPath& outputPath, NumericType scalarType, CalculatorParameter::AngleUnits units)
{
  // 1. Parse (which now populates m_RpnItems via shunting-yard)
  Result<> parseResult = parse();
  if(parseResult.invalid())
  {
    return parseResult;
  }

  // 2. Walk the RPN items using an evaluation stack
  std::stack<CalcValue> evalStack;

  for(const auto& rpnItem : m_RpnItems)
  {
    switch(rpnItem.type)
    {
    case RpnItem::Type::Value: {
      evalStack.push(rpnItem.value);
      break;
    }

    case RpnItem::Type::Operator: {
      const OperatorDef* op = rpnItem.op;
      if(op == nullptr)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), "Internal error: null operator in RPN evaluation.");
      }

      if(op->numArgs == 1)
      {
        // Unary operator / 1-arg function
        if(evalStack.empty())
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NotEnoughArguments), "Not enough arguments for unary operator.");
        }
        CalcValue operand = evalStack.top();
        evalStack.pop();

        auto* operandArr = m_TempDataStructure.getDataAs<Float64Array>(operand.arrayId);
        if(operandArr == nullptr)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), "Internal error: could not find operand array during evaluation.");
        }

        auto tupleShape = operandArr->getTupleShape();
        auto compShape = operandArr->getComponentShape();
        auto* resultArr = Float64Array::CreateWithStore<Float64DataStore>(m_TempDataStructure, nextScratchName(), tupleShape, compShape);

        usize totalSize = operandArr->getSize();
        for(usize i = 0; i < totalSize; i++)
        {
          double val = operandArr->at(i);

          // Handle trig angle unit conversions
          if(op->trigMode == OperatorDef::ForwardTrig && units == CalculatorParameter::AngleUnits::Degrees)
          {
            val = val * (std::numbers::pi / 180.0);
          }

          double res = op->unaryOp(val);

          if(op->trigMode == OperatorDef::InverseTrig && units == CalculatorParameter::AngleUnits::Degrees)
          {
            res = res * (180.0 / std::numbers::pi);
          }

          (*resultArr)[i] = res;
        }

        evalStack.push(CalcValue{operand.kind, resultArr->getId()});
      }
      else if(op->numArgs == 2)
      {
        // Binary operator / 2-arg function
        if(evalStack.size() < 2)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NotEnoughArguments), "Not enough arguments for binary operator.");
        }
        CalcValue rightVal = evalStack.top();
        evalStack.pop();
        CalcValue leftVal = evalStack.top();
        evalStack.pop();

        auto* leftArr = m_TempDataStructure.getDataAs<Float64Array>(leftVal.arrayId);
        auto* rightArr = m_TempDataStructure.getDataAs<Float64Array>(rightVal.arrayId);
        if(leftArr == nullptr || rightArr == nullptr)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), "Internal error: could not find operand arrays during evaluation.");
        }

        // Determine output shape: use array operand's shape (broadcast scalars)
        std::vector<usize> outTupleShape;
        std::vector<usize> outCompShape;
        if(leftVal.kind == CalcValue::Kind::Array)
        {
          outTupleShape = leftArr->getTupleShape();
          outCompShape = leftArr->getComponentShape();
        }
        else
        {
          outTupleShape = rightArr->getTupleShape();
          outCompShape = rightArr->getComponentShape();
        }

        usize numTuples = 1;
        for(auto d : outTupleShape)
        {
          numTuples *= d;
        }
        usize numComps = 1;
        for(auto d : outCompShape)
        {
          numComps *= d;
        }
        usize totalSize = numTuples * numComps;

        auto* resultArr = Float64Array::CreateWithStore<Float64DataStore>(m_TempDataStructure, nextScratchName(), outTupleShape, outCompShape);

        for(usize i = 0; i < totalSize; i++)
        {
          double lv = (leftVal.kind == CalcValue::Kind::Array) ? leftArr->at(i) : leftArr->at(0);
          double rv = (rightVal.kind == CalcValue::Kind::Array) ? rightArr->at(i) : rightArr->at(0);
          (*resultArr)[i] = op->binaryOp(lv, rv);
        }

        CalcValue::Kind resultKind = (leftVal.kind == CalcValue::Kind::Array || rightVal.kind == CalcValue::Kind::Array) ? CalcValue::Kind::Array : CalcValue::Kind::Number;
        evalStack.push(CalcValue{resultKind, resultArr->getId()});
      }
      else
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), fmt::format("Internal error: operator '{}' has unsupported numArgs={}.", op->token, op->numArgs));
      }
      break;
    }

    case RpnItem::Type::ComponentExtract: {
      if(evalStack.empty())
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NotEnoughArguments), "Not enough arguments for component extraction.");
      }
      CalcValue operand = evalStack.top();
      evalStack.pop();

      auto* operandArr = m_TempDataStructure.getDataAs<Float64Array>(operand.arrayId);
      if(operandArr == nullptr)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), "Internal error: could not find operand array for component extraction.");
      }

      usize numComps = operandArr->getNumberOfComponents();
      usize numTuples = operandArr->getNumberOfTuples();
      usize compIdx = rpnItem.componentIndex;

      if(compIdx >= numComps)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::ComponentOutOfRange), fmt::format("Component index {} is out of range for array with {} components.", compIdx, numComps));
      }

      auto* newArr = Float64Array::CreateWithStore<Float64DataStore>(m_TempDataStructure, nextScratchName(), operandArr->getTupleShape(), std::vector<usize>{1});
      for(usize t = 0; t < numTuples; ++t)
      {
        (*newArr)[t] = operandArr->at(t * numComps + compIdx);
      }

      evalStack.push(CalcValue{operand.kind, newArr->getId()});
      break;
    }

    } // end switch
  }

  // 3. Final result
  if(evalStack.size() != 1)
  {
    return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), fmt::format("Internal error: evaluation stack has {} items remaining; expected exactly 1.", evalStack.size()));
  }

  CalcValue finalVal = evalStack.top();
  evalStack.pop();

  auto* resultArr = m_TempDataStructure.getDataAs<Float64Array>(finalVal.arrayId);
  if(resultArr == nullptr)
  {
    return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), "Internal error: could not find final result array.");
  }

  // 4. Copy/cast result into the output array
  DataType outputDataType = ConvertNumericTypeToDataType(scalarType);
  bool isScalar = (finalVal.kind == CalcValue::Kind::Number);
  ExecuteDataFunction(CopyResultFunctor{}, outputDataType, dataStructure, outputPath, resultArr, isScalar);

  return parseResult;
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
  ArrayCalculatorParser parser(m_DataStructure, m_InputValues->SelectedGroup, m_InputValues->InfixEquation, false);
  return parser.evaluateInto(m_DataStructure, m_InputValues->CalculatedArray, m_InputValues->ScalarType, m_InputValues->Units);
}
