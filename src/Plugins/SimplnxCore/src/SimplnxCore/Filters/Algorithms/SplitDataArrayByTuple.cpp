#include "SplitDataArrayByTuple.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>
#include <vector>

using namespace nx::core;

namespace
{
// Bulk transfers target one MiB but always retain one complete tuple.
constexpr usize k_TargetChunkBytes = 1024 * 1024;

/**
 * @brief Multiplies a half-open range of tuple-shape extents.
 * @param shape Supplies tuple extents.
 * @param first First extent index.
 * @param last Exclusive final extent index.
 * @return Product, or one for an empty range.
 * @pre first is not greater than last, and last is not greater than shape size.
 * @pre The product fits usize.
 */
usize CalculateProduct(const ShapeType& shape, usize first, usize last)
{
  return std::accumulate(shape.begin() + first, shape.begin() + last, usize{1}, std::multiplies<>());
}

/**
 * @brief Splits numeric tuple blocks through checked bulk transfers.
 * @tparam T Specifies the source and output value type.
 * @param dataStructure Contains source and output arrays.
 * @param inputArrayPath Identifies the source array.
 * @param outputArrayPaths Identifies ordered output blocks.
 * @param splitDimension Selects the partitioned tuple dimension.
 * @param messageHandler Receives one message per output.
 * @param shouldCancel Signals cancellation between chunks.
 * @return Component-count or bulk-I/O result.
 * @pre Output extents form an ordered partition of the source dimension.
 *
 * The transfer target is one MiB, but a wider tuple produces a larger buffer.
 * Completed output blocks are not restored after cancellation or error.
 */
template <typename T>
Result<> SplitDataArraysScanlineTyped(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, usize splitDimension,
                                      const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel);

/**
 * @class CopySplitDataArrayDirectTask
 * @brief Copies one N-D output block through maximal contiguous row-major scanlines.
 * @tparam T Specifies the source and output value type.
 */
template <typename T>
class CopySplitDataArrayDirectTask
{
public:
  /**
   * @brief Initializes one direct output task.
   * @param inputValues Supplies source values.
   * @param outputValues Receives one output block.
   * @param inputScanlineTuples Number of source tuples in one outer scanline.
   * @param outputScanlineTuples Number of output tuples in one outer scanline.
   * @param inputSplitOffset First source tuple within each scanline.
   * @param outerCount Number of outer scanlines.
   * @param numComponents Number of values per tuple.
   * @param shouldCancel Signals cancellation between outer scanlines.
   * @pre Pointers and calculated ranges remain valid through task completion.
   */
  CopySplitDataArrayDirectTask(const T* inputValues, T* outputValues, usize inputScanlineTuples, usize outputScanlineTuples, usize inputSplitOffset, usize outerCount, usize numComponents,
                               const std::atomic_bool& shouldCancel)
  : m_InputValues(inputValues)
  , m_OutputValues(outputValues)
  , m_InputScanlineTuples(inputScanlineTuples)
  , m_OutputScanlineTuples(outputScanlineTuples)
  , m_InputSplitOffset(inputSplitOffset)
  , m_OuterCount(outerCount)
  , m_NumComponents(numComponents)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Copies all outer scanlines for this output block.
   */
  void operator()() const
  {
    const usize outputScanlineValues = m_OutputScanlineTuples * m_NumComponents;
    for(usize outerIndex = 0; outerIndex < m_OuterCount; outerIndex++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const usize inputTupleOffset = outerIndex * m_InputScanlineTuples + m_InputSplitOffset;
      const usize outputTupleOffset = outerIndex * m_OutputScanlineTuples;
      std::copy_n(m_InputValues + inputTupleOffset * m_NumComponents, outputScanlineValues, m_OutputValues + outputTupleOffset * m_NumComponents);
    }
  }

private:
  const T* m_InputValues = nullptr;
  T* m_OutputValues = nullptr;
  usize m_InputScanlineTuples = 0;
  usize m_OutputScanlineTuples = 0;
  usize m_InputSplitOffset = 0;
  usize m_OuterCount = 0;
  usize m_NumComponents = 0;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Splits numeric tuple blocks through parallel raw-pointer tasks.
 * @tparam T Specifies the source and output value type.
 * @param dataStructure Contains source and output arrays.
 * @param inputArrayPath Identifies the source array.
 * @param outputArrayPaths Identifies ordered output blocks.
 * @param splitDimension Selects the partitioned tuple dimension.
 * @param messageHandler Receives one message per output.
 * @param shouldCancel Signals cancellation during scheduling and scanlines.
 * @return Component-count or bulk-fallback result.
 * @pre Output extents form an ordered partition of the source dimension.
 *
 * If any store is not a concrete DataStore, all outputs use the checked bulk
 * implementation. This prevents mixed direct and bulk output state.
 */
template <typename T>
Result<> SplitDataArraysDirectTyped(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, usize splitDimension,
                                    const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  const auto& inputArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
  const auto* inputStore = dynamic_cast<const DataStore<T>*>(&inputArray.getDataStoreRef());
  if(inputStore == nullptr)
  {
    return SplitDataArraysScanlineTyped<T>(dataStructure, inputArrayPath, outputArrayPaths, splitDimension, messageHandler, shouldCancel);
  }

  const ShapeType& inputTupleShape = inputArray.getTupleShape();
  const usize numComponents = inputArray.getNumberOfComponents();
  const usize innerTuples = CalculateProduct(inputTupleShape, splitDimension + 1, inputTupleShape.size());
  const usize outerCount = CalculateProduct(inputTupleShape, 0, splitDimension);
  const usize inputScanlineTuples = inputTupleShape[splitDimension] * innerTuples;

  for(const DataPath& outputArrayPath : outputArrayPaths)
  {
    auto& outputArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    if(outputArray.getNumberOfComponents() != numComponents)
    {
      return MakeErrorResult(-2036, fmt::format("CopyDataND: Component count mismatch between source ({}) and destination ({}); both arrays must have identical component counts.", numComponents,
                                                outputArray.getNumberOfComponents()));
    }

    auto* outputStore = dynamic_cast<DataStore<T>*>(&outputArray.getDataStoreRef());
    if(outputStore == nullptr)
    {
      return SplitDataArraysScanlineTyped<T>(dataStructure, inputArrayPath, outputArrayPaths, splitDimension, messageHandler, shouldCancel);
    }
  }

  ParallelTaskAlgorithm taskRunner;
  usize splitStart = 0;
  for(usize outputIndex = 0; outputIndex < outputArrayPaths.size(); outputIndex++)
  {
    if(shouldCancel)
    {
      break;
    }

    messageHandler({IFilter::Message::Type::Info, fmt::format("Splitting data array '{}' by tuple ({}/{})", inputArrayPath.toString(), outputIndex + 1, outputArrayPaths.size())});
    auto& outputArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPaths[outputIndex]);
    auto& outputStore = dynamic_cast<DataStore<T>&>(outputArray.getDataStoreRef());
    const usize outputSplitCount = outputArray.getTupleShape()[splitDimension];
    const usize outputScanlineTuples = outputSplitCount * innerTuples;
    taskRunner.execute(
        CopySplitDataArrayDirectTask<T>(inputStore->data(), outputStore.data(), inputScanlineTuples, outputScanlineTuples, splitStart * innerTuples, outerCount, numComponents, shouldCancel));
    splitStart += outputSplitCount;
  }
  taskRunner.wait();

  return {};
}

template <typename T>
Result<> SplitDataArraysScanlineTyped(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, usize splitDimension,
                                      const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  const auto& inputArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
  const auto& inputStore = inputArray.getDataStoreRef();
  const ShapeType& inputTupleShape = inputArray.getTupleShape();
  const usize numComponents = inputArray.getNumberOfComponents();
  if(numComponents == 0 || outputArrayPaths.empty() || shouldCancel)
  {
    return {};
  }

  const usize innerTuples = CalculateProduct(inputTupleShape, splitDimension + 1, inputTupleShape.size());
  const usize outerCount = CalculateProduct(inputTupleShape, 0, splitDimension);
  const usize inputScanlineTuples = inputTupleShape[splitDimension] * innerTuples;
  const usize targetChunkValues = std::max<usize>(1, k_TargetChunkBytes / sizeof(T));
  const usize chunkTuples = std::max<usize>(1, targetChunkValues / numComponents);
  const usize bufferValues = chunkTuples * numComponents;

  for(const DataPath& outputArrayPath : outputArrayPaths)
  {
    auto& outputArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    if(outputArray.getNumberOfComponents() != numComponents)
    {
      return MakeErrorResult(-2036, fmt::format("CopyDataND: Component count mismatch between source ({}) and destination ({}); both arrays must have identical component counts.", numComponents,
                                                outputArray.getNumberOfComponents()));
    }
  }

  auto buffer = std::make_unique<T[]>(bufferValues);

  for(usize outputIndex = 0; outputIndex < outputArrayPaths.size(); outputIndex++)
  {
    messageHandler({IFilter::Message::Type::Info, fmt::format("Splitting data array '{}' by tuple ({}/{})", inputArrayPath.toString(), outputIndex + 1, outputArrayPaths.size())});
  }

  // Outer-first traversal keeps source reads in ascending row-major order.
  for(usize outerIndex = 0; outerIndex < outerCount; outerIndex++)
  {
    usize splitStart = 0;
    for(const DataPath& outputArrayPath : outputArrayPaths)
    {
      auto& outputArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
      auto& outputStore = outputArray.getDataStoreRef();
      const usize outputSplitCount = outputArray.getTupleShape()[splitDimension];
      const usize outputScanlineTuples = outputSplitCount * innerTuples;
      for(usize scanlineOffset = 0; scanlineOffset < outputScanlineTuples; scanlineOffset += chunkTuples)
      {
        if(shouldCancel)
        {
          return {};
        }

        const usize tupleCount = std::min(chunkTuples, outputScanlineTuples - scanlineOffset);
        const usize valueCount = tupleCount * numComponents;
        const usize inputTupleOffset = outerIndex * inputScanlineTuples + splitStart * innerTuples + scanlineOffset;
        const usize outputTupleOffset = outerIndex * outputScanlineTuples + scanlineOffset;

        Result<> result = inputStore.copyIntoBuffer(inputTupleOffset * numComponents, nonstd::span<T>(buffer.get(), valueCount));
        if(result.invalid())
        {
          return result;
        }

        result = outputStore.copyFromBuffer(outputTupleOffset * numComponents, nonstd::span<const T>(buffer.get(), valueCount));
        if(result.invalid())
        {
          return result;
        }
      }
      splitStart += outputSplitCount;
    }
  }

  return {};
}

/**
 * @struct SplitDataArraysDirectTemplateImpl
 * @brief Adapts runtime value dispatch to the direct numeric split.
 */
struct SplitDataArraysDirectTemplateImpl
{
  /**
   * @brief Runs one typed direct split and stores its result.
   * @tparam T Specifies the numeric value type.
   * @param dataStructure Contains source and output arrays.
   * @param inputArrayPath Identifies the source array.
   * @param outputArrayPaths Identifies ordered outputs.
   * @param splitDimension Selects the partitioned tuple dimension.
   * @param messageHandler Receives output messages.
   * @param shouldCancel Signals cancellation.
   * @param result Receives the typed split result.
   */
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, usize splitDimension, const IFilter::MessageHandler& messageHandler,
                  const std::atomic_bool& shouldCancel, Result<>& result) const
  {
    result = SplitDataArraysDirectTyped<T>(dataStructure, inputArrayPath, outputArrayPaths, splitDimension, messageHandler, shouldCancel);
  }
};

/**
 * @struct SplitDataArraysScanlineTemplateImpl
 * @brief Adapts runtime value dispatch to the bulk numeric split.
 */
struct SplitDataArraysScanlineTemplateImpl
{
  /**
   * @brief Runs one typed bulk split and stores its result.
   * @tparam T Specifies the numeric value type.
   * @param dataStructure Contains source and output arrays.
   * @param inputArrayPath Identifies the source array.
   * @param outputArrayPaths Identifies ordered outputs.
   * @param splitDimension Selects the partitioned tuple dimension.
   * @param messageHandler Receives output messages.
   * @param shouldCancel Signals cancellation.
   * @param result Receives the typed split result.
   */
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, usize splitDimension, const IFilter::MessageHandler& messageHandler,
                  const std::atomic_bool& shouldCancel, Result<>& result) const
  {
    result = SplitDataArraysScanlineTyped<T>(dataStructure, inputArrayPath, outputArrayPaths, splitDimension, messageHandler, shouldCancel);
  }
};

/**
 * @class SplitDataArraysDirect
 * @brief Dispatch target for parallel contiguous copies between in-memory DataStores.
 */
class SplitDataArraysDirect
{
public:
  /**
   * @brief Initializes the direct numeric dispatch target.
   * @param dataStructure Contains source and output arrays.
   * @param inputArrayPath Identifies the source array.
   * @param outputArrayPaths Identifies ordered outputs.
   * @param splitDimension Selects the partitioned tuple dimension.
   * @param messageHandler Receives output messages.
   * @param shouldCancel Signals cancellation.
   * @pre All arguments outlive this target.
   */
  SplitDataArraysDirect(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, usize splitDimension,
                        const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  : m_DataStructure(dataStructure)
  , m_InputArrayPath(inputArrayPath)
  , m_OutputArrayPaths(outputArrayPaths)
  , m_SplitDimension(splitDimension)
  , m_MessageHandler(messageHandler)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Runs runtime value dispatch for the direct split.
   * @return Typed direct or fallback result.
   */
  Result<> operator()() const
  {
    const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputArrayPath);
    Result<> result;
    ExecuteDataFunction(SplitDataArraysDirectTemplateImpl{}, inputArray.getDataType(), m_DataStructure, m_InputArrayPath, m_OutputArrayPaths, m_SplitDimension, m_MessageHandler, m_ShouldCancel,
                        result);
    return result;
  }

private:
  DataStructure& m_DataStructure;
  const DataPath& m_InputArrayPath;
  const std::vector<DataPath>& m_OutputArrayPaths;
  usize m_SplitDimension = 0;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class SplitDataArraysScanline
 * @brief Streams N-D output blocks in bounded contiguous chunks for disk-backed stores.
 */
class SplitDataArraysScanline
{
public:
  /**
   * @brief Initializes the bulk numeric dispatch target.
   * @param dataStructure Contains source and output arrays.
   * @param inputArrayPath Identifies the source array.
   * @param outputArrayPaths Identifies ordered outputs.
   * @param splitDimension Selects the partitioned tuple dimension.
   * @param messageHandler Receives output messages.
   * @param shouldCancel Signals cancellation.
   * @pre All arguments outlive this target.
   */
  SplitDataArraysScanline(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, usize splitDimension,
                          const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  : m_DataStructure(dataStructure)
  , m_InputArrayPath(inputArrayPath)
  , m_OutputArrayPaths(outputArrayPaths)
  , m_SplitDimension(splitDimension)
  , m_MessageHandler(messageHandler)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Runs runtime value dispatch for the bulk split.
   * @return Typed bulk-I/O result.
   */
  Result<> operator()() const
  {
    const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputArrayPath);
    Result<> result;
    ExecuteDataFunction(SplitDataArraysScanlineTemplateImpl{}, inputArray.getDataType(), m_DataStructure, m_InputArrayPath, m_OutputArrayPaths, m_SplitDimension, m_MessageHandler, m_ShouldCancel,
                        result);
    return result;
  }

private:
  DataStructure& m_DataStructure;
  const DataPath& m_InputArrayPath;
  const std::vector<DataPath>& m_OutputArrayPaths;
  usize m_SplitDimension = 0;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class SplitDataArrayByTupleImpl
 * @brief Copies one complete generic array output block.
 * @tparam ArrayType Specifies DataArray or StringArray storage behavior.
 *
 * CopyDataND validates and copies the complete block after one cancellation
 * check. This task discards its Result.
 */
template <typename ArrayType>
class SplitDataArrayByTupleImpl
{
public:
  /**
   * @brief Initializes one generic array output task.
   * @param inputArray Supplies source tuples.
   * @param outputArray Receives one output block.
   * @param inputTupleShapeOffsets First source tuple in each dimension.
   * @param shouldCancel Signals cancellation before the complete copy.
   * @pre All arguments outlive this task.
   */
  SplitDataArrayByTupleImpl(const ArrayType& inputArray, ArrayType& outputArray, const std::vector<usize> inputTupleShapeOffsets, const std::atomic_bool& shouldCancel)
  : m_InputArray(inputArray)
  , m_OutputArray(outputArray)
  , m_InputTupleShapeOffsets(inputTupleShapeOffsets)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~SplitDataArrayByTupleImpl() = default;

  SplitDataArrayByTupleImpl(const SplitDataArrayByTupleImpl&) = default;
  SplitDataArrayByTupleImpl(SplitDataArrayByTupleImpl&&) noexcept = default;
  SplitDataArrayByTupleImpl& operator=(const SplitDataArrayByTupleImpl&) = delete;
  SplitDataArrayByTupleImpl& operator=(SplitDataArrayByTupleImpl&&) noexcept = delete;

  /**
   * @brief Copies the output block when cancellation is not set.
   */
  void operator()() const
  {
    convert();
  }

protected:
  /**
   * @brief Invokes CopyDataND for the complete output shape.
   *
   * The current implementation discards the copy Result.
   */
  void convert() const
  {
    if(m_ShouldCancel)
    {
      return;
    }

    auto inputTupleShape = m_InputArray.getTupleShape();
    auto outputTupleShape = m_OutputArray.getTupleShape();
    const std::vector<usize> startOutputTupleOffsets(inputTupleShape.size(), 0);
    CopyFromArray::CopyDataND(m_InputArray, m_OutputArray, m_InputTupleShapeOffsets, startOutputTupleOffsets, outputTupleShape);
  }

private:
  const ArrayType& m_InputArray;
  ArrayType& m_OutputArray;
  const std::vector<usize> m_InputTupleShapeOffsets;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class SplitNeighborListByTupleImpl
 * @brief Copies one complete NeighborList output range.
 * @tparam T Specifies the NeighborList value type.
 *
 * CopyDataND validates and copies the complete range after one cancellation
 * check. This task discards its Result.
 */
template <typename T>
class SplitNeighborListByTupleImpl
{
public:
  /**
   * @brief Initializes one NeighborList output task.
   * @param inputNL Supplies source lists.
   * @param outputNL Receives one output range.
   * @param inputTupleOffset First source list.
   * @param shouldCancel Signals cancellation before the complete copy.
   * @pre All arguments outlive this task.
   */
  SplitNeighborListByTupleImpl(const NeighborList<T>& inputNL, NeighborList<T>& outputNL, usize inputTupleOffset, const std::atomic_bool& shouldCancel)
  : m_InputNL(inputNL)
  , m_OutputNL(outputNL)
  , m_InputTupleOffset(inputTupleOffset)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~SplitNeighborListByTupleImpl() = default;

  SplitNeighborListByTupleImpl(const SplitNeighborListByTupleImpl&) = default;
  SplitNeighborListByTupleImpl(SplitNeighborListByTupleImpl&&) noexcept = default;
  SplitNeighborListByTupleImpl& operator=(const SplitNeighborListByTupleImpl&) = delete;
  SplitNeighborListByTupleImpl& operator=(SplitNeighborListByTupleImpl&&) noexcept = delete;

  /**
   * @brief Copies the output range when cancellation is not set.
   */
  void operator()() const
  {
    convert();
  }

protected:
  /**
   * @brief Invokes CopyDataND for the complete output list range.
   *
   * The current implementation discards the copy Result.
   */
  void convert() const
  {
    if(m_ShouldCancel)
    {
      return;
    }

    auto outputTupleShape = m_OutputNL.getTupleShape();
    usize startOutputOffset = 0;
    CopyFromArray::CopyDataND(m_InputNL, m_OutputNL, {m_InputTupleOffset}, {startOutputOffset}, outputTupleShape);
  }

private:
  const NeighborList<T>& m_InputNL;
  NeighborList<T>& m_OutputNL;
  usize m_InputTupleOffset;
  const std::atomic_bool& m_ShouldCancel;
};

template <typename T>
struct is_allowed_array_type : std::false_type
{
};

template <typename T>
struct is_allowed_array_type<DataArray<T>> : std::true_type
{
};

template <>
struct is_allowed_array_type<StringArray> : std::true_type
{
};

/**
 * @brief Schedules one complete output task for each generic array block.
 * @tparam ArrayType Specifies DataArray or StringArray behavior.
 * @param dataStructure Contains source and output arrays.
 * @param inputArrayPath Identifies the source array.
 * @param outputArrayPaths Identifies ordered outputs.
 * @param splitDimension Selects the partitioned tuple dimension.
 * @param messageHandler Receives output messages.
 * @param shouldCancel Signals cancellation during scheduling and task start.
 * @return Success after all tasks join.
 * @pre Output extents form an ordered partition of the source dimension.
 *
 * Each task copies one output without further cancellation checks and discards
 * its CopyDataND Result. Outputs can complete in a different order.
 */
template <typename ArrayType>
typename std::enable_if<is_allowed_array_type<ArrayType>::value, Result<>>::type SplitArraysByTupleImpl(DataStructure& dataStructure, const DataPath& inputArrayPath,
                                                                                                        const std::vector<DataPath>& outputArrayPaths, usize splitDimension,
                                                                                                        const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  // Independent output blocks run as separate tasks.
  ParallelTaskAlgorithm taskRunner;
  auto& inputArray = dataStructure.getDataRefAs<ArrayType>(inputArrayPath);
  auto inputTupleShape = inputArray.getTupleShape();
  std::vector<usize> inputTupleShapeOffset(inputTupleShape.size(), 0);
  for(usize i = 0; i < outputArrayPaths.size(); ++i)
  {
    if(shouldCancel)
    {
      return {};
    }

    auto& outputArray = dataStructure.getDataRefAs<ArrayType>(outputArrayPaths[i]);

    messageHandler({IFilter::Message::Type::Info, fmt::format("Splitting data array '{}' by tuple ({}/{})", inputArrayPath.toString(), i + 1, outputArrayPaths.size())});

    taskRunner.execute(SplitDataArrayByTupleImpl<ArrayType>(inputArray, outputArray, inputTupleShapeOffset, shouldCancel));

    inputTupleShapeOffset[splitDimension] += outputArray.getTupleShape()[splitDimension];
  }
  taskRunner.wait();

  return {};
}

/**
 * @brief Schedules one complete output task for each NeighborList range.
 * @tparam T Specifies the NeighborList value type.
 * @param dataStructure Contains source and output lists.
 * @param inputArrayPath Identifies the source NeighborList.
 * @param outputArrayPaths Identifies ordered outputs.
 * @param messageHandler Receives output messages.
 * @param shouldCancel Signals cancellation during scheduling and task start.
 * @return Success after task-runner destruction joins all tasks.
 *
 * Each task discards its CopyDataND Result. Output tuple counts define sequential
 * source ranges because NeighborList tuple shapes are one-dimensional.
 */
template <typename T>
Result<> SplitNeighborListsByTupleImpl(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, const IFilter::MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel)
{
  ParallelTaskAlgorithm taskRunner;
  auto& inputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(inputArrayPath);

  usize inputTupleOffset = 0;
  for(usize i = 0; i < outputArrayPaths.size(); ++i)
  {
    if(shouldCancel)
    {
      return {};
    }

    messageHandler({IFilter::Message::Type::Info, fmt::format("Splitting neighbor list '{}' by tuple ({}/{})", inputArrayPath.toString(), i + 1, outputArrayPaths.size())});

    auto& outputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(outputArrayPaths[i]);
    taskRunner.execute(SplitNeighborListByTupleImpl(inputNeighborList, outputNeighborList, inputTupleOffset, shouldCancel));
    inputTupleOffset += outputNeighborList.getNumberOfTuples();
  }

  return {};
}

/**
 * @struct SplitNeighborListsTemplateImpl
 * @brief Adapts runtime NeighborList value dispatch to typed splitting.
 */
struct SplitNeighborListsTemplateImpl
{
  /**
   * @brief Runs one typed NeighborList split and stores its result.
   * @tparam T Specifies the NeighborList value type.
   * @param dataStructure Contains source and output lists.
   * @param inputArrayPath Identifies the source NeighborList.
   * @param outputArrayPaths Identifies ordered outputs.
   * @param messageHandler Receives output messages.
   * @param shouldCancel Signals cancellation.
   * @param result Receives the typed split result.
   */
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, const IFilter::MessageHandler& messageHandler,
                  const std::atomic_bool& shouldCancel, Result<>& result)
  {
    result = SplitNeighborListsByTupleImpl<T>(dataStructure, inputArrayPath, outputArrayPaths, messageHandler, shouldCancel);
  }
};

/**
 * @brief Dispatches numeric splitting from all participating store types.
 * @param dataStructure Contains source and output arrays.
 * @param inputArrayPath Identifies the source DataArray.
 * @param outputArrayPaths Identifies ordered outputs.
 * @param splitDimension Selects the partitioned tuple dimension.
 * @param messageHandler Receives output messages.
 * @param shouldCancel Signals cancellation.
 * @return Direct or bulk numeric split result.
 *
 * One out-of-core output is sufficient to select bulk I/O for every output.
 * This prevents a mixed-path partial split.
 */
Result<> SplitArraysByTuple(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, usize splitDimension,
                            const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  const auto& inputDataArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const IDataArray* representativeOutputArray = nullptr;
  for(const DataPath& outputArrayPath : outputArrayPaths)
  {
    const auto& outputArray = dataStructure.getDataRefAs<IDataArray>(outputArrayPath);
    if(representativeOutputArray == nullptr || IsOutOfCore(outputArray))
    {
      representativeOutputArray = &outputArray;
    }
    if(IsOutOfCore(outputArray))
    {
      break;
    }
  }

  return DispatchAlgorithm<SplitDataArraysDirect, SplitDataArraysScanline>({&inputDataArray, representativeOutputArray}, dataStructure, inputArrayPath, outputArrayPaths, splitDimension,
                                                                           messageHandler, shouldCancel);
}

/**
 * @brief Dispatches NeighborList splitting by list value type.
 * @param dataStructure Contains source and output lists.
 * @param inputArrayPath Identifies the source NeighborList.
 * @param outputArrayPaths Identifies ordered outputs.
 * @param messageHandler Receives output messages.
 * @param shouldCancel Signals cancellation.
 * @return Typed scheduling result.
 */
Result<> SplitNeighborLists(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, const IFilter::MessageHandler& messageHandler,
                            const std::atomic_bool& shouldCancel)
{
  const auto& inputNeighborList = dataStructure.getDataRefAs<INeighborList>(inputArrayPath);
  Result<> result;
  ExecuteNeighborFunction(SplitNeighborListsTemplateImpl{}, inputNeighborList.getDataType(), dataStructure, inputArrayPath, outputArrayPaths, messageHandler, shouldCancel, result);
  return result;
}
} // namespace

SplitDataArrayByTuple::SplitDataArrayByTuple(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             SplitDataArrayByTupleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

SplitDataArrayByTuple::~SplitDataArrayByTuple() noexcept = default;

const std::atomic_bool& SplitDataArrayByTuple::getCancel()
{
  return m_ShouldCancel;
}

Result<> SplitDataArrayByTuple::operator()()
{
  const auto& inputDataArray = m_DataStructure.getDataRefAs<IArray>(m_InputValues->InputArrayPath);
  std::string arrayTypeName = inputDataArray.getTypeName();
  switch(inputDataArray.getArrayType())
  {
  case IArray::ArrayType::DataArray: {
    return SplitArraysByTuple(m_DataStructure, m_InputValues->InputArrayPath, m_InputValues->OutputArrayPaths, m_InputValues->SplitDimension, m_MessageHandler, m_ShouldCancel);
  }
  case IArray::ArrayType::StringArray: {
    return SplitArraysByTupleImpl<StringArray>(m_DataStructure, m_InputValues->InputArrayPath, m_InputValues->OutputArrayPaths, m_InputValues->SplitDimension, m_MessageHandler, m_ShouldCancel);
  }
  case IArray::ArrayType::NeighborListArray: {
    return SplitNeighborLists(m_DataStructure, m_InputValues->InputArrayPath, m_InputValues->OutputArrayPaths, m_MessageHandler, m_ShouldCancel);
  }
  case IArray::ArrayType::Any: {
    return MakeErrorResult(to_underlying(SplitDataArrayByTuple::ErrorCodes::AnyArrayType),
                           fmt::format("The input array '{}' has array type 'Any'.  This SHOULD NOT be possible, so please contact the developers.", m_InputValues->InputArrayPath.toString()));
  }
  default: {
    return MakeErrorResult(
        to_underlying(SplitDataArrayByTuple::ErrorCodes::UnsupportedArrayType),
        fmt::format("The input array '{}' has an array type that is currently not supported by this filter, so please contact the developers.", m_InputValues->InputArrayPath.toString()));
  }
  }
}
