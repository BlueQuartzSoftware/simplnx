#include "ArrayCalculator.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <utility>

using namespace nx::core;

namespace
{

// The evaluator transfers at most this many values per batch. Component count lowers the tuple count.
// Bounded buffers keep out-of-core scratch independent of array size. Bulk transfers avoid one store call per value.
constexpr usize k_ChunkSize = 65536;

/**
 * @struct ParsedItem
 * @brief Stores an infix item before reverse-polish conversion.
 *
 * Parentheses and commas remain in this representation so the parser can
 * validate function arguments before conversion.
 */
struct ParsedItem
{
  /**
   * @enum Kind
   * @brief Identifies the parsed item category.
   */
  enum class Kind
  {
    Scalar,               ///< Stores a scalar literal.
    ArrayRef,             ///< Stores array validation metadata.
    Operator,             ///< Stores an operator definition.
    LParen,               ///< Stores an opening parenthesis.
    RParen,               ///< Stores a closing parenthesis.
    Comma,                ///< Stores a function argument separator.
    ComponentExtract,     ///< Stores a component extraction.
    TupleComponentExtract ///< Stores a tuple and component extraction.
  } kind;

  float64 scalar_value = 0.0;

  // This metadata validates shapes without reading array data.
  DataPath array_path;
  DataType source_data_type = DataType::float64;
  std::vector<usize> array_tuple_shape;
  std::vector<usize> array_comp_shape;

  const OperatorDef* op = nullptr;
  bool is_negative_prefix = false;

  usize component_index = std::numeric_limits<usize>::max();
  usize tuple_index = std::numeric_limits<usize>::max();
};

// The parser uses this static definition to disambiguate unary minus.
const OperatorDef& GetUnaryNegativeOp()
{
  static const OperatorDef sUnaryNeg = {.token = "neg",
                                         .kind = OperatorDef::UnaryPrefix,
                                         .precedence = 4,
                                         .numArgs = 1,
                                         .associativity = OperatorDef::Right,
                                         .trigMode = OperatorDef::None,
                                         .unaryOp = [](double x) { return -x; },
                                         .binaryOp = nullptr};
  return sUnaryNeg;
}

const OperatorDef* FindOperatorByToken(const std::string& token)
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

const OperatorDef* OperatorDefForSymbolToken(TokenType type)
{
  switch(type)
  {
  case TokenType::Plus:
    return FindOperatorByToken("+");
  case TokenType::Minus:
    return FindOperatorByToken("-");
  case TokenType::Star:
    return FindOperatorByToken("*");
  case TokenType::Slash:
    return FindOperatorByToken("/");
  case TokenType::Caret:
    return FindOperatorByToken("^");
  case TokenType::Percent:
    return FindOperatorByToken("%");
  default:
    return nullptr;
  }
}

// The unscoped fallback collects every match so parsing can report ambiguity.
std::vector<DataPath> FindArraysByName(const DataStructure& dataStructure, const std::string& name)
{
  std::vector<DataPath> results;

  auto allPaths = GetAllChildDataPathsRecursive(dataStructure, DataPath{}, DataObject::Type::DataArray);
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

bool IsBinaryOp(const ParsedItem& item)
{
  return item.kind == ParsedItem::Kind::Operator && item.op != nullptr && item.op->kind == OperatorDef::BinaryInfix && !item.is_negative_prefix;
}

/**
 * @brief Groups function arguments for reverse-polish conversion.
 * @param items Receives parsed infix items and inserts argument parentheses.
 *
 * Extra parentheses preserve comma-delimited function boundaries during
 * shunting-yard conversion.
 */
void WrapFunctionArguments(std::vector<ParsedItem>& items)
{
  std::vector<ParsedItem> out;
  out.reserve(items.size() * 2);

  for(size_t i = 0; i < items.size(); ++i)
  {
    const auto& item = items[i];

    if(item.kind == ParsedItem::Kind::Operator && item.op != nullptr && item.op->kind == OperatorDef::Function && i + 1 < items.size() && items[i + 1].kind == ParsedItem::Kind::LParen)
    {
      out.push_back(item);
      out.push_back(items[++i]);
      int depth = 1;
      size_t argStart = out.size();

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

/**
 * @struct ReadSingleElementFunctor
 * @brief Reads one typed source value as float64.
 *
 * The reduction pass makes expression-sized reads. The streaming loop does not
 * use this functor.
 */
struct ReadSingleElementFunctor
{
  /**
   * @brief Reads one source value.
   * @tparam T Specifies the source element type.
   * @param sourceArray Contains the typed source values.
   * @param flatIndex Identifies the source value.
   * @return Source value converted to float64.
   */
  template <typename T>
  float64 operator()(const IDataArray& sourceArray, usize flatIndex)
  {
    const auto& typedSource = dynamic_cast<const DataArray<T>&>(sourceArray);
    return static_cast<float64>(typedSource.at(flatIndex));
  }
};

/**
 * @struct ReadChunkToFloat64Functor
 * @brief Converts one bounded source range to float64 values.
 *
 * The source buffer matches the caller chunk. One bulk read avoids per-value
 * out-of-core access.
 */
struct ReadChunkToFloat64Functor
{
  /**
   * @brief Reads one bounded source range.
   * @tparam T Specifies the source element type.
   * @param sourceArray Contains the typed source values.
   * @param startIndex Identifies the first source value.
   * @param destBuffer Receives converted values.
   * @return The source-store bulk-read result.
   */
  template <typename T>
  Result<> operator()(const IDataArray& sourceArray, usize startIndex, nonstd::span<float64> destBuffer)
  {
    const auto& typedSource = dynamic_cast<const DataArray<T>&>(sourceArray);
    const auto& sourceStore = typedSource.getDataStoreRef();
    const usize count = destBuffer.size();

    // The bool vector specialization has no contiguous data. This buffer avoids that specialization.
    auto rawBuf = std::make_unique<T[]>(count);
    Result<> readResult = sourceStore.copyIntoBuffer(startIndex, nonstd::span<T>(rawBuf.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize i = 0; i < count; i++)
    {
      destBuffer[i] = static_cast<float64>(rawBuf[i]);
    }
    return {};
  }
};

/**
 * @struct WriteChunkFromFloat64Functor
 * @brief Converts one bounded float64 range to the output type.
 *
 * The output buffer matches the caller chunk. One bulk write avoids per-value
 * out-of-core access.
 */
struct WriteChunkFromFloat64Functor
{
  /**
   * @brief Writes one bounded output range.
   * @tparam T Specifies the output element type.
   * @param ds Contains the output array.
   * @param outputPath Identifies the output array.
   * @param startIndex Identifies the first output value.
   * @param srcBuffer Supplies float64 values.
   * @return The output-store bulk-write result.
   */
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const DataPath& outputPath, usize startIndex, nonstd::span<const float64> srcBuffer)
  {
    auto& outputStore = dataStructure.getDataRefAs<DataArray<T>>(outputPath).getDataStoreRef();
    const usize count = srcBuffer.size();

    // The bool vector specialization has no contiguous data. This buffer avoids that specialization.
    auto writeBuf = std::make_unique<T[]>(count);
    for(usize i = 0; i < count; i++)
    {
      writeBuf[i] = static_cast<T>(srcBuffer[i]);
    }
    return outputStore.copyFromBuffer(startIndex, nonstd::span<const T>(writeBuf.get(), count));
  }
};

/**
 * @struct FillScalarResultFunctor
 * @brief Fills one output array with a scalar result.
 *
 * A scalar expression evaluates once. The array fill then broadcasts that value
 * without a per-element calculator loop.
 */
struct FillScalarResultFunctor
{
  /**
   * @brief Fills the output array.
   * @tparam T Specifies the output element type.
   * @param ds Contains the output array.
   * @param outputPath Identifies the output array.
   * @param scalarValue Supplies the result value.
   */
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& outputPath, float64 scalarValue)
  {
    auto& output = dataStructure.getDataRefAs<DataArray<T>>(outputPath);
    output.fill(static_cast<T>(scalarValue));
  }
};

/**
 * @brief Computes the first item index for each reverse-polish subexpression.
 * @param rpn Supplies a valid reverse-polish sequence.
 * @return Start index for the value produced by each reverse-polish item.
 *
 * Reverse-polish subexpressions are contiguous. The reduction and scalar
 * evaluators use these starts without traversing source arrays.
 */
std::vector<usize> ComputeSpanStarts(const std::vector<RpnItem>& rpn)
{
  std::vector<usize> spanStart(rpn.size(), 0);
  std::vector<usize> pending; // The stack stores start indices for pending values.

  for(usize i = 0; i < rpn.size(); i++)
  {
    const RpnItem& item = rpn[i];
    const bool isBinaryOperator = item.type == RpnItem::Type::Operator && item.op != nullptr && item.op->numArgs == 2;
    const bool consumesOneOperand = (item.type == RpnItem::Type::Operator && !isBinaryOperator) || item.type == RpnItem::Type::ComponentExtract || item.type == RpnItem::Type::TupleComponentExtract;

    usize myStart = i;
    if(isBinaryOperator)
    {
      pending.pop_back(); // right operand's start -- the span still begins at the left operand
      myStart = pending.back();
      pending.pop_back();
    }
    else if(consumesOneOperand)
    {
      myStart = pending.back();
      pending.pop_back();
    }
    // Scalar and ArrayRef items are leaves, so myStart remains i.

    spanStart[i] = myStart;
    pending.push_back(myStart);
  }

  return spanStart;
}

/**
 * @brief Evaluates one reverse-polish subexpression at one tuple and component.
 * @param dataStructure Resolves source arrays.
 * @param rpn Supplies reverse-polish items.
 * @param nodeIndex Identifies the subexpression result item.
 * @param spanStart Supplies subexpression start indices.
 * @param tupleIdx Identifies the source tuple.
 * @param compIdx Identifies the source component.
 * @param units Selects trigonometric angle units.
 * @return Scalar value, or an evaluation error.
 *
 * The reduction pass reads at most one value from each source leaf. The
 * streaming loop does not call this function. Component extraction applies its
 * literal index to the complete operand subtree.
 */
Result<float64> EvaluateSingleValue(const DataStructure& dataStructure, const std::vector<RpnItem>& rpn, usize nodeIndex, const std::vector<usize>& spanStart, usize tupleIdx, usize compIdx,
                                    CalculatorParameter::AngleUnits units)
{
  const RpnItem& item = rpn[nodeIndex];

  switch(item.type)
  {
  case RpnItem::Type::Scalar:
    return {item.scalarValue};

  case RpnItem::Type::ArrayRef: {
    const auto* sourceArrayPtr = dataStructure.getDataAs<IDataArray>(item.arrayPath);
    if(sourceArrayPtr == nullptr)
    {
      return MakeErrorResult<float64>(static_cast<int32>(CalculatorErrorCode::InvalidEquation),
                                      fmt::format("Internal error: array '{}' could not be resolved during evaluation.", item.arrayPath.toString()));
    }
    const usize numComps = sourceArrayPtr->getNumberOfComponents();
    const usize flatIndex = tupleIdx * numComps + compIdx;
    const float64 value = ExecuteDataFunction(ReadSingleElementFunctor{}, item.sourceDataType, *sourceArrayPtr, flatIndex);
    return {value};
  }

  case RpnItem::Type::Operator: {
    const OperatorDef* opPtr = item.op;
    if(opPtr == nullptr)
    {
      return MakeErrorResult<float64>(static_cast<int32>(CalculatorErrorCode::InvalidEquation), "Internal error: null operator encountered during scalar evaluation.");
    }

    if(opPtr->numArgs == 1)
    {
      Result<float64> operandResult = EvaluateSingleValue(dataStructure, rpn, nodeIndex - 1, spanStart, tupleIdx, compIdx, units);
      if(operandResult.invalid())
      {
        return operandResult;
      }
      float64 val = operandResult.value();
      if(opPtr->trigMode == OperatorDef::ForwardTrig && units == CalculatorParameter::AngleUnits::Degrees)
      {
        val = val * (std::numbers::pi / 180.0);
      }
      float64 res = opPtr->unaryOp(val);
      if(opPtr->trigMode == OperatorDef::InverseTrig && units == CalculatorParameter::AngleUnits::Degrees)
      {
        res = res * (180.0 / std::numbers::pi);
      }
      return {res};
    }

    // The right operand start identifies the end of the left operand.
    const usize rightIdx = nodeIndex - 1;
    const usize rightStart = spanStart[rightIdx];
    const usize leftIdx = rightStart - 1;

    Result<float64> rightResult = EvaluateSingleValue(dataStructure, rpn, rightIdx, spanStart, tupleIdx, compIdx, units);
    if(rightResult.invalid())
    {
      return rightResult;
    }
    Result<float64> leftResult = EvaluateSingleValue(dataStructure, rpn, leftIdx, spanStart, tupleIdx, compIdx, units);
    if(leftResult.invalid())
    {
      return leftResult;
    }
    return {opPtr->binaryOp(leftResult.value(), rightResult.value())};
  }

  case RpnItem::Type::ComponentExtract: {
    // The literal index applies to the complete operand subtree.
    return EvaluateSingleValue(dataStructure, rpn, nodeIndex - 1, spanStart, tupleIdx, item.componentIndex, units);
  }

  case RpnItem::Type::TupleComponentExtract:
    return MakeErrorResult<float64>(static_cast<int32>(CalculatorErrorCode::InvalidEquation), "Internal error: nested TupleComponentExtract encountered during scalar evaluation.");
  }

  return MakeErrorResult<float64>(static_cast<int32>(CalculatorErrorCode::InvalidEquation), "Internal error: unrecognized RPN item type during scalar evaluation.");
}

/**
 * @struct RpnNodeShape
 * @brief Stores one simulated reverse-polish value shape.
 *
 * Shape simulation reads array metadata only. It does not read array values.
 */
struct RpnNodeShape
{
  bool is_scalar = false;
  usize num_components = 1;
};

/**
 * @struct ShapeSimResult
 * @brief Stores simulated result shape and bounded buffer limits.
 */
struct ShapeSimResult
{
  RpnNodeShape final_shape;
  usize max_stack_depth = 0;
  usize max_components = 1;
};

/**
 * @brief Simulates the shape of one reverse-polish subexpression.
 * @param dataStructure Resolves source array metadata.
 * @param rpn Supplies reverse-polish items.
 * @param startIndex Identifies the first subexpression item.
 * @param endIndex Identifies the final subexpression item.
 * @return Simulated shape and bounded stack requirements, or an expression or component error.
 *
 * The simulation applies chunk-evaluation broadcast rules without reading
 * values. It validates component extraction after operand shape is available.
 */
Result<ShapeSimResult> SimulateShapes(const DataStructure& dataStructure, const std::vector<RpnItem>& rpn, usize startIndex, usize endIndex)
{
  if(rpn.empty() || startIndex > endIndex || endIndex >= rpn.size())
  {
    return MakeErrorResult<ShapeSimResult>(static_cast<int32>(CalculatorErrorCode::InvalidEquation),
                                           fmt::format("Expression item range [{}, {}] is invalid for an expression with {} items.", startIndex, endIndex, rpn.size()));
  }

  ShapeSimResult out;
  std::vector<RpnNodeShape> stack;

  for(usize i = startIndex; i <= endIndex; i++)
  {
    const RpnItem& item = rpn[i];
    switch(item.type)
    {
    case RpnItem::Type::Scalar:
      stack.push_back({true, 1});
      break;

    case RpnItem::Type::ArrayRef: {
      const auto* sourceArrayPtr = dataStructure.getDataAs<IDataArray>(item.arrayPath);
      const usize numComps = (sourceArrayPtr != nullptr) ? sourceArrayPtr->getNumberOfComponents() : 1;
      stack.push_back({false, numComps});
      break;
    }

    case RpnItem::Type::Operator: {
      const OperatorDef* opPtr = item.op;
      if(opPtr == nullptr)
      {
        return MakeErrorResult<ShapeSimResult>(static_cast<int32>(CalculatorErrorCode::InvalidEquation), fmt::format("Expression item {} has no operator definition.", i));
      }
      if(opPtr->numArgs != 1 && opPtr->numArgs != 2)
      {
        return MakeErrorResult<ShapeSimResult>(
            static_cast<int32>(CalculatorErrorCode::InvalidEquation),
            fmt::format("Operator '{}' at expression item {} has an unsupported argument count of {}. Only one or two arguments are supported.", opPtr->token, i, opPtr->numArgs));
      }
      if(stack.size() < static_cast<usize>(opPtr->numArgs))
      {
        return MakeErrorResult<ShapeSimResult>(
            static_cast<int32>(CalculatorErrorCode::InvalidEquation),
            fmt::format("Operator '{}' at expression item {} requires {} operands, but only {} are available. Supply each missing operand.", opPtr->token, i, opPtr->numArgs, stack.size()));
      }
      if(opPtr->numArgs == 2)
      {
        const RpnNodeShape right = stack.back();
        stack.pop_back();
        const RpnNodeShape left = stack.back();
        stack.pop_back();
        if(!left.is_scalar && !right.is_scalar && left.num_components != right.num_components)
        {
          return MakeErrorResult<ShapeSimResult>(
              static_cast<int32>(CalculatorErrorCode::InconsistentCompDims),
              fmt::format("Operator '{}' has array operands with mismatching component counts (left: {}, right: {}). Use arrays with the same number of components or a scalar operand.", opPtr->token,
                          left.num_components, right.num_components));
        }
        const bool resultScalar = left.is_scalar && right.is_scalar;
        const usize numComps = left.is_scalar ? right.num_components : left.num_components;
        stack.push_back({resultScalar, resultScalar ? 1 : numComps});
      }
      else
      {
        const RpnNodeShape operand = stack.back();
        stack.pop_back();
        stack.push_back({operand.is_scalar, operand.is_scalar ? 1 : operand.num_components});
      }
      break;
    }

    case RpnItem::Type::ComponentExtract: {
      if(stack.empty())
      {
        return MakeErrorResult<ShapeSimResult>(static_cast<int32>(CalculatorErrorCode::InvalidEquation),
                                               fmt::format("Component extraction [{}] at expression item {} has no operand.", item.componentIndex, i));
      }
      const RpnNodeShape operand = stack.back();
      stack.pop_back();
      const usize numComps = operand.is_scalar ? 1 : operand.num_components;
      if(item.componentIndex >= numComps)
      {
        return MakeErrorResult<ShapeSimResult>(static_cast<int32>(CalculatorErrorCode::ComponentOutOfRange),
                                               fmt::format("Component index {} is out of range for array with {} components.", item.componentIndex, numComps));
      }
      // Extracting component zero from a scalar preserves scalar broadcasting.
      stack.push_back({operand.is_scalar, 1});
      break;
    }

    case RpnItem::Type::TupleComponentExtract:
      if(stack.empty())
      {
        return MakeErrorResult<ShapeSimResult>(static_cast<int32>(CalculatorErrorCode::InvalidEquation),
                                               fmt::format("Tuple/component extraction [{}, {}] at expression item {} has no operand.", item.tupleIndex, item.componentIndex, i));
      }
      // Literal tuple/component extraction supplies a scalar without reading its value.
      stack.back() = {.is_scalar = true, .num_components = 1};
      break;
    }

    out.max_stack_depth = std::max(out.max_stack_depth, stack.size());
    out.max_components = std::max(out.max_components, stack.back().num_components);
  }

  if(stack.size() != 1)
  {
    return MakeErrorResult<ShapeSimResult>(static_cast<int32>(CalculatorErrorCode::InvalidEquation),
                                           fmt::format("Expression item range [{}, {}] leaves {} values, but exactly one result is required.", startIndex, endIndex, stack.size()));
  }
  out.final_shape = stack.back();
  return {out};
}

/**
 * @brief Finds the tuple count of one reverse-polish operand.
 * @param dataStructure Resolves source arrays.
 * @param rpn Supplies reverse-polish items.
 * @param startIndex Identifies the first operand item.
 * @param endIndex Identifies the final operand item.
 * @return First source-array tuple count, or one for a scalar-only operand.
 *
 * Tuple extraction uses this result to validate its literal tuple index.
 */
usize FindOperandNumTuples(const DataStructure& dataStructure, const std::vector<RpnItem>& rpn, usize startIndex, usize endIndex)
{
  for(usize i = startIndex; i <= endIndex; i++)
  {
    if(rpn[i].type == RpnItem::Type::ArrayRef)
    {
      const auto* sourceArrayPtr = dataStructure.getDataAs<IDataArray>(rpn[i].arrayPath);
      if(sourceArrayPtr != nullptr)
      {
        return sourceArrayPtr->getNumberOfTuples();
      }
    }
  }
  return 1;
}

/**
 * @brief Reduces literal tuple and component extraction to scalar items.
 * @param dataStructure Resolves source arrays.
 * @param rpn Receives reverse-polish items to reduce.
 * @param units Selects trigonometric angle units.
 * @return Success, or a tuple, component, or evaluation error.
 *
 * Inner extractions are reduced first. Each resolution reads only one value per
 * source leaf and does not materialize an array-sized temporary buffer.
 */
Result<> ResolveTupleComponentExtracts(const DataStructure& dataStructure, std::vector<RpnItem>& rpn, CalculatorParameter::AngleUnits units)
{
  while(true)
  {
    usize tceIndex = rpn.size();
    for(usize i = 0; i < rpn.size(); i++)
    {
      if(rpn[i].type == RpnItem::Type::TupleComponentExtract)
      {
        tceIndex = i;
        break;
      }
    }
    if(tceIndex == rpn.size())
    {
      break; // All literal tuple/component extractions are resolved.
    }

    const std::vector<usize> spanStart = ComputeSpanStarts(rpn);
    const usize operandIdx = tceIndex - 1;
    const usize operandStart = spanStart[operandIdx];

    const usize tupleIdx = rpn[tceIndex].tupleIndex;
    const usize compIdx = rpn[tceIndex].componentIndex;

    const usize numTuplesOperand = FindOperandNumTuples(dataStructure, rpn, operandStart, operandIdx);
    if(tupleIdx >= numTuplesOperand)
    {
      return MakeErrorResult(static_cast<int32>(CalculatorErrorCode::TupleOutOfRange), fmt::format("Tuple index {} is out of range for array with {} tuples.", tupleIdx, numTuplesOperand));
    }

    Result<ShapeSimResult> shapeResult = SimulateShapes(dataStructure, rpn, operandStart, operandIdx);
    if(shapeResult.invalid())
    {
      return ConvertResult(std::move(shapeResult));
    }
    const usize numCompsOperand = shapeResult.value().final_shape.is_scalar ? 1 : shapeResult.value().final_shape.num_components;
    if(compIdx >= numCompsOperand)
    {
      return MakeErrorResult(static_cast<int32>(CalculatorErrorCode::ComponentOutOfRange), fmt::format("Component index {} is out of range for array with {} components.", compIdx, numCompsOperand));
    }

    Result<float64> valueResult = EvaluateSingleValue(dataStructure, rpn, operandIdx, spanStart, tupleIdx, compIdx, units);
    if(valueResult.invalid())
    {
      return ConvertResult(std::move(valueResult));
    }

    RpnItem scalarItem;
    scalarItem.type = RpnItem::Type::Scalar;
    scalarItem.scalarValue = valueResult.value();

    rpn.erase(rpn.begin() + static_cast<std::ptrdiff_t>(operandStart), rpn.begin() + static_cast<std::ptrdiff_t>(tceIndex) + 1);
    rpn.insert(rpn.begin() + static_cast<std::ptrdiff_t>(operandStart), scalarItem);
  }

  return {};
}

} // anonymous namespace

const std::vector<OperatorDef>& nx::core::getOperatorRegistry()
{
  static const std::vector<OperatorDef> sRegistry = []() {
    std::vector<OperatorDef> reg;
    reg.reserve(23);

    reg.push_back({"+", OperatorDef::BinaryInfix, 1, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return a + b; }});
    reg.push_back({"-", OperatorDef::BinaryInfix, 1, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return a - b; }});
    reg.push_back({"*", OperatorDef::BinaryInfix, 2, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return a * b; }});
    reg.push_back({"/", OperatorDef::BinaryInfix, 2, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return a / b; }});
    reg.push_back({"%", OperatorDef::BinaryInfix, 2, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return std::fmod(a, b); }});
    reg.push_back({"^", OperatorDef::BinaryInfix, 3, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return std::pow(a, b); }});

    reg.push_back({"abs", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::abs(x); }, nullptr});
    reg.push_back({"sqrt", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::sqrt(x); }, nullptr});
    reg.push_back({"ceil", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::ceil(x); }, nullptr});
    reg.push_back({"floor", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::floor(x); }, nullptr});
    reg.push_back({"exp", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::exp(x); }, nullptr});
    reg.push_back({"ln", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::log(x); }, nullptr});
    reg.push_back({"log10", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::None, [](double x) { return std::log10(x); }, nullptr});

    reg.push_back({"sin", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::ForwardTrig, [](double x) { return std::sin(x); }, nullptr});
    reg.push_back({"cos", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::ForwardTrig, [](double x) { return std::cos(x); }, nullptr});
    reg.push_back({"tan", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::ForwardTrig, [](double x) { return std::tan(x); }, nullptr});

    reg.push_back({"asin", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::InverseTrig, [](double x) { return std::asin(x); }, nullptr});
    reg.push_back({"acos", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::InverseTrig, [](double x) { return std::acos(x); }, nullptr});
    reg.push_back({"atan", OperatorDef::Function, 5, 1, OperatorDef::Left, OperatorDef::InverseTrig, [](double x) { return std::atan(x); }, nullptr});

    reg.push_back({"log", OperatorDef::Function, 5, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double base, double val) { return std::log(val) / std::log(base); }});
    reg.push_back({"root", OperatorDef::Function, 5, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double val, double n) { return std::pow(val, 1.0 / n); }});
    reg.push_back({"min", OperatorDef::Function, 5, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return std::min(a, b); }});
    reg.push_back({"max", OperatorDef::Function, 5, 2, OperatorDef::Left, OperatorDef::None, nullptr, [](double a, double b) { return std::max(a, b); }});

    return reg;
  }();

  return sRegistry;
}

ArrayCalculatorParser::ArrayCalculatorParser(const DataStructure& dataStructure, DataPath selectedGroupPath, std::string infixEquation, const std::atomic_bool& shouldCancel)
: m_DataStructure(dataStructure)
, m_SelectedGroupPath(std::move(selectedGroupPath))
, m_InfixEquation(std::move(infixEquation))
, m_ShouldCancel(shouldCancel)
{
}

std::vector<Token> ArrayCalculatorParser::tokenize(const std::string& equation)
{
  std::vector<Token> tokens;
  const size_t len = equation.size();
  size_t i = 0;

  while(i < len)
  {
    const char c = equation[i];

    if(std::isspace(static_cast<unsigned char>(c)) != 0)
    {
      ++i;
      continue;
    }

    if((std::isdigit(static_cast<unsigned char>(c)) != 0) || (c == '.' && i + 1 < len && (std::isdigit(static_cast<unsigned char>(equation[i + 1])) != 0)))
    {
      size_t start = i;
      bool hasDot = false;
      while(i < len && ((std::isdigit(static_cast<unsigned char>(equation[i])) != 0) || equation[i] == '.'))
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

    if((std::isalpha(static_cast<unsigned char>(c)) != 0) || c == '_')
    {
      size_t start = i;
      while(i < len && ((std::isalnum(static_cast<unsigned char>(equation[i])) != 0) || equation[i] == '_'))
      {
        ++i;
      }
      tokens.push_back({TokenType::Identifier, equation.substr(start, i - start), start});
      continue;
    }

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

    // Parsing reports unknown characters with the surrounding expression context.
    tokens.push_back({TokenType::Identifier, std::string(1, c), i});
    ++i;
  }

  return tokens;
}

Result<> ArrayCalculatorParser::parse()
{
  Result<> result;

  std::vector<Token> tokens = tokenize(m_InfixEquation);
  if(tokens.empty())
  {
    return MakeErrorResult(static_cast<int>(CalculatorErrorCode::EmptyEquation), "The infix expression is empty.");
  }

  // Merge unquoted multiword array names before resolving built-ins and paths.
  {
    std::vector<Token> merged;
    merged.reserve(tokens.size());
    size_t i = 0;
    while(i < tokens.size())
    {
      if(tokens[i].type == TokenType::Identifier)
      {
        size_t runStart = i;
        size_t runEnd = i + 1;
        while(runEnd < tokens.size() && tokens[runEnd].type == TokenType::Identifier)
        {
          ++runEnd;
        }
        size_t runLen = runEnd - runStart;

        if(runLen > 1)
        {
          // Longest matches preserve an array name that contains a shorter name.
          bool foundMatch = false;
          for(size_t len = runLen; len >= 2; --len)
          {
            for(size_t start = runStart; start + len <= runEnd; ++start)
            {
              std::string mergedName = tokens[start].text;
              for(size_t k = start + 1; k < start + len; ++k)
              {
                mergedName += " " + tokens[k].text;
              }

              bool found = false;
              if(!m_SelectedGroupPath.empty())
              {
                found = ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, mergedName);
              }
              if(!found)
              {
                auto paths = FindArraysByName(m_DataStructure, mergedName);
                found = !paths.empty();
              }

              if(found)
              {
                for(size_t k = runStart; k < start; ++k)
                {
                  merged.push_back(tokens[k]);
                }
                merged.push_back({TokenType::Identifier, mergedName, tokens[start].position});
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

  // Resolve identifiers and attach following bracket indexing in one pass.
  std::vector<ParsedItem> items;
  items.reserve(tokens.size());

  for(size_t i = 0; i < tokens.size(); ++i)
  {
    const Token& tok = tokens[i];

    if(tok.type == TokenType::LBracket)
    {
      // Bracket indexes accept [C] or [T, C].
      if(items.empty())
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OrphanedComponent), "Index operator '[' is not paired with a valid array name or closing parenthesis.");
      }

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
      ParsedItem& prevItem = items.back();

      if(prevItem.kind == ParsedItem::Kind::ArrayRef)
      {
        usize numComponents = 1;
        for(usize d : prevItem.array_comp_shape)
        {
          numComponents *= d;
        }
        usize numTuples = 1;
        for(usize d : prevItem.array_tuple_shape)
        {
          numTuples *= d;
        }

        if(bracketNumbers.size() == 1)
        {
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

          ParsedItem ce;
          ce.kind = ParsedItem::Kind::ComponentExtract;
          ce.component_index = compIdx;
          items.push_back(ce);
        }
        else if(bracketNumbers.size() == 2)
        {
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

          ParsedItem tce;
          tce.kind = ParsedItem::Kind::TupleComponentExtract;
          tce.tuple_index = tupleIdx;
          tce.component_index = compIdx;
          items.push_back(tce);
        }
        else
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidComponent), "Bracket index must contain 1 or 2 numbers (e.g. [C] or [T, C]).");
        }
      }
      else if(prevItem.kind == ParsedItem::Kind::RParen)
      {
        if(bracketNumbers.size() == 1)
        {
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
          ce.component_index = compIdx;
          items.push_back(ce);
        }
        else if(bracketNumbers.size() == 2)
        {
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

          ParsedItem tce;
          tce.kind = ParsedItem::Kind::TupleComponentExtract;
          tce.tuple_index = tupleIdx;
          tce.component_index = compIdx;
          items.push_back(tce);
        }
        else
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidComponent), "Sub-expression index must be [C] or [T, C].");
        }
      }
      else
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OrphanedComponent), fmt::format("Index operator '{}' is not paired with a valid array name or closing parenthesis.", tok.text));
      }

      i = j;
      continue;
    }

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

      ParsedItem pi;
      pi.kind = ParsedItem::Kind::Scalar;
      pi.scalar_value = numValue;
      items.push_back(pi);

      {
        bool arrayExists = false;
        if(!m_SelectedGroupPath.empty())
        {
          arrayExists = ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, tok.text);
        }
        if(!arrayExists)
        {
          arrayExists = !FindArraysByName(m_DataStructure, tok.text).empty();
        }
        if(arrayExists)
        {
          result.warnings().push_back(Warning{.code = static_cast<int>(CalculatorWarningCode::AmbiguousNameWarning),
                                              .message = fmt::format("Item '{}' in the infix expression is the name of an array, but it is currently being used as a number."
                                                                     "\nTo treat this item as an array name, please add double quotes around the item (i.e. \"{}\").",
                                                                     tok.text, tok.text)});
        }
      }
      break;
    }

    case TokenType::Identifier: {
      const OperatorDef* opDefPtr = FindOperatorByToken(tok.text);
      if(opDefPtr != nullptr)
      {
        {
          bool arrayExists = false;
          if(!m_SelectedGroupPath.empty())
          {
            arrayExists = ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, tok.text);
          }
          if(!arrayExists)
          {
            arrayExists = !FindArraysByName(m_DataStructure, tok.text).empty();
          }
          if(arrayExists)
          {
            result.warnings().push_back(Warning{.code = static_cast<int>(CalculatorWarningCode::AmbiguousNameWarning),
                                                .message = fmt::format("Item '{}' in the infix expression is the name of an array, but it is currently being used as a mathematical operator."
                                                                       "\nTo treat this item as an array name, please add double quotes around the item (i.e. \"{}\").",
                                                                       tok.text, tok.text)});
          }
        }

        ParsedItem pi;
        pi.kind = ParsedItem::Kind::Operator;
        pi.op = opDefPtr;
        items.push_back(pi);
      }
      else if(tok.text == "pi" || tok.text == "e")
      {
        float64 constValue = (tok.text == "pi") ? std::numbers::pi : std::numbers::e;

        ParsedItem pi;
        pi.kind = ParsedItem::Kind::Scalar;
        pi.scalar_value = constValue;
        items.push_back(pi);

        {
          bool arrayExists = false;
          if(!m_SelectedGroupPath.empty())
          {
            arrayExists = ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, tok.text);
          }
          if(!arrayExists)
          {
            arrayExists = !FindArraysByName(m_DataStructure, tok.text).empty();
          }
          if(arrayExists)
          {
            result.warnings().push_back(Warning{.code = static_cast<int>(CalculatorWarningCode::AmbiguousNameWarning),
                                                .message = fmt::format("Item '{}' in the infix expression is the name of an array, but it is currently being used as a built-in constant."
                                                                       "\nTo treat this item as an array name, please add double quotes around the item (i.e. \"{}\").",
                                                                       tok.text, tok.text)});
          }
        }
      }
      else
      {
        if(!m_SelectedGroupPath.empty() && ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, tok.text))
        {
          DataPath arrayPath = m_SelectedGroupPath.createChildPath(tok.text);
          const auto* dataArrayPtr = m_DataStructure.getDataAs<IDataArray>(arrayPath);
          if(dataArrayPtr == nullptr)
          {
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::UnrecognizedItem), fmt::format("Could not access array '{}' in selected group.", tok.text));
          }

          ParsedItem pi;
          pi.kind = ParsedItem::Kind::ArrayRef;
          pi.array_path = arrayPath;
          pi.source_data_type = dataArrayPtr->getDataType();
          pi.array_tuple_shape = dataArrayPtr->getTupleShape();
          pi.array_comp_shape = dataArrayPtr->getComponentShape();
          items.push_back(pi);
        }
        else
        {
          auto foundPaths = FindArraysByName(m_DataStructure, tok.text);
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
          const auto* dataArrayPtr = m_DataStructure.getDataAs<IDataArray>(foundPaths[0]);
          if(dataArrayPtr == nullptr)
          {
            return MakeErrorResult(static_cast<int>(CalculatorErrorCode::UnrecognizedItem), fmt::format("Could not access array '{}'.", tok.text));
          }

          ParsedItem pi;
          pi.kind = ParsedItem::Kind::ArrayRef;
          pi.array_path = foundPaths[0];
          pi.source_data_type = dataArrayPtr->getDataType();
          pi.array_tuple_shape = dataArrayPtr->getTupleShape();
          pi.array_comp_shape = dataArrayPtr->getComponentShape();
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

      // A single quoted component resolves in the selected group before a DataStructure path lookup.
      if(pathComponents.size() == 1 && !m_SelectedGroupPath.empty())
      {
        DataPath childPath = m_SelectedGroupPath.createChildPath(pathComponents[0]);
        if(m_DataStructure.getDataAs<IDataArray>(childPath) != nullptr)
        {
          quotedPath = childPath;
        }
      }

      const auto* dataArrayPtr = m_DataStructure.getDataAs<IDataArray>(quotedPath);
      if(dataArrayPtr == nullptr)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidArrayName), fmt::format("The item '\"{}\"' is not a valid array path in the DataStructure.", tok.text));
      }

      ParsedItem pi;
      pi.kind = ParsedItem::Kind::ArrayRef;
      pi.array_path = quotedPath;
      pi.source_data_type = dataArrayPtr->getDataType();
      pi.array_tuple_shape = dataArrayPtr->getTupleShape();
      pi.array_comp_shape = dataArrayPtr->getComponentShape();
      items.push_back(pi);
      break;
    }

    case TokenType::Plus:
    case TokenType::Minus:
    case TokenType::Star:
    case TokenType::Slash:
    case TokenType::Caret:
    case TokenType::Percent: {
      const OperatorDef* opDefPtr = OperatorDefForSymbolToken(tok.type);
      if(opDefPtr == nullptr)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidSymbol), fmt::format("Unknown operator symbol '{}'.", tok.text));
      }

      {
        bool arrayExists = false;
        if(!m_SelectedGroupPath.empty())
        {
          arrayExists = ContainsDataArrayName(m_DataStructure, m_SelectedGroupPath, tok.text);
        }
        if(!arrayExists)
        {
          arrayExists = !FindArraysByName(m_DataStructure, tok.text).empty();
        }
        if(arrayExists)
        {
          result.warnings().push_back(Warning{.code = static_cast<int>(CalculatorWarningCode::AmbiguousNameWarning),
                                              .message = fmt::format("Item '{}' in the infix expression is the name of an array, but it is currently being used as a mathematical operator."
                                                                     "\nTo treat this item as an array name, please add double quotes around the item (i.e. \"{}\").",
                                                                     tok.text, tok.text)});
        }
      }

      ParsedItem pi;
      pi.kind = ParsedItem::Kind::Operator;
      pi.op = opDefPtr;
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
      return MakeErrorResult(static_cast<int>(CalculatorErrorCode::InvalidEquation), "Unexpected bracket token encountered.");
    }
    }
  }

  // A leading minus or a minus after an operator, parenthesis, or comma is unary.
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

    bool isUnary = false;
    if(i == 0)
    {
      isUnary = true;
    }
    else
    {
      const auto& prev = items[i - 1];
      if(IsBinaryOp(prev))
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
      item.op = &GetUnaryNegativeOp();
      item.is_negative_prefix = true;
    }
  }

  // Parentheses make each function argument an independent RPN expression.
  WrapFunctionArguments(items);

  // Validate function structure before operator and shape constraints.
  for(size_t i = 0; i < items.size(); ++i)
  {
    const auto& item = items[i];
    if(item.kind == ParsedItem::Kind::Operator && item.op != nullptr && item.op->kind == OperatorDef::Function)
    {
      if(i + 1 >= items.size() || items[i + 1].kind != ParsedItem::Kind::LParen)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OperatorNoOpeningParen), fmt::format("The operator/function '{}' does not have a valid opening parenthesis.", item.op->token));
      }

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
        else if(depth >= 1 && (items[j].kind == ParsedItem::Kind::Scalar || items[j].kind == ParsedItem::Kind::ArrayRef || (items[j].kind == ParsedItem::Kind::Operator && items[j].op != nullptr)))
        {
          hasValueInside = true;
        }
      }
      if(!foundClose)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OperatorNoClosingParen), fmt::format("The operator/function '{}' does not have a valid closing parenthesis.", item.op->token));
      }

      if(!hasValueInside && commaCount == 0)
      {
        if(item.op->numArgs == 2)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NotEnoughArguments),
                                 fmt::format("The function '{}' requires {} arguments, but none were provided.", item.op->token, item.op->numArgs));
        }
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NoNumericArguments), fmt::format("The function '{}' does not have any arguments that simplify down to a number.", item.op->token));
      }

      if(!hasValueInside && commaCount > 0)
      {
        if(item.op->numArgs == 1)
        {
          return MakeErrorResult(static_cast<int>(CalculatorErrorCode::TooManyArguments),
                                 fmt::format("The function '{}' requires {} argument, but more were provided.", item.op->token, item.op->numArgs));
        }
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NoNumericArguments), fmt::format("The function '{}' does not have any arguments that simplify down to a number.", item.op->token));
      }

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

  for(size_t i = 0; i < items.size(); ++i)
  {
    if(items[i].kind == ParsedItem::Kind::Comma)
    {
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

  for(size_t i = 0; i < items.size(); ++i)
  {
    const auto& item = items[i];
    if(!IsBinaryOp(item))
    {
      continue;
    }
    bool hasLeft = false;
    if(i > 0)
    {
      const auto& prev = items[i - 1];
      if(prev.kind == ParsedItem::Kind::Scalar || prev.kind == ParsedItem::Kind::ArrayRef || prev.kind == ParsedItem::Kind::RParen || prev.kind == ParsedItem::Kind::ComponentExtract ||
         prev.kind == ParsedItem::Kind::TupleComponentExtract)
      {
        hasLeft = true;
      }
    }
    if(!hasLeft)
    {
      return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OperatorNoLeftValue), fmt::format("The binary operator '{}' does not have a valid left-hand value.", item.op->token));
    }
    bool hasRight = false;
    if(i + 1 < items.size())
    {
      const auto& next = items[i + 1];
      if(next.kind == ParsedItem::Kind::Scalar || next.kind == ParsedItem::Kind::ArrayRef || next.kind == ParsedItem::Kind::LParen)
      {
        hasRight = true;
      }
      else if(next.kind == ParsedItem::Kind::Operator && next.op != nullptr)
      {
        hasRight = true;
      }
    }
    if(!hasRight)
    {
      return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OperatorNoRightValue), fmt::format("The binary operator '{}' does not have a valid right-hand value.", item.op->token));
    }
  }

  for(size_t i = 0; i < items.size(); ++i)
  {
    const auto& item = items[i];
    if(item.is_negative_prefix)
    {
      bool hasRight = false;
      if(i + 1 < items.size())
      {
        const auto& next = items[i + 1];
        if(next.kind == ParsedItem::Kind::Scalar || next.kind == ParsedItem::Kind::ArrayRef || next.kind == ParsedItem::Kind::LParen)
        {
          hasRight = true;
        }
        else if(next.kind == ParsedItem::Kind::Operator && next.op != nullptr)
        {
          hasRight = true;
        }
      }
      if(!hasRight)
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::OperatorNoRightValue), "The unary negative operator does not have a valid right-hand value.");
      }
    }
  }

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

  // Component and tuple extraction change the effective shape of an array operand.
  std::vector<usize> arrayTupleShape;
  std::vector<usize> arrayCompShape;
  usize arrayNumTuples = 0;
  bool hasArray = false;
  bool hasNumericValue = false;
  bool tupleShapesMatch = true;

  for(size_t vi = 0; vi < items.size(); ++vi)
  {
    const auto& item = items[vi];
    if(item.kind == ParsedItem::Kind::Scalar || item.kind == ParsedItem::Kind::ArrayRef)
    {
      hasNumericValue = true;
    }
    if(item.kind == ParsedItem::Kind::ArrayRef)
    {
      std::vector<usize> ts = item.array_tuple_shape;
      std::vector<usize> cs = item.array_comp_shape;

      if(vi + 1 < items.size() && items[vi + 1].kind == ParsedItem::Kind::ComponentExtract)
      {
        cs = {1};
      }
      else if(vi + 1 < items.size() && items[vi + 1].kind == ParsedItem::Kind::TupleComponentExtract)
      {
        // A literal tuple/component extraction is scalar and does not supply output shape.
        continue;
      }

      usize nt = 1;
      for(usize d : ts)
      {
        nt *= d;
      }

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

  if(!hasNumericValue)
  {
    return MakeErrorResult(static_cast<int>(CalculatorErrorCode::NoNumericArguments), "The expression does not have any arguments that simplify down to a number.");
  }

  // Convert the validated infix items to reverse-polish notation.
  m_RpnItems.clear();
  std::vector<ParsedItem> opStack;

  for(const auto& item : items)
  {
    switch(item.kind)
    {
    case ParsedItem::Kind::Scalar: {
      RpnItem rpn;
      rpn.type = RpnItem::Type::Scalar;
      rpn.scalarValue = item.scalar_value;
      m_RpnItems.push_back(rpn);
      break;
    }

    case ParsedItem::Kind::ArrayRef: {
      RpnItem rpn;
      rpn.type = RpnItem::Type::ArrayRef;
      rpn.arrayPath = item.array_path;
      rpn.sourceDataType = item.source_data_type;
      m_RpnItems.push_back(rpn);
      break;
    }

    case ParsedItem::Kind::LParen: {
      opStack.push_back(item);
      break;
    }

    case ParsedItem::Kind::RParen: {
      while(!opStack.empty() && opStack.back().kind != ParsedItem::Kind::LParen)
      {
        const auto& top = opStack.back();
        RpnItem rpn;
        rpn.type = RpnItem::Type::Operator;
        rpn.op = top.op;
        m_RpnItems.push_back(rpn);
        opStack.pop_back();
      }
      if(opStack.empty())
      {
        return MakeErrorResult(static_cast<int>(CalculatorErrorCode::MismatchedParentheses),
                               fmt::format("One or more parentheses are mismatched in the chosen infix expression '{}'.", m_InfixEquation));
      }
      opStack.pop_back();
      // Complete the function call before a following postfix extraction consumes its result.
      if(!opStack.empty() && opStack.back().kind == ParsedItem::Kind::Operator && opStack.back().op != nullptr && opStack.back().op->kind == OperatorDef::Function)
      {
        RpnItem rpn;
        rpn.type = RpnItem::Type::Operator;
        rpn.op = opStack.back().op;
        m_RpnItems.push_back(rpn);
        opStack.pop_back();
      }
      break;
    }

    case ParsedItem::Kind::Comma: {
      while(!opStack.empty() && opStack.back().kind != ParsedItem::Kind::LParen)
      {
        const auto& top = opStack.back();
        RpnItem rpn;
        rpn.type = RpnItem::Type::Operator;
        rpn.op = top.op;
        m_RpnItems.push_back(rpn);
        opStack.pop_back();
      }
      break;
    }

    case ParsedItem::Kind::Operator: {
      const OperatorDef* incomingOpPtr = item.is_negative_prefix ? &GetUnaryNegativeOp() : item.op;
      int incomingPrec = incomingOpPtr->precedence;
      bool isLeftAssoc = (incomingOpPtr->associativity == OperatorDef::Left);

      while(!opStack.empty() && opStack.back().kind == ParsedItem::Kind::Operator)
      {
        const auto& topItem = opStack.back();
        const OperatorDef* topOpPtr = topItem.is_negative_prefix ? &GetUnaryNegativeOp() : topItem.op;
        int topPrec = topOpPtr->precedence;

        if(topPrec > incomingPrec || (topPrec == incomingPrec && isLeftAssoc))
        {
          RpnItem rpn;
          rpn.type = RpnItem::Type::Operator;
          rpn.op = topOpPtr;
          m_RpnItems.push_back(rpn);
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
      RpnItem rpn;
      rpn.type = RpnItem::Type::ComponentExtract;
      rpn.componentIndex = item.component_index;
      m_RpnItems.push_back(rpn);
      break;
    }

    case ParsedItem::Kind::TupleComponentExtract: {
      RpnItem rpn;
      rpn.type = RpnItem::Type::TupleComponentExtract;
      rpn.tupleIndex = item.tuple_index;
      rpn.componentIndex = item.component_index;
      m_RpnItems.push_back(rpn);
      break;
    }
    }
  }

  while(!opStack.empty())
  {
    const auto& top = opStack.back();
    if(top.kind == ParsedItem::Kind::LParen)
    {
      return MakeErrorResult(static_cast<int>(CalculatorErrorCode::MismatchedParentheses), fmt::format("One or more parentheses are mismatched in the chosen infix expression '{}'.", m_InfixEquation));
    }
    const OperatorDef* topOpPtr = top.is_negative_prefix ? &GetUnaryNegativeOp() : top.op;
    RpnItem rpn;
    rpn.type = RpnItem::Type::Operator;
    rpn.op = topOpPtr;
    m_RpnItems.push_back(rpn);
    opStack.pop_back();
  }

  // The complete expression determines output shape; an indexed scalar can be one operand of an array result.
  Result<ShapeSimResult> shapeResult = SimulateShapes(m_DataStructure, m_RpnItems, 0, m_RpnItems.size() - 1);
  if(shapeResult.invalid())
  {
    return MergeResults(std::move(result), ConvertResult(std::move(shapeResult)));
  }

  const RpnNodeShape& finalShape = shapeResult.value().final_shape;
  if(finalShape.is_scalar)
  {
    m_ParsedTupleShape = {1};
    m_ParsedComponentShape = {1};
  }
  else
  {
    m_ParsedTupleShape = tupleShapesMatch ? arrayTupleShape : std::vector<usize>{arrayNumTuples};
    usize arrayNumComponents = 1;
    for(usize dimension : arrayCompShape)
    {
      arrayNumComponents *= dimension;
    }
    m_ParsedComponentShape = finalShape.num_components == arrayNumComponents ? arrayCompShape : std::vector<usize>{finalShape.num_components};
  }

  return result;
}

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

Result<> ArrayCalculatorParser::evaluateInto(DataStructure& dataStructure, const DataPath& outputPath, NumericType scalarType, CalculatorParameter::AngleUnits units,
                                             const IFilter::MessageHandler& messageHandler)
{
  Result<> parseResult = parse();
  if(parseResult.invalid())
  {
    return parseResult;
  }

  const DataType outputDataType = ConvertNumericTypeToDataType(scalarType);

  // A local RPN copy permits literal tuple/component reduction without changing parser state.
  std::vector<RpnItem> rpn = m_RpnItems;
  Result<> reduceResult = ResolveTupleComponentExtracts(m_DataStructure, rpn, units);
  if(reduceResult.invalid())
  {
    return reduceResult;
  }

  Result<ShapeSimResult> shapeResult = SimulateShapes(m_DataStructure, rpn, 0, rpn.size() - 1);
  if(shapeResult.invalid())
  {
    return ConvertResult(std::move(shapeResult));
  }
  const ShapeSimResult& shapeInfo = shapeResult.value();

  if(shapeInfo.final_shape.is_scalar)
  {
    const std::vector<usize> spanStart = ComputeSpanStarts(rpn);
    Result<float64> scalarResult = EvaluateSingleValue(m_DataStructure, rpn, rpn.size() - 1, spanStart, 0, 0, units);
    if(scalarResult.invalid())
    {
      return ConvertResult(std::move(scalarResult));
    }
    ExecuteDataFunction(FillScalarResultFunctor{}, outputDataType, dataStructure, outputPath, scalarResult.value());
    return parseResult;
  }

  // Array-valued evaluation reuses buffers bounded by chunk size and RPN stack depth.
  auto& outputArray = dataStructure.getDataRefAs<IDataArray>(outputPath);
  const usize outputNumTuples = outputArray.getNumberOfTuples();
  const usize outputNumComps = outputArray.getNumberOfComponents();
  const usize tuplesPerChunk = std::max<usize>(1, k_ChunkSize / std::max<usize>(1, outputNumComps));

  // Resolve paths once so chunk evaluation does not repeat DataStructure lookup.
  std::vector<const IDataArray*> resolvedArrays(rpn.size(), nullptr);
  for(usize i = 0; i < rpn.size(); i++)
  {
    if(rpn[i].type == RpnItem::Type::ArrayRef)
    {
      resolvedArrays[i] = m_DataStructure.getDataAs<IDataArray>(rpn[i].arrayPath);
      if(resolvedArrays[i] == nullptr)
      {
        return MakeErrorResult(static_cast<int32>(CalculatorErrorCode::InvalidEquation),
                               fmt::format("Internal error: array '{}' could not be resolved during evaluation.", rpn[i].arrayPath.toString()));
      }
    }
  }

  /**
   * @struct ChunkStackEntry
   * @brief Stores one reusable value during chunk evaluation.
   *
   * Capacity stays bounded by one chunk and the maximum expression component count.
   */
  struct ChunkStackEntry
  {
    bool is_scalar = false;
    float64 scalar_value = 0.0;
    usize num_components = 1;
    std::vector<float64> buffer;
  };
  std::vector<ChunkStackEntry> stack(shapeInfo.max_stack_depth);
  for(auto& entry : stack)
  {
    entry.buffer.reserve(tuplesPerChunk * shapeInfo.max_components);
  }
  std::vector<float64> outWriteBuf;
  outWriteBuf.reserve(tuplesPerChunk * outputNumComps);

  ThrottledMessageHandler progressThrottle(messageHandler);
  progressThrottle.reset(outputNumTuples, "Evaluating Array Expression");
  for(usize tupleStart = 0; tupleStart < outputNumTuples; tupleStart += tuplesPerChunk)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize tupleCount = std::min(tuplesPerChunk, outputNumTuples - tupleStart);
    usize depth = 0;

    for(usize rpnIdx = 0; rpnIdx < rpn.size(); rpnIdx++)
    {
      const RpnItem& item = rpn[rpnIdx];
      switch(item.type)
      {
      case RpnItem::Type::Scalar: {
        stack[depth].is_scalar = true;
        stack[depth].scalar_value = item.scalarValue;
        depth++;
        break;
      }

      case RpnItem::Type::ArrayRef: {
        ChunkStackEntry& entry = stack[depth];
        const IDataArray* sourceArrayPtr = resolvedArrays[rpnIdx];
        const usize numComps = sourceArrayPtr->getNumberOfComponents();
        const usize count = tupleCount * numComps;
        entry.is_scalar = false;
        entry.num_components = numComps;
        entry.buffer.resize(count);
        if(item.sourceDataType == DataType::float64)
        {
          const auto& typedArray = dynamic_cast<const Float64Array&>(*sourceArrayPtr);
          Result<> readResult = typedArray.getDataStoreRef().copyIntoBuffer(tupleStart * numComps, nonstd::span<float64>(entry.buffer.data(), count));
          if(readResult.invalid())
          {
            return readResult;
          }
        }
        else
        {
          Result<> readResult = ExecuteDataFunction(ReadChunkToFloat64Functor{}, item.sourceDataType, *sourceArrayPtr, tupleStart * numComps, nonstd::span<float64>(entry.buffer.data(), count));
          if(readResult.invalid())
          {
            return readResult;
          }
        }
        depth++;
        break;
      }

      case RpnItem::Type::Operator: {
        const OperatorDef* opPtr = item.op;
        if(opPtr->numArgs == 1)
        {
          ChunkStackEntry& operand = stack[depth - 1];
          if(operand.is_scalar)
          {
            float64 val = operand.scalar_value;
            if(opPtr->trigMode == OperatorDef::ForwardTrig && units == CalculatorParameter::AngleUnits::Degrees)
            {
              val = val * (std::numbers::pi / 180.0);
            }
            float64 res = opPtr->unaryOp(val);
            if(opPtr->trigMode == OperatorDef::InverseTrig && units == CalculatorParameter::AngleUnits::Degrees)
            {
              res = res * (180.0 / std::numbers::pi);
            }
            operand.scalar_value = res;
          }
          else
          {
            const usize count = tupleCount * operand.num_components;
            for(usize i = 0; i < count; i++)
            {
              float64 val = operand.buffer[i];
              if(opPtr->trigMode == OperatorDef::ForwardTrig && units == CalculatorParameter::AngleUnits::Degrees)
              {
                val = val * (std::numbers::pi / 180.0);
              }
              float64 res = opPtr->unaryOp(val);
              if(opPtr->trigMode == OperatorDef::InverseTrig && units == CalculatorParameter::AngleUnits::Degrees)
              {
                res = res * (180.0 / std::numbers::pi);
              }
              operand.buffer[i] = res;
            }
          }
        }
        else
        {
          ChunkStackEntry& right = stack[depth - 1];
          ChunkStackEntry& left = stack[depth - 2];
          depth--; // pop right; the result replaces left, which becomes the new top of stack

          if(left.is_scalar && right.is_scalar)
          {
            left.scalar_value = opPtr->binaryOp(left.scalar_value, right.scalar_value);
          }
          else
          {
            const bool leftWasScalar = left.is_scalar;
            const bool rightWasScalar = right.is_scalar;
            const float64 leftScalarVal = left.scalar_value;
            const float64 rightScalarVal = right.scalar_value;
            const usize numComps = leftWasScalar ? right.num_components : left.num_components;
            const usize count = tupleCount * numComps;

            left.buffer.resize(count);
            for(usize i = 0; i < count; i++)
            {
              const float64 lv = leftWasScalar ? leftScalarVal : left.buffer[i];
              const float64 rv = rightWasScalar ? rightScalarVal : right.buffer[i];
              left.buffer[i] = opPtr->binaryOp(lv, rv);
            }
            left.is_scalar = false;
            left.num_components = numComps;
          }
        }
        break;
      }

      case RpnItem::Type::ComponentExtract: {
        ChunkStackEntry& operand = stack[depth - 1];
        const usize compIdx = item.componentIndex;
        if(!operand.is_scalar)
        {
          const usize numComps = operand.num_components;
          for(usize t = 0; t < tupleCount; t++)
          {
            operand.buffer[t] = operand.buffer[t * numComps + compIdx];
          }
          operand.buffer.resize(tupleCount);
          operand.num_components = 1;
        }
        // Component zero does not change a scalar operand.
        break;
      }

      case RpnItem::Type::TupleComponentExtract:
        // Reduction removes tuple/component extraction items before chunk evaluation.
        break;
      }
    }

    const ChunkStackEntry& result = stack[0];
    outWriteBuf.resize(tupleCount * outputNumComps);
    if(result.is_scalar)
    {
      std::fill(outWriteBuf.begin(), outWriteBuf.end(), result.scalar_value);
    }
    else
    {
      std::copy(result.buffer.begin(), result.buffer.begin() + static_cast<std::ptrdiff_t>(tupleCount * outputNumComps), outWriteBuf.begin());
    }

    if(outputDataType == DataType::float64)
    {
      auto& outputStore = dataStructure.getDataRefAs<Float64Array>(outputPath).getDataStoreRef();
      Result<> writeResult = outputStore.copyFromBuffer(tupleStart * outputNumComps, nonstd::span<const float64>(outWriteBuf.data(), tupleCount * outputNumComps));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
    else
    {
      Result<> writeResult = ExecuteDataFunction(WriteChunkFromFloat64Functor{}, outputDataType, dataStructure, outputPath, tupleStart * outputNumComps,
                                                 nonstd::span<const float64>(outWriteBuf.data(), tupleCount * outputNumComps));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
    progressThrottle.updatePercent(tupleStart + tupleCount);
  }

  return parseResult;
}

ArrayCalculator::ArrayCalculator(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ArrayCalculatorInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

const std::atomic_bool& ArrayCalculator::getCancel()
{
  return m_ShouldCancel;
}

Result<> ArrayCalculator::operator()()
{
  ArrayCalculatorParser parser(m_DataStructure, m_InputValues->SelectedGroup, m_InputValues->InfixEquation, m_ShouldCancel);
  return parser.evaluateInto(m_DataStructure, m_InputValues->CalculatedArray, m_InputValues->ScalarType, m_InputValues->Units, m_MessageHandler);
}
