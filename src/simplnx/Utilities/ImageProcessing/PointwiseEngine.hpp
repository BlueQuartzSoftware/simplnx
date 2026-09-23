#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <initializer_list>
#include <limits>
#include <memory>
#include <numeric>
#include <string_view>
#include <type_traits>

namespace nx::core::ImageProcessing
{
namespace detail
{
inline constexpr usize k_PointwiseTargetScratchBytes = 64 * 1024 * 1024;

struct PointwiseEndpointLayout
{
  IDataStore::StoreType storeType = IDataStore::StoreType::InMemory;
  const ShapeType& tupleShape;
  usize numComponents = 0;
  std::string_view name;
  enum class Access : uint8
  {
    Read,
    Write
  } access = Access::Read;
};

struct PointwiseBatchPlan
{
  usize batchValues = 0;
  usize totalBatches = 0;
  usize alignmentValues = 0;
  bool usedMismatchedLayoutFallback = false;
};

struct PointwiseExecutionOptions
{
  usize targetScratchBytes = k_PointwiseTargetScratchBytes;
  bool forceBuffered = false;
};

inline bool CheckedMultiply(usize left, usize right, usize& product) noexcept
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

/**
 * @brief Plans a bounded pointwise batch in flat values. Each out-of-core endpoint contributes
 *        a full trailing-dimensional slab alignment: number of components for rank-one stores,
 *        or number of components times tupleShape[1..] for higher-rank stores. Checked LCM combines
 *        differing endpoint layouts while it remains bounded by the target or one endpoint slab.
 *        A larger common alignment falls back to the OOC writer's slab (or the largest endpoint slab
 *        when there is no OOC writer), keeping allocation bounded and output writes rectangular.
 */
inline Result<PointwiseBatchPlan> MakePointwiseBatchPlan(usize totalValues, usize targetValues, std::initializer_list<PointwiseEndpointLayout> endpoints)
{
  if(totalValues == 0)
  {
    return {PointwiseBatchPlan{}};
  }

  targetValues = std::max<usize>(1, targetValues);
  usize combinedAlignment = 1;
  usize largestEndpointAlignment = 1;
  usize writerAlignment = 0;
  bool combinedAlignmentOverflow = false;
  for(const PointwiseEndpointLayout& endpoint : endpoints)
  {
    if(endpoint.storeType != IDataStore::StoreType::OutOfCore)
    {
      continue;
    }
    if(endpoint.numComponents == 0)
    {
      return MakeErrorResult<PointwiseBatchPlan>(-8690, fmt::format("Pointwise operation {} out-of-core store has zero components for {} total values.", endpoint.name, totalValues));
    }
    if(endpoint.tupleShape.empty())
    {
      return MakeErrorResult<PointwiseBatchPlan>(-8691, fmt::format("Pointwise operation {} out-of-core store has an empty tuple shape for {} total values.", endpoint.name, totalValues));
    }

    usize tupleValues = 1;
    usize slabValues = endpoint.numComponents;
    for(usize dimensionIndex = 0; dimensionIndex < endpoint.tupleShape.size(); ++dimensionIndex)
    {
      const usize dimension = endpoint.tupleShape[dimensionIndex];
      if(dimension == 0)
      {
        return MakeErrorResult<PointwiseBatchPlan>(-8692,
                                                   fmt::format("Pointwise operation {} out-of-core store tuple dimension {} is zero for {} total values.", endpoint.name, dimensionIndex, totalValues));
      }
      usize nextTupleValues = 0;
      if(!CheckedMultiply(tupleValues, dimension, nextTupleValues))
      {
        return MakeErrorResult<PointwiseBatchPlan>(
            -8693, fmt::format("Pointwise operation {} out-of-core tuple shape overflows while multiplying dimension {} (value {}).", endpoint.name, dimensionIndex, dimension));
      }
      tupleValues = nextTupleValues;

      if(dimensionIndex > 0)
      {
        usize nextSlabValues = 0;
        if(!CheckedMultiply(slabValues, dimension, nextSlabValues))
        {
          return MakeErrorResult<PointwiseBatchPlan>(
              -8694, fmt::format("Pointwise operation {} out-of-core slab alignment overflows while multiplying dimension {} (value {}).", endpoint.name, dimensionIndex, dimension));
        }
        slabValues = nextSlabValues;
      }
    }

    usize layoutValues = 0;
    if(!CheckedMultiply(tupleValues, endpoint.numComponents, layoutValues))
    {
      return MakeErrorResult<PointwiseBatchPlan>(
          -8695, fmt::format("Pointwise operation {} out-of-core layout overflows for {} tuples and {} components.", endpoint.name, tupleValues, endpoint.numComponents));
    }
    if(layoutValues != totalValues)
    {
      return MakeErrorResult<PointwiseBatchPlan>(
          -8696, fmt::format("Pointwise operation {} out-of-core layout contains {} values, but the operation contains {} values.", endpoint.name, layoutValues, totalValues));
    }
    if(slabValues == 0 || totalValues % slabValues != 0)
    {
      return MakeErrorResult<PointwiseBatchPlan>(
          -8697, fmt::format("Pointwise operation {} out-of-core slab alignment ({}) does not divide the total value count ({}).", endpoint.name, slabValues, totalValues));
    }

    largestEndpointAlignment = std::max(largestEndpointAlignment, slabValues);
    if(endpoint.access == PointwiseEndpointLayout::Access::Write)
    {
      writerAlignment = std::max(writerAlignment, slabValues);
    }

    if(!combinedAlignmentOverflow)
    {
      const usize divisor = std::gcd(combinedAlignment, slabValues);
      const usize reducedAlignment = combinedAlignment / divisor;
      usize nextAlignment = 0;
      if(!CheckedMultiply(reducedAlignment, slabValues, nextAlignment))
      {
        combinedAlignmentOverflow = true;
      }
      else
      {
        combinedAlignment = nextAlignment;
      }
    }
  }

  const usize alignmentBound = std::max(targetValues, largestEndpointAlignment);
  const bool useFallback = combinedAlignmentOverflow || combinedAlignment > alignmentBound;
  const usize selectedAlignment = useFallback ? (writerAlignment != 0 ? writerAlignment : largestEndpointAlignment) : combinedAlignment;
  if(selectedAlignment == 0 || totalValues % selectedAlignment != 0)
  {
    return MakeErrorResult<PointwiseBatchPlan>(-8698,
                                               fmt::format("Pointwise operation selected out-of-core slab alignment ({}) does not divide the total value count ({}).", selectedAlignment, totalValues));
  }

  const usize batchValues = std::min(totalValues, targetValues >= selectedAlignment ? (targetValues / selectedAlignment) * selectedAlignment : selectedAlignment);
  const usize totalBatches = totalValues / batchValues + static_cast<usize>((totalValues % batchValues) != 0);
  return {PointwiseBatchPlan{batchValues, totalBatches, selectedAlignment, useFallback}};
}

template <class T, class U, class MapOpT>
struct PointwiseMapBody
{
  const T* inputBuffer;
  U* outputBuffer;
  const MapOpT& mapOp;
  const std::atomic_bool& shouldCancel;

  void operator()(const Range& range) const
  {
    constexpr usize k_CancelCheckValues = 65536;
    usize blockStart = range.min();
    while(blockStart < range.max())
    {
      if(shouldCancel)
      {
        return;
      }
      const usize blockEnd = blockStart + std::min(k_CancelCheckValues, range.max() - blockStart);
      for(usize i = blockStart; i < blockEnd; ++i)
      {
        outputBuffer[i] = mapOp(inputBuffer[i]);
      }
      blockStart = blockEnd;
    }
  }
};

template <class T, class U, class MapOpT>
Result<> ApplyPointwiseImpl(const AbstractDataStore<T>& inputStore, AbstractDataStore<U>& outputStore, const MapOpT& mapOp, const std::atomic_bool& shouldCancel,
                            const IFilter::MessageHandler& messageHandler, const PointwiseExecutionOptions& options)
{
  const usize totalValues = inputStore.getSize();
  if(outputStore.getSize() != totalValues)
  {
    return MakeErrorResult(-8355, fmt::format("Pointwise operation input and output store sizes must match. Input size: {}; output size: {}.", totalValues, outputStore.getSize()));
  }

  const auto* inMemoryInputStore = dynamic_cast<const DataStore<T>*>(&inputStore);
  auto* inMemoryOutputStore = dynamic_cast<DataStore<U>*>(&outputStore);
  if(!options.forceBuffered && inMemoryInputStore != nullptr && inMemoryOutputStore != nullptr)
  {
    if(shouldCancel)
    {
      return {};
    }

    // Exact DataStore casts guarantee resident, contiguous storage. Obtain the views before entering TBB so workers
    // never call a DataStore method; they only read immutable input memory and write disjoint output values. Any OOC
    // or other abstract store takes the buffered path below.
    const nonstd::span<const T> inputSpan = inMemoryInputStore->createSpan();
    nonstd::span<U> outputSpan = inMemoryOutputStore->createSpan();
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, totalValues);
    parallelAlgorithm.execute(PointwiseMapBody<T, U, MapOpT>{inputSpan.data(), outputSpan.data(), mapOp, shouldCancel});
    return {};
  }

  if(totalValues == 0 || shouldCancel)
  {
    return {};
  }

  constexpr usize k_BytesPerValue = std::is_same_v<T, U> ? sizeof(T) : sizeof(T) + sizeof(U);
  const usize targetValues = std::max<usize>(1, options.targetScratchBytes / k_BytesPerValue);
  Result<PointwiseBatchPlan> planResult =
      MakePointwiseBatchPlan(totalValues, targetValues,
                             {{inputStore.getStoreType(), inputStore.getTupleShape(), inputStore.getNumberOfComponents(), "input"},
                              {outputStore.getStoreType(), outputStore.getTupleShape(), outputStore.getNumberOfComponents(), "output", PointwiseEndpointLayout::Access::Write}});
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const PointwiseBatchPlan& plan = planResult.value();

  MessageHelper messageHelper(messageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(plan.totalBatches);
  progressHelper.setProgressMessageTemplate("Applying pointwise operation: {:.1f}%");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  auto inputBuffer = std::make_unique_for_overwrite<T[]>(plan.batchValues);
  std::unique_ptr<U[]> outputBuffer;
  U* mappedBuffer = nullptr;
  if constexpr(std::is_same_v<T, U>)
  {
    mappedBuffer = inputBuffer.get();
  }
  else
  {
    outputBuffer = std::make_unique_for_overwrite<U[]>(plan.batchValues);
    mappedBuffer = outputBuffer.get();
  }
  for(usize start = 0; start < totalValues;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(plan.batchValues, totalValues - start);
    if(Result<> readResult = inputStore.copyIntoBuffer(start, nonstd::span<T>(inputBuffer.get(), count)); readResult.invalid())
    {
      return readResult;
    }
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, count);
    parallelAlgorithm.execute(PointwiseMapBody<T, U, MapOpT>{inputBuffer.get(), mappedBuffer, mapOp, shouldCancel});
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> writeResult = outputStore.copyFromBuffer(start, nonstd::span<const U>(mappedBuffer, count)); writeResult.invalid())
    {
      return writeResult;
    }
    progressMessenger.sendProgressMessage(1);
    start += count;
  }

  return {};
}
} // namespace detail

/**
 * @brief Applies a per-value map @p mapOp to every value of @p inputStore, writing results to
 *        @p outputStore. Exact DataStore endpoints use direct contiguous spans. Other stores stream
 *        through the AbstractDataStore bulk-I/O API with a combined 64 MiB scratch target. Batches
 *        normally use a bounded common alignment of complete trailing-dimensional slabs for every
 *        out-of-core endpoint. If mismatched layouts would require an oversized common alignment,
 *        batching falls back to the output slab alignment so allocation remains bounded and writes
 *        stay rectangular; the input backend handles those pathological reads. One selected alignment
 *        unit is the minimum when it exceeds the target. Per-batch work is parallelized over local
 *        buffers; store I/O remains serial because DataStore and AbstractDataStore are not thread-safe.
 *
 * @pre @p inputStore and @p outputStore have the same value count (getSize()).
 */
template <class T, class U, class MapOpT>
Result<> ApplyPointwise(const AbstractDataStore<T>& inputStore, AbstractDataStore<U>& outputStore, const MapOpT& mapOp, const std::atomic_bool& shouldCancel,
                        const IFilter::MessageHandler& messageHandler)
{
  return detail::ApplyPointwiseImpl(inputStore, outputStore, mapOp, shouldCancel, messageHandler, {});
}
} // namespace nx::core::ImageProcessing
