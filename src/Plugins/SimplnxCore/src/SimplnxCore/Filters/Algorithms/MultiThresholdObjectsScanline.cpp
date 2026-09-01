#include "MultiThresholdObjectsScanline.hpp"

#include "MultiThresholdObjects.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ArrayThreshold.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>

using namespace nx::core;

// Scanline evaluates a complete threshold tree per bounded tuple chunk. This
// avoids disk-backed per-element access and cell-count temporary vectors.

namespace
{
/**
 * @brief Specifies the tuple count in one bulk-I/O chunk.
 */
constexpr usize k_ChunkSize = 65536;

/**
 * @brief Applies one comparison to a buffered input chunk.
 * @tparam CompT Specifies the comparison functor.
 * @tparam InputT Specifies the input scalar type.
 * @tparam MaskT Specifies the output mask type.
 * @param inputBuffer Provides flat input values.
 * @param numComponents Specifies input components per tuple.
 * @param componentIndex Specifies the compared component.
 * @param chunkTuples Specifies tuple count in the chunk.
 * @param trueValue Represents a match.
 * @param falseValue Represents a nonmatch.
 * @param outputBuffer Receives mask values.
 * @param comparisonValue Specifies the typed comparison value.
 */
template <class CompT, class InputT, class MaskT>
void filterChunkWithComparison(const InputT* inputBuffer, usize numComponents, usize componentIndex, usize chunkTuples, MaskT trueValue, MaskT falseValue, MaskT* outputBuffer, InputT comparisonValue)
{
  for(usize i = 0; i < chunkTuples; ++i)
  {
    InputT inputValue = inputBuffer[i * numComponents + componentIndex];
    outputBuffer[i] = CompT{}(inputValue, comparisonValue) ? trueValue : falseValue;
  }
}

/**
 * @brief Selects a comparison operator for one buffered input chunk.
 * @tparam InputT Specifies the input scalar type.
 * @tparam MaskT Specifies the output mask type.
 * @param compOperator Specifies the comparison operator.
 * @param inputBuffer Provides flat input values.
 * @param numComponents Specifies input components per tuple.
 * @param componentIndex Specifies the compared component.
 * @param chunkTuples Specifies tuple count in the chunk.
 * @param trueValue Represents a match.
 * @param falseValue Represents a nonmatch.
 * @param outputBuffer Receives mask values.
 * @param comparisonValue Specifies the typed comparison value.
 */
template <class InputT, class MaskT>
void filterChunk(ArrayThreshold::ComparisonType compOperator, const InputT* inputBuffer, usize numComponents, usize componentIndex, usize chunkTuples, MaskT trueValue, MaskT falseValue,
                 MaskT* outputBuffer, InputT comparisonValue)
{
  switch(compOperator)
  {
  case ArrayThreshold::ComparisonType::LessThan:
    filterChunkWithComparison<std::less<>, InputT, MaskT>(inputBuffer, numComponents, componentIndex, chunkTuples, trueValue, falseValue, outputBuffer, comparisonValue);
    break;
  case ArrayThreshold::ComparisonType::GreaterThan:
    filterChunkWithComparison<std::greater<>, InputT, MaskT>(inputBuffer, numComponents, componentIndex, chunkTuples, trueValue, falseValue, outputBuffer, comparisonValue);
    break;
  case ArrayThreshold::ComparisonType::Operator_Equal:
    filterChunkWithComparison<std::equal_to<>, InputT, MaskT>(inputBuffer, numComponents, componentIndex, chunkTuples, trueValue, falseValue, outputBuffer, comparisonValue);
    break;
  case ArrayThreshold::ComparisonType::Operator_NotEqual:
    filterChunkWithComparison<std::not_equal_to<>, InputT, MaskT>(inputBuffer, numComponents, componentIndex, chunkTuples, trueValue, falseValue, outputBuffer, comparisonValue);
    break;
  default: {
    std::string errorMessage = fmt::format("MultiThresholdObjects Comparison Operator not understood: '{}'", static_cast<int>(compOperator));
    throw std::runtime_error(errorMessage);
  }
  }
}

/**
 * @brief Merges a child threshold chunk into the current result.
 * @tparam MaskT Specifies the output mask type.
 * @param chunkTuples Specifies tuple count in the chunk.
 * @param currentBuffer Receives merged mask values.
 * @param unionOperator Selects logical OR or AND.
 * @param newBuffer Provides child mask values.
 * @param inverse Inverts child values before merging when true.
 * @param trueValue Represents a match.
 * @param falseValue Represents a nonmatch.
 * @param shouldCancel Stops later tuples when true.
 */
template <typename MaskT>
void insertThresholdChunk(usize chunkTuples, MaskT* currentBuffer, IArrayThreshold::UnionOperator unionOperator, MaskT* newBuffer, bool inverse, MaskT trueValue, MaskT falseValue,
                          const std::atomic_bool& shouldCancel)
{
  for(usize i = 0; i < chunkTuples; i++)
  {
    if((i % 4096) == 0 && shouldCancel)
    {
      return;
    }
    if(inverse)
    {
      newBuffer[i] = (newBuffer[i] == trueValue) ? falseValue : trueValue;
    }

    if(IArrayThreshold::UnionOperator::Or == unionOperator)
    {
      currentBuffer[i] = (currentBuffer[i] == trueValue || newBuffer[i] == trueValue) ? trueValue : falseValue;
    }
    else if(currentBuffer[i] == falseValue || newBuffer[i] == falseValue)
    {
      currentBuffer[i] = falseValue;
    }
  }
}

/**
 * @struct ChunkedThresholdHelper
 * @brief Reads one input chunk and applies one threshold comparison.
 */
struct ChunkedThresholdHelper
{
  /**
   * @brief Evaluates one typed leaf threshold chunk.
   * @tparam InputT Specifies the threshold input type.
   * @tparam MaskT Specifies the output mask type.
   * @param iDataArray Provides threshold input values.
   * @param compOperator Specifies the comparison operator.
   * @param compValue Specifies the comparison value.
   * @param componentIndex Specifies the compared component.
   * @param chunkStartTuple Specifies the first tuple index.
   * @param chunkTuples Specifies tuple count in the chunk.
   * @param trueValue Represents a match.
   * @param falseValue Represents a nonmatch.
   * @param tempBuffer Receives chunk mask values.
   * @return Error from the input store, or success.
   */
  template <typename InputT, typename MaskT>
  Result<> operator()(const IDataArray& iDataArray, ArrayThreshold::ComparisonType compOperator, ArrayThreshold::ComparisonValue compValue, usize componentIndex, usize chunkStartTuple,
                      usize chunkTuples, MaskT trueValue, MaskT falseValue, MaskT* tempBuffer)
  {
    const auto& inputStore = iDataArray.template getIDataStoreRefAs<AbstractDataStore<InputT>>();
    usize numComponents = inputStore.getNumberOfComponents();

    // The array buffer avoids the std::vector<bool> specialization. Bulk I/O
    // requires contiguous scalar storage.
    usize flatStart = chunkStartTuple * numComponents;
    usize flatCount = chunkTuples * numComponents;
    auto inputBuffer = std::make_unique<InputT[]>(flatCount);
    Result<> readResult = inputStore.copyIntoBuffer(flatStart, nonstd::span<InputT>(inputBuffer.get(), flatCount));
    if(readResult.invalid())
    {
      return readResult;
    }

    InputT comparisonValueTyped = static_cast<InputT>(compValue);
    filterChunk<InputT, MaskT>(compOperator, inputBuffer.get(), numComponents, componentIndex, chunkTuples, trueValue, falseValue, tempBuffer, comparisonValueTyped);
    return {};
  }
};

/**
 * @brief Recursively evaluates one threshold-tree node for a tuple chunk.
 * @tparam MaskT Specifies the output mask type.
 * @param node Specifies a threshold leaf or set.
 * @param dataStructure Provides threshold input arrays.
 * @param chunkStart Specifies the first tuple index.
 * @param chunkTuples Specifies tuple count in the chunk.
 * @param trueValue Represents a match.
 * @param falseValue Represents a nonmatch.
 * @param output Receives chunk mask values.
 * @param shouldCancel Stops later tree work when true.
 * @return Error from input bulk I/O, or success after cancellation.
 *
 * Child buffers are released as recursion unwinds, so peak scratch depends on
 * chunk size and tree depth rather than the total output tuple count.
 */
template <typename MaskT>
Result<> EvaluateScanlineNode(const IArrayThreshold& node, const DataStructure& dataStructure, usize chunkStart, usize chunkTuples, MaskT trueValue, MaskT falseValue, MaskT* output,
                              const std::atomic_bool& shouldCancel)
{
  if(shouldCancel)
  {
    return {};
  }
  if(const auto* threshold = dynamic_cast<const ArrayThreshold*>(&node); threshold != nullptr)
  {
    const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(threshold->getArrayPath());
    Result<> result = ExecuteDataFunction(ChunkedThresholdHelper{}, inputArray.getDataType(), inputArray, threshold->getComparisonType(), threshold->getComparisonValue(),
                                          threshold->getComponentIndex(), chunkStart, chunkTuples, trueValue, falseValue, output);
    if(result.invalid())
    {
      return result;
    }
  }
  else if(const auto* thresholdSet = dynamic_cast<const ArrayThresholdSet*>(&node); thresholdSet != nullptr)
  {
    std::fill_n(output, chunkTuples, falseValue);
    bool hasChild = false;
    for(const auto& child : thresholdSet->getArrayThresholds())
    {
      if(shouldCancel)
      {
        return {};
      }
      auto childOutput = std::make_unique<MaskT[]>(chunkTuples);
      Result<> result = EvaluateScanlineNode(*child, dataStructure, chunkStart, chunkTuples, trueValue, falseValue, childOutput.get(), shouldCancel);
      if(result.invalid())
      {
        return result;
      }
      if(!hasChild)
      {
        std::copy_n(childOutput.get(), chunkTuples, output);
        hasChild = true;
      }
      else
      {
        insertThresholdChunk(chunkTuples, output, child->getUnionOperator(), childOutput.get(), false, trueValue, falseValue, shouldCancel);
      }
    }
  }
  if(node.isInverted())
  {
    for(usize i = 0; i < chunkTuples; ++i)
    {
      if((i % 4096) == 0 && shouldCancel)
      {
        return {};
      }
      output[i] = (output[i] == trueValue) ? falseValue : trueValue;
    }
  }
  return {};
}

/**
 * @struct ScanlineEvaluator
 * @brief Evaluates complete threshold chunks and writes each output chunk.
 */
struct ScanlineEvaluator
{
  /**
   * @brief Evaluates and writes one typed output mask.
   * @tparam MaskT Specifies the output mask type.
   * @param thresholdSet Specifies the root threshold set.
   * @param dataStructure Provides threshold input arrays.
   * @param outputArray Receives mask values.
   * @param trueValue Represents a match.
   * @param falseValue Represents a nonmatch.
   * @param shouldCancel Stops before later chunks when true.
   * @return Error from bulk I/O, or success after cancellation.
   */
  template <typename MaskT>
  Result<> operator()(const ArrayThresholdSet& thresholdSet, const DataStructure& dataStructure, IDataArray& outputArray, MaskT trueValue, MaskT falseValue, const std::atomic_bool& shouldCancel)
  {
    auto& outputStore = outputArray.template getIDataStoreRefAs<AbstractDataStore<MaskT>>();
    for(usize chunkStart = 0; chunkStart < outputStore.getNumberOfTuples(); chunkStart += k_ChunkSize)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize chunkTuples = std::min(k_ChunkSize, outputStore.getNumberOfTuples() - chunkStart);
      auto outputBuffer = std::make_unique<MaskT[]>(chunkTuples);
      Result<> result = EvaluateScanlineNode(thresholdSet, dataStructure, chunkStart, chunkTuples, trueValue, falseValue, outputBuffer.get(), shouldCancel);
      if(result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return {};
      }
      result = outputStore.copyFromBuffer(chunkStart, nonstd::span<const MaskT>(outputBuffer.get(), chunkTuples));
      if(result.invalid())
      {
        return result;
      }
    }
    return {};
  }
};
} // namespace

MultiThresholdObjectsScanline::MultiThresholdObjectsScanline(DataStructure& dataStructure, const IFilter::MessageHandler&, const std::atomic_bool& shouldCancel,
                                                             const MultiThresholdObjectsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
{
}

MultiThresholdObjectsScanline::~MultiThresholdObjectsScanline() noexcept = default;

Result<> MultiThresholdObjectsScanline::operator()()
{
  auto thresholdsObject = m_InputValues->ArrayThresholdsObject;
  auto maskArrayName = m_InputValues->OutputDataArrayName;
  auto maskArrayType = m_InputValues->CreatedMaskType;
  auto useCustomTrueValue = m_InputValues->UseCustomTrueValue;
  auto useCustomFalseValue = m_InputValues->UseCustomFalseValue;
  auto customTrueValue = m_InputValues->CustomTrueValue;
  auto customFalseValue = m_InputValues->CustomFalseValue;

  float64 trueValue = useCustomTrueValue ? customTrueValue : 1.0;
  float64 falseValue = useCustomFalseValue ? customFalseValue : 0.0;

  DataPath maskArrayPath = (*thresholdsObject.getRequiredPaths().begin()).replaceName(maskArrayName);
  auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(maskArrayPath);

  return ExecuteDataFunction(ScanlineEvaluator{}, maskArrayType, thresholdsObject, m_DataStructure, maskArray, trueValue, falseValue, m_ShouldCancel);
}
