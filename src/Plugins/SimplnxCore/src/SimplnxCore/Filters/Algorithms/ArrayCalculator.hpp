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
// RAII sentinel for temporary Float64Arrays in the evaluator.
// Move-only. When an Owned CalcBuffer is destroyed, it removes its
// DataArray from the scratch DataStructure via removeData().
// ---------------------------------------------------------------------------
class SIMPLNXCORE_EXPORT CalcBuffer
{
public:
  // --- Factory methods ---

  /**
   * @brief Zero-copy reference to an existing Float64Array in the real DataStructure.
   * Read-only. Destructor: no-op.
   */
  static CalcBuffer borrow(const Float64Array& source);

  /**
   * @brief Allocate a temp Float64Array in tempDS and convert source data from any numeric type.
   * Owned. Destructor: removes the temp array from tempDS.
   */
  static CalcBuffer convertFrom(DataStructure& tempDS, const IDataArray& source, const std::string& name);

  /**
   * @brief Allocate a 1-element temp Float64Array with the given scalar value.
   * Owned. Destructor: removes the temp array from tempDS.
   */
  static CalcBuffer scalar(DataStructure& tempDS, float64 value, const std::string& name);

  /**
   * @brief Allocate an empty temp Float64Array with the given shape.
   * Owned. Destructor: removes the temp array from tempDS.
   */
  static CalcBuffer allocate(DataStructure& tempDS, const std::string& name, std::vector<usize> tupleShape, std::vector<usize> compShape);

  /**
   * @brief Wrap the output DataArray<float64> for direct writing.
   * Not owned. Destructor: no-op.
   */
  static CalcBuffer wrapOutput(DataArray<float64>& outputArray);

  // --- Move-only, non-copyable ---
  CalcBuffer(CalcBuffer&& other) noexcept;
  CalcBuffer& operator=(CalcBuffer&& other) noexcept;
  ~CalcBuffer();

  CalcBuffer(const CalcBuffer&) = delete;
  CalcBuffer& operator=(const CalcBuffer&) = delete;

  // --- Element access ---
  float64 read(usize index) const;
  void write(usize index, float64 value);
  void fill(float64 value);

  // --- Metadata ---
  usize size() const;
  usize numTuples() const;
  usize numComponents() const;
  std::vector<usize> tupleShape() const;
  std::vector<usize> compShape() const;
  bool isScalar() const;
  bool isOwned() const;
  bool isOutputDirect() const;
  void markAsScalar();

  // --- Access underlying array (for final copy to non-float64 output) ---
  const Float64Array& array() const;

private:
  CalcBuffer() = default;

  enum class Storage
  {
    Borrowed,
    Owned,
    OutputDirect
  };

  Storage m_Storage = Storage::Owned;

  // Borrowed: const pointer to source Float64Array in real DataStructure
  const Float64Array* m_BorrowedArray = nullptr;

  // Owned: pointer to temp Float64Array + reference to its DataStructure for cleanup
  DataStructure* m_TempDS = nullptr;
  DataObject::IdType m_ArrayId = 0;
  Float64Array* m_OwnedArray = nullptr;

  // OutputDirect: writable pointer to output DataArray<float64>
  DataArray<float64>* m_OutputArray = nullptr;

  bool m_IsScalar = false;
};

// ---------------------------------------------------------------------------
// A single item in the RPN (reverse-polish notation) evaluation sequence.
// Data-free: stores DataPath references and scalar values, not DataObject IDs.
// ---------------------------------------------------------------------------
struct SIMPLNXCORE_EXPORT RpnItem
{
  enum class Type
  {
    Scalar,
    ArrayRef,
    Operator,
    ComponentExtract,
    TupleComponentExtract
  } type;

  // Scalar
  float64 scalarValue = 0.0;

  // ArrayRef
  DataPath arrayPath;
  DataType sourceDataType = DataType::float64;

  // Operator
  const OperatorDef* op = nullptr;

  // ComponentExtract / TupleComponentExtract
  usize componentIndex = std::numeric_limits<usize>::max();
  usize tupleIndex = std::numeric_limits<usize>::max();
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
  ArrayCalculatorParser(const DataStructure& dataStructure, const DataPath& selectedGroupPath, const std::string& infixEquation, const std::atomic_bool& shouldCancel);
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

private:
  /**
   * @brief Runs the full parsing pipeline (tokenize, merge identifiers,
   * resolve, bracket indexing, minus disambiguation, wrap function args,
   * validate) and populates m_RpnItems.
   */
  Result<> parse();

  const DataStructure& m_DataStructure;
  DataPath m_SelectedGroupPath;
  std::string m_InfixEquation;

  // Populated by parse(); consumed by evaluateInto()
  std::vector<RpnItem> m_RpnItems;

  // Shape info determined during validation
  std::vector<usize> m_ParsedTupleShape;
  std::vector<usize> m_ParsedComponentShape;
  const std::atomic_bool& m_ShouldCancel;
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
