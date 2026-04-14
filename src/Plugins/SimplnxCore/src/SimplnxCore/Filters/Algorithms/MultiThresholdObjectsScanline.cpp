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

// =============================================================================
// MultiThresholdObjectsScanline — Out-of-Core (OOC) Algorithm
//
// This file implements the out-of-core (Scanline) variant of MultiThresholdObjects.
// It is selected by DispatchAlgorithm when any input array uses chunked on-disk
// storage (e.g., ZarrStore / HDF5 chunked store).
//
// PROBLEM:
//   The Direct variant uses getComponentValue() for per-element input reads and
//   operator[] for per-element output writes, plus allocates an O(n) temporary
//   result vector per threshold condition. When data is stored out-of-core:
//   - Each getComponentValue() call may load an entire chunk from disk
//   - The O(n) temporary vector wastes memory when only a chunk is needed
//   - operator[] writes to the output mask may also trigger chunk load/evict
//
// SOLUTION — CHUNKED PROCESSING:
//   Process data in fixed-size 64K-tuple chunks:
//   1. Read a chunk of the input array via copyIntoBuffer() (one bulk read)
//   2. Apply the threshold comparison to produce a chunk-sized temp buffer
//   3. For the first condition: write temp buffer to output via copyFromBuffer()
//   4. For subsequent conditions: read current output chunk, merge AND/OR, write back
//
// MEMORY SAVINGS:
//   Peak memory per threshold condition is O(k_ChunkSize) = O(64K) instead of O(n).
//   For a 100M-tuple dataset, this reduces temporary memory from ~100 MB to ~64 KB.
//
// IMPLEMENTATION NOTE:
//   Temporary buffers use std::unique_ptr<T[]> instead of std::vector<T> to avoid
//   the std::vector<bool> specialization, which would prevent direct memory access
//   needed for copyIntoBuffer/copyFromBuffer spans.
// =============================================================================

namespace
{
/**
 * @brief Chunk size for OOC processing. Each iteration reads/writes this many
 * tuples via bulk I/O. 64K tuples balances between minimizing I/O calls and
 * keeping per-chunk memory small.
 */
constexpr usize k_ChunkSize = 65536;

/**
 * @brief Applies a single comparison operator to a chunk of input data, writing
 * trueValue/falseValue into the chunk-sized output buffer.
 *
 * @tparam CompT Comparison functor (std::less<>, std::greater<>, etc.)
 * @tparam InputT The input array element type
 * @tparam MaskT The output mask element type
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
 * @brief Dispatches the comparison based on the ComparisonType enum.
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
 * @brief Merges a chunk of new threshold results into the current output chunk.
 */
template <typename MaskT>
void insertThresholdChunk(usize chunkTuples, MaskT* currentBuffer, IArrayThreshold::UnionOperator unionOperator, MaskT* newBuffer, bool inverse, MaskT trueValue, MaskT falseValue)
{
  for(usize i = 0; i < chunkTuples; i++)
  {
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
 * @brief Functor that reads a chunk of the input array via copyIntoBuffer and
 * applies the threshold comparison to produce chunk-sized output.
 */
struct ChunkedThresholdHelper
{
  template <typename InputT, typename MaskT>
  void operator()(const IDataArray& iDataArray, ArrayThreshold::ComparisonType compOperator, ArrayThreshold::ComparisonValue compValue, usize componentIndex, usize chunkStartTuple, usize chunkTuples,
                  MaskT trueValue, MaskT falseValue, MaskT* tempBuffer)
  {
    const auto& inputStore = iDataArray.template getIDataStoreRefAs<AbstractDataStore<InputT>>();
    usize numComponents = inputStore.getNumberOfComponents();

    // Read input chunk (flat elements = tuples * components)
    // Use unique_ptr instead of vector to avoid std::vector<bool> specialization
    usize flatStart = chunkStartTuple * numComponents;
    usize flatCount = chunkTuples * numComponents;
    auto inputBuffer = std::make_unique<InputT[]>(flatCount);
    inputStore.copyIntoBuffer(flatStart, nonstd::span<InputT>(inputBuffer.get(), flatCount));

    InputT comparisonValueTyped = static_cast<InputT>(compValue);
    filterChunk<InputT, MaskT>(compOperator, inputBuffer.get(), numComponents, componentIndex, chunkTuples, trueValue, falseValue, tempBuffer, comparisonValueTyped);
  }
};

/**
 * @brief Processes a single ArrayThreshold comparison in chunks for OOC.
 *
 * For each 64K-tuple chunk:
 *   1. Allocate a chunk-sized temp buffer (O(64K), not O(n))
 *   2. Read a chunk of the input array via ChunkedThresholdHelper (copyIntoBuffer)
 *   3. Apply the comparison operator to fill the temp buffer with TRUE/FALSE
 *   4. If this is the first condition (replaceInput=true): write the temp buffer
 *      directly to the output mask store via copyFromBuffer
 *   5. If this is a subsequent condition: read the current output chunk via
 *      copyIntoBuffer, merge using AND/OR logic, then write back
 *
 * This chunk-by-chunk approach replaces the Direct variant's O(n) tempResultVector
 * with an O(chunkSize) buffer, and replaces per-element input reads with bulk I/O.
 *
 * @tparam MaskT The output mask element type
 * @param comparisonValue The threshold condition to evaluate
 * @param dataStructure DataStructure containing the input array
 * @param outputResultStore The output mask data store
 * @param err Error code (set on failure)
 * @param replaceInput If true, overwrite output; if false, merge with existing
 * @param inverse If true, invert the comparison result before merging
 * @param trueValue Value to write for TRUE elements
 * @param falseValue Value to write for FALSE elements
 * @param shouldCancel Cancellation flag
 */
template <typename MaskT>
void ThresholdValueChunked(const ArrayThreshold& comparisonValue, const DataStructure& dataStructure, AbstractDataStore<MaskT>& outputResultStore, int32& err, bool replaceInput, bool inverse,
                           MaskT trueValue, MaskT falseValue, const std::atomic_bool& shouldCancel)
{
  usize totalTuples = outputResultStore.getNumberOfTuples();

  ArrayThreshold::ComparisonType compOperator = comparisonValue.getComparisonType();
  ArrayThreshold::ComparisonValue compValue = comparisonValue.getComparisonValue();
  IArrayThreshold::UnionOperator unionOperator = comparisonValue.getUnionOperator();

  DataPath inputDataArrayPath = comparisonValue.getArrayPath();
  usize componentIndex = comparisonValue.getComponentIndex();

  const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(inputDataArrayPath);
  DataType inputDataType = iDataArray.getDataType();

  // Process in chunks
  for(usize chunkStart = 0; chunkStart < totalTuples; chunkStart += k_ChunkSize)
  {
    if(shouldCancel)
    {
      return;
    }

    usize chunkTuples = std::min(k_ChunkSize, totalTuples - chunkStart);

    // Chunk-sized temp buffer for this threshold's results
    // Use unique_ptr instead of vector to avoid std::vector<bool> specialization
    auto tempBuffer = std::make_unique<MaskT[]>(chunkTuples);
    std::fill_n(tempBuffer.get(), chunkTuples, falseValue);

    // Apply threshold comparison to this chunk
    ExecuteDataFunction(ChunkedThresholdHelper{}, inputDataType, iDataArray, compOperator, compValue, componentIndex, chunkStart, chunkTuples, trueValue, falseValue, tempBuffer.get());

    if(replaceInput)
    {
      if(inverse)
      {
        for(usize i = 0; i < chunkTuples; i++)
        {
          tempBuffer[i] = (tempBuffer[i] == trueValue) ? falseValue : trueValue;
        }
      }
      // Write temp buffer directly to output store
      outputResultStore.copyFromBuffer(chunkStart, nonstd::span<const MaskT>(tempBuffer.get(), chunkTuples));
    }
    else
    {
      // Read current output chunk, merge, write back
      auto currentBuffer = std::make_unique<MaskT[]>(chunkTuples);
      outputResultStore.copyIntoBuffer(chunkStart, nonstd::span<MaskT>(currentBuffer.get(), chunkTuples));
      insertThresholdChunk<MaskT>(chunkTuples, currentBuffer.get(), unionOperator, tempBuffer.get(), inverse, trueValue, falseValue);
      outputResultStore.copyFromBuffer(chunkStart, nonstd::span<const MaskT>(currentBuffer.get(), chunkTuples));
    }
  }
}

struct ThresholdValueChunkedFunctor
{
  template <typename MaskT>
  void operator()(const ArrayThreshold& comparisonValue, const DataStructure& dataStructure, IDataArray& outputResultArray, int32& err, bool replaceInput, bool inverse, MaskT trueValue,
                  MaskT falseValue, const std::atomic_bool& shouldCancel)
  {
    ThresholdValueChunked(comparisonValue, dataStructure, outputResultArray.template getIDataStoreRefAs<AbstractDataStore<MaskT>>(), err, replaceInput, inverse, trueValue, falseValue, shouldCancel);
  }
};

/**
 * @brief Processes an ArrayThresholdSet (a group of thresholds with AND/OR logic) in chunks for OOC.
 *
 * Recursively evaluates each child threshold in the set. Each child may be either
 * a single ArrayThreshold (handled by ThresholdValueChunked) or a nested
 * ArrayThresholdSet (handled recursively). The first child replaces the output;
 * subsequent children merge using their AND/OR union operator.
 *
 * @tparam MaskT The output mask element type
 */
template <typename MaskT>
void ThresholdSetChunked(const ArrayThresholdSet& inputComparisonSet, const DataStructure& dataStructure, AbstractDataStore<MaskT>& outputResultStore, int32& err, bool replaceInput, bool inverse,
                         MaskT trueValue, MaskT falseValue, const std::atomic_bool& shouldCancel)
{
  bool firstValueFound = false;

  ArrayThresholdSet::CollectionType thresholds = inputComparisonSet.getArrayThresholds();
  for(const std::shared_ptr<IArrayThreshold>& threshold : thresholds)
  {
    if(shouldCancel)
    {
      return;
    }

    const IArrayThreshold* thresholdPtr = threshold.get();
    if(const auto* comparisonSet = dynamic_cast<const ArrayThresholdSet*>(thresholdPtr); comparisonSet != nullptr)
    {
      ThresholdSetChunked<MaskT>(*comparisonSet, dataStructure, outputResultStore, err, !firstValueFound, false, trueValue, falseValue, shouldCancel);
      firstValueFound = true;
    }
    else if(const auto* comparisonValue = dynamic_cast<const ArrayThreshold*>(thresholdPtr); comparisonValue != nullptr)
    {
      ThresholdValueChunked<MaskT>(*comparisonValue, dataStructure, outputResultStore, err, !firstValueFound, false, trueValue, falseValue, shouldCancel);
      firstValueFound = true;
    }
  }
}

struct ThresholdSetChunkedFunctor
{
  template <typename MaskT>
  void operator()(const ArrayThresholdSet& inputComparisonSet, const DataStructure& dataStructure, IDataArray& outputResultArray, int32& err, bool replaceInput, bool inverse, MaskT trueValue,
                  MaskT falseValue, const std::atomic_bool& shouldCancel)
  {
    ThresholdSetChunked<MaskT>(inputComparisonSet, dataStructure, outputResultArray.template getIDataStoreRefAs<AbstractDataStore<MaskT>>(), err, replaceInput, inverse, trueValue, falseValue,
                               shouldCancel);
  }
};
} // namespace

// -----------------------------------------------------------------------------
MultiThresholdObjectsScanline::MultiThresholdObjectsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             const MultiThresholdObjectsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MultiThresholdObjectsScanline::~MultiThresholdObjectsScanline() noexcept = default;

// -----------------------------------------------------------------------------
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
  ArrayThresholdSet::CollectionType ThresholdSet = thresholdsObject.getArrayThresholds();

  bool firstValueFound = false;
  int32 err = 0;
  for(usize threshIdx = 0; threshIdx < ThresholdSet.size(); threshIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto& threshold = ThresholdSet[threshIdx];
    const IArrayThreshold* thresholdPtr = threshold.get();
    if(const auto* comparisonSet = dynamic_cast<const ArrayThresholdSet*>(thresholdPtr); comparisonSet != nullptr)
    {
      ExecuteDataFunction(ThresholdSetChunkedFunctor{}, maskArrayType, *comparisonSet, m_DataStructure, m_DataStructure.getDataRefAs<IDataArray>(maskArrayPath), err, !firstValueFound,
                          thresholdsObject.isInverted(), trueValue, falseValue, m_ShouldCancel);
      firstValueFound = true;
    }
    else if(const auto* comparisonValue = dynamic_cast<const ArrayThreshold*>(thresholdPtr); comparisonValue != nullptr)
    {
      ExecuteDataFunction(ThresholdValueChunkedFunctor{}, maskArrayType, *comparisonValue, m_DataStructure, m_DataStructure.getDataRefAs<IDataArray>(maskArrayPath), err, !firstValueFound,
                          thresholdsObject.isInverted(), trueValue, falseValue, m_ShouldCancel);
      firstValueFound = true;
    }
  }

  return {};
}
