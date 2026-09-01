#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/CalculatorParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"

#include <atomic>
#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @enum CalculatorErrorCode
 * @brief Identifies calculator error conditions.
 *
 * The values preserve CalculatorItem error codes for pipeline compatibility.
 */
enum class CalculatorErrorCode : int
{
  Success = 0,                           ///< No calculation error.
  InvalidEquation = -4009,               ///< Expression syntax or semantics cannot form a calculation.
  InvalidComponent = -4010,              ///< Bracket-index syntax is invalid.
  EmptyEquation = -4011,                 ///< Expression contains no tokens.
  EmptyCalArray = -4012,                 ///< Calculated array has no values.
  EmptySelMatrix = -4013,                ///< Selected group has no arrays.
  LostAttrMatrix = -4014,                ///< Selected group is unavailable.
  IncorrectTupleCount = -4015,           ///< Output tuple count is invalid.
  InconsistentTuples = -4016,            ///< Referenced arrays have different tuple counts.
  UnrecognizedItem = -4017,              ///< Token cannot resolve to an operator or array.
  MismatchedParentheses = -4018,         ///< Parentheses do not balance.
  UnexpectedOutput = -4019,              ///< Output layout is unsupported.
  ComponentOutOfRange = -4020,           ///< Selected component is outside its operand.
  InvalidArrayName = -4022,              ///< Quoted array path cannot resolve.
  InconsistentIndexing = -4023,          ///< Referenced arrays use incompatible indexing.
  InconsistentCompDims = -4024,          ///< Referenced arrays have different component shapes.
  AttrArrayZeroTuplesWarning = -4025,    ///< Referenced array has no tuples.
  OrphanedComponent = -4026,             ///< Index does not follow an array or expression.
  OperatorNoLeftValue = -4027,           ///< Binary operator lacks a left operand.
  OperatorNoRightValue = -4028,          ///< Operator lacks a right operand.
  OperatorNoOpeningParen = -4029,        ///< Function lacks an opening parenthesis.
  OperatorNoClosingParen = -4030,        ///< Function lacks a closing parenthesis.
  NoNumericArguments = -4031,            ///< Expression has no numeric values.
  MissingArguments = -4032,              ///< Function call omits required arguments.
  NotEnoughArguments = -4033,            ///< Function has fewer arguments than required.
  TooManyArguments = -4034,              ///< Function has more arguments than allowed.
  InvalidSymbol = -4035,                 ///< Token uses an unsupported symbol.
  NoPrecedingUnaryOperator = -4036,      ///< Comma is not inside function arguments.
  InvalidOutputArrayType = -4037,        ///< Output element type is unsupported.
  AttributeMatrixInsertionError = -4038, ///< Output group cannot accept the calculated array.
  AmbiguousArrayName = -4039,            ///< Unquoted name resolves to multiple arrays.
  TupleOutOfRange = -4040                ///< Selected tuple is outside its operand.
};

/**
 * @enum CalculatorWarningCode
 * @brief Identifies calculator warning conditions.
 *
 * The values preserve CalculatorItem warning codes for pipeline compatibility.
 */
enum class CalculatorWarningCode : int
{
  None = 0,                    ///< No warning condition.
  NumericValueWarning = -5010, ///< Numeric input has an ambiguous interpretation.
  AmbiguousNameWarning = -5011 ///< Name conflicts with a built-in item.
};

/**
 * @enum TokenType
 * @brief Identifies lexical item types in an equation.
 */
enum class TokenType
{
  Number,       ///< Numeric literal.
  Identifier,   ///< Unquoted name.
  QuotedString, ///< Quoted array path.
  Plus,         ///< Addition symbol.
  Minus,        ///< Subtraction or unary-negative symbol.
  Star,         ///< Multiplication symbol.
  Slash,        ///< Division symbol.
  Caret,        ///< Exponent symbol.
  Percent,      ///< Remainder symbol.
  LParen,       ///< Opening parenthesis.
  RParen,       ///< Closing parenthesis.
  LBracket,     ///< Opening index bracket.
  RBracket,     ///< Closing index bracket.
  Comma         ///< Function argument separator.
};

/**
 * @struct Token
 * @brief Stores one lexical item from an equation.
 */
struct SIMPLNXCORE_EXPORT Token
{
  TokenType type;
  std::string text;
  size_t position = 0; ///< Zero-based byte offset in the equation.
};

/**
 * @struct OperatorDef
 * @brief Defines one calculator operator or function.
 */
struct SIMPLNXCORE_EXPORT OperatorDef
{
  std::string token;

  /**
   * @enum Kind
   * @brief Identifies operator expression syntax.
   */
  enum Kind
  {
    BinaryInfix, ///< Uses two operands between values.
    Function,    ///< Uses function-call syntax.
    UnaryPrefix  ///< Uses one operand after the symbol.
  } kind;

  int precedence;
  int numArgs;

  /**
   * @enum Associativity
   * @brief Identifies how equal-precedence operators group.
   */
  enum Associativity
  {
    Left, ///< Groups from left to right.
    Right ///< Groups from right to left.
  } associativity = Left;

  /**
   * @enum TrigMode
   * @brief Identifies required angle conversion.
   */
  enum TrigMode
  {
    None,        ///< Does not convert angle units.
    ForwardTrig, ///< Converts degrees before trigonometric evaluation.
    InverseTrig  ///< Converts inverse-trigonometric results to degrees.
  } trigMode = None;

  // This callable evaluates one operand when numArgs is one.
  std::function<double(double)> unaryOp;
  // This callable evaluates two operands when numArgs is two.
  std::function<double(double, double)> binaryOp;
};

/**
 * @struct RpnItem
 * @brief Stores one reverse-polish notation item.
 *
 * Array items retain paths and data types instead of DataObject IDs. This keeps
 * parsed expressions independent of transient object identifiers.
 */
struct SIMPLNXCORE_EXPORT RpnItem
{
  /**
   * @enum Type
   * @brief Identifies the value or operation stored by an item.
   */
  enum class Type
  {
    Scalar,               ///< Stores one scalar literal.
    ArrayRef,             ///< Reads values from one array.
    Operator,             ///< Applies one registered operation.
    ComponentExtract,     ///< Selects one component from an operand.
    TupleComponentExtract ///< Selects one tuple and component from an operand.
  } type;

  float64 scalarValue = 0.0;

  DataPath arrayPath;
  DataType sourceDataType = DataType::float64; ///< Selects typed bulk-I/O dispatch.

  // This pointer references the static operator registry.
  const OperatorDef* op = nullptr;

  usize componentIndex = std::numeric_limits<usize>::max();
  usize tupleIndex = std::numeric_limits<usize>::max();
};

/**
 * @brief Returns the supported calculator operators and functions.
 * @return Static registry with process-long lifetime.
 */
SIMPLNXCORE_EXPORT const std::vector<OperatorDef>& getOperatorRegistry();

/**
 * @struct ArrayCalculatorInputValues
 * @brief Stores filter values for an ArrayCalculator execution.
 */
struct SIMPLNXCORE_EXPORT ArrayCalculatorInputValues
{
  DataPath SelectedGroup; ///< Restricts unquoted array-name resolution.
  std::string InfixEquation;
  // This unit choice applies to trigonometric operators.
  CalculatorParameter::AngleUnits Units;
  NumericType ScalarType;
  DataPath CalculatedArray; ///< Identifies the preflight-created output array.
};

/**
 * @class ArrayCalculatorParser
 * @brief Parses and evaluates an infix calculator equation.
 *
 * Literal tuple and component extraction use only expression-sized state.
 * Array-valued evaluation uses bounded bulk-I/O chunks, not array-sized buffers.
 */
class SIMPLNXCORE_EXPORT ArrayCalculatorParser
{
public:
  /**
   * @brief Initializes an equation parser.
   * @param dataStructure Resolves source arrays.
   * @param selectedGroupPath Restricts unquoted array-name resolution.
   * @param infixEquation Defines the equation to parse.
   * @param shouldCancel Signals cancellation during chunk evaluation.
   * @pre dataStructure and shouldCancel outlive this parser.
   *
   * The parser copies selectedGroupPath and infixEquation.
   */
  ArrayCalculatorParser(const DataStructure& dataStructure, const DataPath& selectedGroupPath, const std::string& infixEquation, const std::atomic_bool& shouldCancel);
  /**
   * @brief Destroys the equation parser.
   */
  ~ArrayCalculatorParser() noexcept = default;

  ArrayCalculatorParser(const ArrayCalculatorParser&) = delete;
  ArrayCalculatorParser(ArrayCalculatorParser&&) noexcept = delete;
  ArrayCalculatorParser& operator=(const ArrayCalculatorParser&) = delete;
  ArrayCalculatorParser& operator=(ArrayCalculatorParser&&) noexcept = delete;

  /**
   * @brief Parses and validates the infix equation.
   * @param outTupleShape Receives the parsed output tuple shape.
   * @param outComponentShape Receives the parsed output component shape.
   * @return Validation result.
   *
   * The filter uses both shapes to create the output array during preflight.
   */
  Result<> parseAndValidate(std::vector<usize>& outTupleShape, std::vector<usize>& outComponentShape);

  /**
   * @brief Evaluates the infix equation into an output array.
   * @param dataStructure Contains the output array.
   * @param outputPath Identifies the output array.
   * @param scalarType Selects the output element type.
   * @param units Selects trigonometric angle units.
   * @return Success, or a parsing or evaluation error.
   *
   * Literal tuple and component extraction uses expression-sized state. Array-valued expressions use
   * reusable bounded chunks and bulk transfers.
   *
   * Cancellation is checked before each array-valued chunk. When a checkpoint observes the signal,
   * the method returns success. Earlier output chunks remain. Later chunks are not written.
   *
   * The current type-dispatched bulk-I/O calls do not inspect Result values. A storage failure can
   * leave partial output without an error result.
   */
  Result<> evaluateInto(DataStructure& dataStructure, const DataPath& outputPath, NumericType scalarType, CalculatorParameter::AngleUnits units);

  /**
   * @brief Splits an equation into lexical tokens.
   * @param equation Defines the equation source.
   * @return Tokens with zero-based source positions.
   *
   * An unrecognized character becomes an identifier token. Parsing then reports
   * the contextual error.
   */
  static std::vector<Token> tokenize(const std::string& equation);

private:
  /**
   * @brief Builds reverse-polish notation from the infix equation.
   * @return Parse result.
   */
  Result<> parse();

  const DataStructure& m_DataStructure;
  DataPath m_SelectedGroupPath;
  std::string m_InfixEquation;

  // parse() rebuilds this sequence before validation or evaluation.
  std::vector<RpnItem> m_RpnItems;

  // Validation supplies these shapes to filter preflight.
  std::vector<usize> m_ParsedTupleShape;
  std::vector<usize> m_ParsedComponentShape;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class ArrayCalculator
 * @brief Evaluates the filter-provided calculator equation.
 */
class SIMPLNXCORE_EXPORT ArrayCalculator
{
public:
  /**
   * @brief Initializes the calculator algorithm.
   * @param dataStructure Contains the source and output arrays.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation during chunk evaluation.
   * @param inputValues Defines the calculator expression and output.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ArrayCalculator(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ArrayCalculatorInputValues* inputValues);
  /**
   * @brief Destroys the calculator algorithm.
   */
  ~ArrayCalculator() noexcept;

  ArrayCalculator(const ArrayCalculator&) = delete;
  ArrayCalculator(ArrayCalculator&&) noexcept = delete;
  ArrayCalculator& operator=(const ArrayCalculator&) = delete;
  ArrayCalculator& operator=(ArrayCalculator&&) noexcept = delete;

  /**
   * @brief Evaluates the calculator expression.
   * @return Success, or a parsing or evaluation error.
   *
   * Cancellation is checked only for array-valued chunk evaluation. A scalar-only expression runs
   * to completion. Earlier output chunks remain when cancellation stops array-valued evaluation.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ArrayCalculatorInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
