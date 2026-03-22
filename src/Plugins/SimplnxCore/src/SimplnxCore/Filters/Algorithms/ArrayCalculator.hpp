#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/CalculatorParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"

#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace nx::core
{

// ---------------------------------------------------------------------------
// Error codes preserved from the legacy CalculatorItem::ErrorCode enum
// ---------------------------------------------------------------------------
enum class CalculatorErrorCode : int
{
  Success = 0,
  InvalidEquation = -4009,
  InvalidComponent = -4010,
  EmptyEquation = -4011,
  EmptyCalArray = -4012,
  EmptySelMatrix = -4013,
  LostAttrMatrix = -4014,
  IncorrectTupleCount = -4015,
  InconsistentTuples = -4016,
  UnrecognizedItem = -4017,
  MismatchedParentheses = -4018,
  UnexpectedOutput = -4019,
  ComponentOutOfRange = -4020,
  InvalidArrayName = -4022,
  InconsistentIndexing = -4023,
  InconsistentCompDims = -4024,
  AttrArrayZeroTuplesWarning = -4025,
  OrphanedComponent = -4026,
  OperatorNoLeftValue = -4027,
  OperatorNoRightValue = -4028,
  OperatorNoOpeningParen = -4029,
  OperatorNoClosingParen = -4030,
  NoNumericArguments = -4031,
  MissingArguments = -4032,
  NotEnoughArguments = -4033,
  TooManyArguments = -4034,
  InvalidSymbol = -4035,
  NoPrecedingUnaryOperator = -4036,
  InvalidOutputArrayType = -4037,
  AttributeMatrixInsertionError = -4038,
  AmbiguousArrayName = -4039,
  TupleOutOfRange = -4040
};

// ---------------------------------------------------------------------------
// Warning codes preserved from the legacy CalculatorItem::WarningCode enum
// ---------------------------------------------------------------------------
enum class CalculatorWarningCode : int
{
  None = 0,
  NumericValueWarning = -5010,
  AmbiguousNameWarning = -5011
};

// ---------------------------------------------------------------------------
// Lexer token types
// ---------------------------------------------------------------------------
enum class TokenType
{
  Number,
  Identifier,
  QuotedString,
  Plus,
  Minus,
  Star,
  Slash,
  Caret,
  Percent,
  LParen,
  RParen,
  LBracket,
  RBracket,
  Comma
};

// ---------------------------------------------------------------------------
// A single token produced by the lexer
// ---------------------------------------------------------------------------
struct SIMPLNXCORE_EXPORT Token
{
  TokenType type;
  std::string text;
  size_t position = 0;
};

// ---------------------------------------------------------------------------
// Definition of an operator or function in the calculator language
// ---------------------------------------------------------------------------
struct SIMPLNXCORE_EXPORT OperatorDef
{
  std::string token;

  enum Kind
  {
    BinaryInfix,
    Function,
    UnaryPrefix
  } kind;

  int precedence;
  int numArgs;

  enum Associativity
  {
    Left,
    Right
  } associativity = Left;

  enum TrigMode
  {
    None,
    ForwardTrig,
    InverseTrig
  } trigMode = None;

  std::function<double(double)> unaryOp;
  std::function<double(double, double)> binaryOp;
};

// ---------------------------------------------------------------------------
// A value that lives on the evaluation stack (either a scalar or an array)
// ---------------------------------------------------------------------------
struct SIMPLNXCORE_EXPORT CalcValue
{
  enum class Kind
  {
    Number,
    Array
  } kind;

  DataObject::IdType arrayId;
};

// ---------------------------------------------------------------------------
// A single item in the RPN (reverse-polish notation) evaluation sequence
// ---------------------------------------------------------------------------
struct SIMPLNXCORE_EXPORT RpnItem
{
  enum class Type
  {
    Value,
    Operator,
    ComponentExtract
  } type;

  CalcValue value;
  const OperatorDef* op = nullptr;
  int componentIndex = -1;
};

// ---------------------------------------------------------------------------
// Returns the global table of all supported operators and functions
// ---------------------------------------------------------------------------
SIMPLNXCORE_EXPORT const std::vector<OperatorDef>& getOperatorRegistry();

// ---------------------------------------------------------------------------
// Input values passed from ArrayCalculatorFilter to the algorithm
// ---------------------------------------------------------------------------
struct SIMPLNXCORE_EXPORT ArrayCalculatorInputValues
{
  DataPath SelectedGroup;
  std::string InfixEquation;
  CalculatorParameter::AngleUnits Units;
  NumericType ScalarType;
  DataPath CalculatedArray;
};

// ---------------------------------------------------------------------------
// Parses and validates an infix calculator equation, then evaluates it
// ---------------------------------------------------------------------------
class SIMPLNXCORE_EXPORT ArrayCalculatorParser
{
public:
  ArrayCalculatorParser(const DataStructure& dataStructure, const DataPath& selectedGroupPath, const std::string& infixEquation, bool isPreflight);
  ~ArrayCalculatorParser() noexcept = default;

  ArrayCalculatorParser(const ArrayCalculatorParser&) = delete;
  ArrayCalculatorParser(ArrayCalculatorParser&&) noexcept = delete;
  ArrayCalculatorParser& operator=(const ArrayCalculatorParser&) = delete;
  ArrayCalculatorParser& operator=(ArrayCalculatorParser&&) noexcept = delete;

  /**
   * @brief Tokenises, parses, and validates the infix equation.
   * On success the output tuple and component shapes are written to the
   * out-parameters so the filter can create the output array in preflight.
   */
  Result<> parseAndValidate(std::vector<usize>& outTupleShape, std::vector<usize>& outComponentShape);

  /**
   * @brief Evaluates the already-parsed equation and writes the result into
   * the output array at @p outputPath inside @p dataStructure.
   */
  Result<> evaluateInto(DataStructure& dataStructure, const DataPath& outputPath, NumericType scalarType, CalculatorParameter::AngleUnits units);

  /**
   * @brief Pure lexer -- splits an equation string into tokens.
   */
  static std::vector<Token> tokenize(const std::string& equation);

  // Expose temp DataStructure for evaluator (Task 1e will use this)
  DataStructure& getTempDataStructure()
  {
    return m_TempDataStructure;
  }

private:
  /**
   * @brief Runs the full parsing pipeline (tokenize, merge identifiers,
   * resolve, bracket indexing, minus disambiguation, wrap function args,
   * validate) and populates m_ParsedItems.
   */
  Result<> parse();

  /**
   * @brief Creates a unique scratch name for temporary arrays.
   */
  std::string nextScratchName();

  /**
   * @brief Creates a Float64Array in m_TempDataStructure from a source
   * IDataArray, converting all values to double.  When m_IsPreflight is
   * true the array is allocated but data is not copied.
   * @return the DataObject::IdType of the newly created array
   */
  DataObject::IdType copyArrayToTemp(const IDataArray& sourceArray);

  /**
   * @brief Creates a 1-element Float64Array in m_TempDataStructure with the
   * given scalar value.
   * @return the DataObject::IdType of the newly created array
   */
  DataObject::IdType createScalarInTemp(double value);

  const DataStructure& m_DataStructure;
  DataStructure m_TempDataStructure;
  DataPath m_SelectedGroupPath;
  std::string m_InfixEquation;
  bool m_IsPreflight;
  usize m_ScratchCounter = 0;

  // Populated by parse(); consumed by evaluateInto() (Task 1e)
  std::vector<RpnItem> m_RpnItems;

  // Shape info determined during validation
  std::vector<usize> m_ParsedTupleShape;
  std::vector<usize> m_ParsedComponentShape;
};

// ---------------------------------------------------------------------------
// Top-level algorithm class invoked by ArrayCalculatorFilter::executeImpl()
// ---------------------------------------------------------------------------
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
