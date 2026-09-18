#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/core.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <vector>

namespace nx::core::ImageProcessing
{
inline constexpr int32 k_MaskWrongType = -8100;

/**
 * @brief Applies a typed scalar mask in bounded chunks. Each chunk bulk-reads the input and mask, updates
 *        only the local input buffer in parallel, and bulk-writes it to the output. Backing-store access
 *        remains serial because DataStore implementations do not provide concurrent-access guarantees.
 */
template <class T, class MaskT>
Result<> ApplyTypedMask(const AbstractDataStore<T>& inputStore, AbstractDataStore<T>& outputStore, const AbstractDataStore<MaskT>& maskStore, usize numComponents, T outsideValue,
                        const std::atomic_bool& shouldCancel)
{
  const usize numTuples = inputStore.getNumberOfTuples();
  if(numComponents == 0)
  {
    return MakeErrorResult(-8106, "Mask input must have at least one component per tuple, but its component count is 0.");
  }
  if(numTuples > std::numeric_limits<usize>::max() / numComponents)
  {
    return MakeErrorResult(-8107, fmt::format("Mask input size overflows usize: {} tuples with {} components per tuple.", numTuples, numComponents));
  }
  const usize numValues = numTuples * numComponents;
  if(inputStore.getSize() != numValues || outputStore.getSize() != numValues)
  {
    return MakeErrorResult(-8108, fmt::format("Mask input/output store sizes must both equal the expected value count ({} tuples x {} components = {}). Actual input size: {}; actual output size: {}.",
                                              numTuples, numComponents, numValues, inputStore.getSize(), outputStore.getSize()));
  }
  if(maskStore.getNumberOfTuples() != numTuples || maskStore.getSize() != numTuples)
  {
    return MakeErrorResult(-8109, fmt::format("Mask store must contain exactly one value for each of the {} input tuples. Actual mask tuple count: {}; actual mask size: {}.", numTuples,
                                              maskStore.getNumberOfTuples(), maskStore.getSize()));
  }
  if(numTuples == 0)
  {
    return {};
  }

  constexpr usize k_TargetWorkingSetBytes = 32 * 1024 * 1024;
  if(numComponents > (std::numeric_limits<usize>::max() - sizeof(MaskT)) / sizeof(T))
  {
    return MakeErrorResult(-8110, fmt::format("Mask working-set size overflows usize for {} input components of {} bytes each and a {}-byte mask value.", numComponents, sizeof(T), sizeof(MaskT)));
  }
  const usize bytesPerTuple = numComponents * sizeof(T) + sizeof(MaskT);
  if(numTuples > std::numeric_limits<usize>::max() / bytesPerTuple)
  {
    return MakeErrorResult(-8111, fmt::format("Mask useful working-set size overflows usize for {} tuples at {} bytes per tuple.", numTuples, bytesPerTuple));
  }
  const usize usefulWorkingSetBytes = numTuples * bytesPerTuple;
  const bool usesOutOfCoreStore =
      inputStore.getStoreType() == IDataStore::StoreType::OutOfCore || outputStore.getStoreType() == IDataStore::StoreType::OutOfCore || maskStore.getStoreType() == IDataStore::StoreType::OutOfCore;
  CacheMemoryBudgetManager::WorkingMemoryReservation workingMemoryReservation;
  usize targetWorkingSetBytes = std::min(k_TargetWorkingSetBytes, usefulWorkingSetBytes);
  if(usesOutOfCoreStore)
  {
    constexpr uint32 k_PreferredNumerator = 1;
    constexpr uint32 k_PreferredDenominator = 4;
    const uint64 preferredBytes = ResolveWorkingMemoryFractionBytes(usefulWorkingSetBytes, k_PreferredNumerator, k_PreferredDenominator, k_TargetWorkingSetBytes);
    workingMemoryReservation = ReserveWorkingMemory(preferredBytes, usefulWorkingSetBytes);
    const usize grantedBytes = static_cast<usize>(std::min<uint64>(workingMemoryReservation.sizeBytes(), std::numeric_limits<usize>::max()));
    if(grantedBytes >= bytesPerTuple)
    {
      targetWorkingSetBytes = grantedBytes;
    }
    else
    {
      // The adaptive grant cannot hold one complete tuple. Release it and retain the established bounded fallback.
      workingMemoryReservation = {};
    }
  }
  const usize maximumChunkTuples = std::min(numTuples, std::max<usize>(1, targetWorkingSetBytes / bytesPerTuple));
  const usize batchCount = 1 + (numTuples - 1) / maximumChunkTuples;
  const usize chunkTuples = 1 + (numTuples - 1) / batchCount;
  if(workingMemoryReservation.sizeBytes() > 0)
  {
    workingMemoryReservation.shrinkTo(static_cast<uint64>(chunkTuples * bytesPerTuple));
  }
  std::vector<T> values(chunkTuples * numComponents);
  std::vector<MaskT> mask(chunkTuples);

  for(usize startTuple = 0; startTuple < numTuples; startTuple += chunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(chunkTuples, numTuples - startTuple);
    const usize valueCount = count * numComponents;
    if(Result<> r = maskStore.copyIntoBuffer(startTuple, nonstd::span<MaskT>(mask.data(), count)); r.invalid())
    {
      return r;
    }
    if(shouldCancel)
    {
      return {};
    }

    const bool allOutside = std::all_of(mask.begin(), mask.begin() + count, [](MaskT value) { return value == static_cast<MaskT>(0); });
    if(allOutside)
    {
      std::fill_n(values.data(), valueCount, outsideValue);
    }
    else if(Result<> r = inputStore.copyIntoBuffer(startTuple * numComponents, nonstd::span<T>(values.data(), valueCount)); r.invalid())
    {
      return r;
    }

    auto applyOutsideValue = [&](const Range& tupleRange) {
      if(shouldCancel)
      {
        return;
      }
      if(numComponents == 1)
      {
        for(usize tuple = tupleRange.min(); tuple < tupleRange.max(); ++tuple)
        {
          values[tuple] = mask[tuple] == static_cast<MaskT>(0) ? outsideValue : values[tuple];
        }
        return;
      }
      for(usize tuple = tupleRange.min(); tuple < tupleRange.max(); ++tuple)
      {
        if(mask[tuple] == static_cast<MaskT>(0))
        {
          std::fill_n(values.data() + tuple * numComponents, numComponents, outsideValue);
        }
      }
    };
    if(!allOutside)
    {
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, count);
      parallelAlgorithm.execute(applyOutsideValue);
    }
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> r = outputStore.copyFromBuffer(startTuple * numComponents, nonstd::span<const T>(values.data(), valueCount)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

/**
 * @brief Applies a mask: for each tuple, keep all numComponents of the input pixel where mask != 0, else
 *        set all components to outsideValue. Tuple-chunked bulk I/O (bounded buffers), in-core == OOC.
 */
template <class T>
Result<> ApplyMask(const AbstractDataStore<T>& inputStore, AbstractDataStore<T>& outputStore, const IDataArray& maskArray, usize numComponents, T outsideValue, const std::atomic_bool& shouldCancel,
                   const IFilter::MessageHandler& messageHandler)
{
  switch(maskArray.getDataType())
  {
  case DataType::uint8:
    return ApplyTypedMask(inputStore, outputStore, maskArray.getIDataStoreRefAs<AbstractDataStore<uint8>>(), numComponents, outsideValue, shouldCancel);
  case DataType::uint16:
    return ApplyTypedMask(inputStore, outputStore, maskArray.getIDataStoreRefAs<AbstractDataStore<uint16>>(), numComponents, outsideValue, shouldCancel);
  case DataType::uint32:
    return ApplyTypedMask(inputStore, outputStore, maskArray.getIDataStoreRefAs<AbstractDataStore<uint32>>(), numComponents, outsideValue, shouldCancel);
  default:
    return MakeErrorResult(k_MaskWrongType,
                           fmt::format("Mask array '{}' must be uint8, uint16, or uint32, but its data type is '{}'.", maskArray.getName(), DataTypeToString(maskArray.getDataType())));
  }
}
} // namespace nx::core::ImageProcessing
