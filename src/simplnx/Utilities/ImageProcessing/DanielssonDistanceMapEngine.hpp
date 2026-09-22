#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/SweepTemporaryStore.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <exception>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
constexpr usize k_Danielsson2DResidentLimit = 64ULL * 1024ULL * 1024ULL;
constexpr usize k_Danielsson2DFixedStateBytes = 4096;
constexpr usize k_DanielssonVisitArrayLimit = 1ULL * 1024ULL * 1024ULL;
constexpr usize k_DanielssonMaxDimension = static_cast<usize>(std::numeric_limits<int32>::max() / 2);

// A per-axis visit entry for ITK's ReflectiveImageRegionConstIterator odometer: the coordinate to visit and the pull
// offset for that axis on this visit (-1 on the forward sub-sweep -> pull i-1; +1 on the backward sub-sweep -> pull
// i+1; 0 for a size-1 axis, which is never updated).
struct AxisVisit
{
  int64 coord;
  int32 pull;
};

// Build the reflective odometer visit list for one axis of length @p n. Forward sub-sweep visits 1..n-1 (each pulling
// i-1), then the backward sub-sweep visits n-2..0 (each pulling i+1). Interior coords are visited twice, the two
// endpoints once each -- exactly ITK's ReflectiveImageRegionConstIterator with begin/end offset 1. A size<=1 axis
// yields a single no-update visit at 0. The +-1 ranges guarantee the pulled neighbor is always in-bounds.
inline std::vector<AxisVisit> BuildAxisVisits(int64 n)
{
  std::vector<AxisVisit> visits;
  if(n <= 1)
  {
    visits.push_back({0, 0});
    return visits;
  }
  visits.reserve(static_cast<usize>(2 * n - 2));
  for(int64 i = 1; i < n; ++i)
  {
    visits.push_back({i, -1}); // forward pass: pull the neighbor at i-1
  }
  for(int64 i = n - 2; i >= 0; --i)
  {
    visits.push_back({i, +1}); // backward pass: pull the neighbor at i+1
  }
  return visits;
}

/** @brief Emits the reflective axis visits without retaining an axis-sized vector. */
template <class Function>
void ForEachAxisVisit(int64 n, Function&& function)
{
  if(n <= 1)
  {
    function(AxisVisit{0, 0});
    return;
  }
  for(int64 index = 1; index < n; ++index)
  {
    function(AxisVisit{index, -1});
  }
  for(int64 index = n - 2; index >= 0; --index)
  {
    function(AxisVisit{index, 1});
  }
}

struct Danielsson2DBufferPlan
{
  usize coreRows = 0;
  usize coreCols = 0;
  usize residentBytes = 0;
  bool valid = false;
  bool overflow = false;
};

inline bool DanielssonCheckedAdd(usize left, usize right, usize& result)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  result = left + right;
  return true;
}

inline bool DanielssonCheckedMultiply(usize left, usize right, usize& result)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  result = left * right;
  return true;
}

inline bool DanielssonAxisVisitCount(usize length, usize& count)
{
  if(length <= 1)
  {
    count = 1;
    return true;
  }
  usize doubled = 0;
  if(!DanielssonCheckedMultiply(length, usize{2}, doubled))
  {
    return false;
  }
  count = doubled - 2;
  return true;
}

inline bool DanielssonXYVisitArraysFit(usize nx, usize ny, usize visitArrayLimit, usize& payloadBytes)
{
  usize xCount = 0;
  usize yCount = 0;
  usize totalCount = 0;
  payloadBytes = 0;
  return DanielssonAxisVisitCount(nx, xCount) && DanielssonAxisVisitCount(ny, yCount) && DanielssonCheckedAdd(xCount, yCount, totalCount) &&
         DanielssonCheckedMultiply(totalCount, sizeof(AxisVisit), payloadBytes) && payloadBytes <= visitArrayLimit;
}

struct DanielssonResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize requiredBytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return requiredBytes > 0 && reservation.sizeBytes() == requiredBytes;
  }
};

struct DanielssonResidentVectorPrefixPlan
{
  usize fixedBytes = 0;
  usize vectorPlaneBytes = 0;
  usize axisVisitBytes = 0;
  usize residentPrefixPlanes = 0;
  usize residentPrefixValues = 0;
  usize residentPrefixBytes = 0;
  usize residentBytes = 0;
};

/** @brief Test-only transfer witness for the bounded 3-D vector scratch. */
struct DanielssonVectorTransferStats
{
  usize scratchReads = 0;
  usize scratchWrites = 0;
  usize scratchReadValues = 0;
  usize scratchWriteValues = 0;
  usize scratchReadBytes = 0;
  usize scratchWriteBytes = 0;
  std::vector<usize> scratchReadPlanes;
  std::vector<usize> scratchWritePlanes;
};

/**
 * @brief Plane-indexed router over a full-width (int32) vector-map scratch store, with an optional resident prefix.
 *
 * @tparam ScratchStoreT Bulk linear-offset store backing planes at or past @c prefixPlanes: either a real
 *         AbstractDataStore<int32> (a compressed OOC-format store, or plain resident memory when neither endpoint is
 *         out-of-core) or a raw fixed-record detail::SweepTemporaryStore<int32> that keeps the traffic off the
 *         deflate codec. Both expose the same copyIntoBuffer/copyFromBuffer(usize, span<int32>) surface, so this
 *         class is oblivious to which one it is given.
 */
template <class ScratchStoreT>
class DanielssonHybridVectorPlaneStore
{
public:
  DanielssonHybridVectorPlaneStore(ScratchStoreT& scratch, std::vector<int32>& residentPrefix, usize prefixPlanes, usize vectorValuesPerPlane, const std::atomic_bool& shouldCancel,
                                   DanielssonVectorTransferStats* transferStats = nullptr)
  : m_Scratch(scratch)
  , m_ResidentPrefix(residentPrefix)
  , m_PrefixPlanes(prefixPlanes)
  , m_VectorValuesPerPlane(vectorValuesPerPlane)
  , m_ShouldCancel(shouldCancel)
  , m_TransferStats(transferStats)
  {
  }

  Result<> readPlane(usize planeIndex, nonstd::span<int32> values) const
  {
    return transferPlane(planeIndex, values, "read");
  }

  Result<> writePlane(usize planeIndex, nonstd::span<const int32> values)
  {
    return transferPlane(planeIndex, values, "write");
  }

private:
  template <class SpanT>
  Result<> transferPlane(usize planeIndex, SpanT values, std::string_view operation) const
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    usize prefixValues = 0;
    usize scratchOffset = 0;
    if(m_VectorValuesPerPlane == 0 || values.size() != m_VectorValuesPerPlane || !DanielssonCheckedMultiply(m_PrefixPlanes, m_VectorValuesPerPlane, prefixValues) ||
       !DanielssonCheckedMultiply(planeIndex, m_VectorValuesPerPlane, scratchOffset) || scratchOffset > m_Scratch.getSize() || m_VectorValuesPerPlane > m_Scratch.getSize() - scratchOffset)
    {
      return MakeErrorResult(-8390, fmt::format("Danielsson hybrid vector-plane {} has invalid plane, buffer, prefix, or scratch bounds. Plane: {}; prefix planes: {}; vector values per plane: {}; "
                                                "buffer values: {}; prefix values: {}; scratch offset: {}; scratch values: {}.",
                                                operation, planeIndex, m_PrefixPlanes, m_VectorValuesPerPlane, values.size(), prefixValues, scratchOffset, m_Scratch.getSize()));
    }
    if(planeIndex < m_PrefixPlanes)
    {
      const usize prefixOffset = scratchOffset;
      if(prefixOffset > m_ResidentPrefix.size() || m_VectorValuesPerPlane > m_ResidentPrefix.size() - prefixOffset)
      {
        return MakeErrorResult(-8390, fmt::format("Danielsson hybrid vector-plane {} exceeds resident prefix storage. Plane: {}; prefix offset: {}; vector values per plane: {}; resident values: {}.",
                                                  operation, planeIndex, prefixOffset, m_VectorValuesPerPlane, m_ResidentPrefix.size()));
      }
      if constexpr(std::is_const_v<typename SpanT::element_type>)
      {
        std::copy(values.begin(), values.end(), m_ResidentPrefix.begin() + static_cast<std::ptrdiff_t>(prefixOffset));
      }
      else
      {
        std::copy_n(m_ResidentPrefix.data() + prefixOffset, m_VectorValuesPerPlane, values.data());
      }
      return {};
    }
    if constexpr(std::is_const_v<typename SpanT::element_type>)
    {
      if(m_TransferStats != nullptr)
      {
        ++m_TransferStats->scratchWrites;
        m_TransferStats->scratchWriteValues += m_VectorValuesPerPlane;
        m_TransferStats->scratchWriteBytes += m_VectorValuesPerPlane * sizeof(int32);
        m_TransferStats->scratchWritePlanes.push_back(planeIndex);
      }
      return m_Scratch.copyFromBuffer(scratchOffset, values);
    }
    else
    {
      if(m_TransferStats != nullptr)
      {
        ++m_TransferStats->scratchReads;
        m_TransferStats->scratchReadValues += m_VectorValuesPerPlane;
        m_TransferStats->scratchReadBytes += m_VectorValuesPerPlane * sizeof(int32);
        m_TransferStats->scratchReadPlanes.push_back(planeIndex);
      }
      return m_Scratch.copyIntoBuffer(scratchOffset, values);
    }
  }

  ScratchStoreT& m_Scratch;
  std::vector<int32>& m_ResidentPrefix;
  usize m_PrefixPlanes = 0;
  usize m_VectorValuesPerPlane = 0;
  const std::atomic_bool& m_ShouldCancel;
  DanielssonVectorTransferStats* m_TransferStats = nullptr;
};

inline bool DanielssonCanUseCompactVectorScratch(const SizeVec3& dims)
{
  const usize maxDimension = std::max({dims[0], dims[1], dims[2]});
  return maxDimension <= static_cast<usize>(std::numeric_limits<int16>::max() / 2);
}

/**
 * @brief Hybrid router that stores suffix vector planes as checked int16 values.
 *
 * @tparam ScratchStoreT Bulk linear-offset store backing planes at or past @c prefixPlanes: either a real
 *         AbstractDataStore<int16> (a compressed OOC-format store, or plain resident memory when neither endpoint is
 *         out-of-core) or a raw fixed-record detail::SweepTemporaryStore<int16> that keeps the traffic off the
 *         deflate codec. Both expose the same copyIntoBuffer/copyFromBuffer(usize, span<int16>) surface, so this
 *         class is oblivious to which one it is given.
 */
template <class ScratchStoreT>
class DanielssonCompactHybridVectorPlaneStore
{
public:
  DanielssonCompactHybridVectorPlaneStore(ScratchStoreT& scratch, std::vector<int32>& residentPrefix, std::vector<int16>& staging, usize prefixPlanes, usize vectorValuesPerPlane,
                                          const std::atomic_bool& shouldCancel, DanielssonVectorTransferStats* transferStats = nullptr)
  : m_Scratch(scratch)
  , m_ResidentPrefix(residentPrefix)
  , m_Staging(staging)
  , m_PrefixPlanes(prefixPlanes)
  , m_VectorValuesPerPlane(vectorValuesPerPlane)
  , m_ShouldCancel(shouldCancel)
  , m_TransferStats(transferStats)
  {
  }

  Result<> readPlane(usize planeIndex, nonstd::span<int32> values) const
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(Result<> result = validate(planeIndex, values.size(), "read"); result.invalid())
    {
      return result;
    }
    if(planeIndex < m_PrefixPlanes)
    {
      std::copy_n(m_ResidentPrefix.data() + planeIndex * m_VectorValuesPerPlane, m_VectorValuesPerPlane, values.data());
      return {};
    }
    if(m_TransferStats != nullptr)
    {
      ++m_TransferStats->scratchReads;
      m_TransferStats->scratchReadValues += m_VectorValuesPerPlane;
      m_TransferStats->scratchReadBytes += m_VectorValuesPerPlane * sizeof(int16);
      m_TransferStats->scratchReadPlanes.push_back(planeIndex);
    }
    const usize scratchOffset = planeIndex * m_VectorValuesPerPlane;
    if(Result<> result = m_Scratch.copyIntoBuffer(scratchOffset, nonstd::span<int16>(m_Staging.data(), m_VectorValuesPerPlane)); result.invalid())
    {
      return result;
    }
    for(usize index = 0; index < m_VectorValuesPerPlane; ++index)
    {
      values[index] = m_Staging[index];
    }
    return {};
  }

  Result<> writePlane(usize planeIndex, nonstd::span<const int32> values)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(Result<> result = validate(planeIndex, values.size(), "write"); result.invalid())
    {
      return result;
    }
    if(planeIndex < m_PrefixPlanes)
    {
      std::copy(values.begin(), values.end(), m_ResidentPrefix.begin() + static_cast<std::ptrdiff_t>(planeIndex * m_VectorValuesPerPlane));
      return {};
    }
    for(usize index = 0; index < m_VectorValuesPerPlane; ++index)
    {
      if(values[index] < std::numeric_limits<int16>::lowest() || values[index] > std::numeric_limits<int16>::max())
      {
        return MakeErrorResult(-8390, fmt::format("Danielsson compact vector scratch cannot encode plane {} value {} at component {}.", planeIndex, values[index], index));
      }
      m_Staging[index] = static_cast<int16>(values[index]);
    }
    if(m_TransferStats != nullptr)
    {
      ++m_TransferStats->scratchWrites;
      m_TransferStats->scratchWriteValues += m_VectorValuesPerPlane;
      m_TransferStats->scratchWriteBytes += m_VectorValuesPerPlane * sizeof(int16);
      m_TransferStats->scratchWritePlanes.push_back(planeIndex);
    }
    return m_Scratch.copyFromBuffer(planeIndex * m_VectorValuesPerPlane, nonstd::span<const int16>(m_Staging.data(), m_VectorValuesPerPlane));
  }

private:
  Result<> validate(usize planeIndex, usize valueCount, std::string_view operation) const
  {
    usize scratchOffset = 0;
    if(m_VectorValuesPerPlane == 0 || valueCount != m_VectorValuesPerPlane || m_Staging.size() < m_VectorValuesPerPlane ||
       !DanielssonCheckedMultiply(planeIndex, m_VectorValuesPerPlane, scratchOffset) || scratchOffset > m_Scratch.getSize() || m_VectorValuesPerPlane > m_Scratch.getSize() - scratchOffset)
    {
      return MakeErrorResult(
          -8390, fmt::format("Danielsson compact vector-plane {} has invalid plane, buffer, or scratch bounds. Plane: {}; vector values per plane: {}; buffer values: {}; scratch values: {}.",
                             operation, planeIndex, m_VectorValuesPerPlane, valueCount, m_Scratch.getSize()));
    }
    return {};
  }

  ScratchStoreT& m_Scratch;
  std::vector<int32>& m_ResidentPrefix;
  std::vector<int16>& m_Staging;
  usize m_PrefixPlanes = 0;
  usize m_VectorValuesPerPlane = 0;
  const std::atomic_bool& m_ShouldCancel;
  DanielssonVectorTransferStats* m_TransferStats = nullptr;
};

template <class T>
Result<DanielssonResidentVectorPrefixPlan> CreateDanielssonResidentVectorPrefixPlan(const SizeVec3& dims, usize grantBytes)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {DanielssonResidentVectorPrefixPlan{}};
  }
  usize planeValues = 0;
  usize vectorPlaneValues = 0;
  usize vectorPlaneBytes = 0;
  usize liveVectorValues = 0;
  usize liveVectorBytes = 0;
  usize outputPlaneBytes = 0;
  usize compactStagingBytes = 0;
  usize axisVisitBytes = 0;
  usize fixedBytes = 0;
  if(!DanielssonCheckedMultiply(dims[0], dims[1], planeValues) || !DanielssonCheckedMultiply(planeValues, usize{3}, vectorPlaneValues) ||
     !DanielssonCheckedMultiply(vectorPlaneValues, sizeof(int32), vectorPlaneBytes) || !DanielssonCheckedMultiply(usize{2}, vectorPlaneValues, liveVectorValues) ||
     !DanielssonCheckedMultiply(liveVectorValues, sizeof(int32), liveVectorBytes) || !DanielssonCheckedMultiply(planeValues, sizeof(T), fixedBytes) ||
     !DanielssonCheckedAdd(fixedBytes, liveVectorBytes, fixedBytes) || !DanielssonCheckedMultiply(planeValues, sizeof(float32), outputPlaneBytes) ||
     !DanielssonCheckedAdd(fixedBytes, outputPlaneBytes, fixedBytes) ||
     (DanielssonCanUseCompactVectorScratch(dims) &&
      (!DanielssonCheckedMultiply(vectorPlaneValues, sizeof(int16), compactStagingBytes) || !DanielssonCheckedAdd(fixedBytes, compactStagingBytes, fixedBytes))))
  {
    return MakeErrorResult<DanielssonResidentVectorPrefixPlan>(
        -8390, fmt::format("Danielsson resident vector-prefix plan overflows for dimensions {} and input element size {}.", StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }

  // The slab retains X/Y reflective-visit vectors only when their payload fits the fixed visit limit. WorkingMemory
  // shrinks its reservation to this plan, so this payload must be part of the fixed live state before prefix sizing.
  if(DanielssonXYVisitArraysFit(dims[0], dims[1], k_DanielssonVisitArrayLimit, axisVisitBytes))
  {
    if(!DanielssonCheckedAdd(fixedBytes, axisVisitBytes, fixedBytes))
    {
      return MakeErrorResult<DanielssonResidentVectorPrefixPlan>(
          -8390, fmt::format("Danielsson resident vector-prefix plan overflows while adding X/Y reflective-visit payload for dimensions {}.", StringUtilities::formatDimensions3D(dims)));
    }
  }
  else
  {
    axisVisitBytes = 0;
  }

  DanielssonResidentVectorPrefixPlan plan;
  plan.fixedBytes = fixedBytes;
  plan.vectorPlaneBytes = vectorPlaneBytes;
  plan.axisVisitBytes = axisVisitBytes;
  if(grantBytes < fixedBytes)
  {
    return {plan};
  }
  // The turnaround retains the last forward state in the existing live plane buffer. Only planes 0..Z-3 can use the
  // optional resident prefix, so reserving more cannot reduce scratch transfers.
  const usize storableForwardPlanes = dims[2] > 2 ? dims[2] - 2 : 0;
  plan.residentPrefixPlanes = std::min(storableForwardPlanes, (grantBytes - fixedBytes) / vectorPlaneBytes);
  if(plan.residentPrefixPlanes == 0)
  {
    return {plan};
  }
  if(!DanielssonCheckedMultiply(plan.residentPrefixPlanes, vectorPlaneValues, plan.residentPrefixValues) ||
     !DanielssonCheckedMultiply(plan.residentPrefixPlanes, vectorPlaneBytes, plan.residentPrefixBytes) || !DanielssonCheckedAdd(fixedBytes, plan.residentPrefixBytes, plan.residentBytes))
  {
    return MakeErrorResult<DanielssonResidentVectorPrefixPlan>(-8390, fmt::format("Danielsson resident vector-prefix bytes overflow for dimensions {}, grant {}, fixed bytes {}, and prefix planes {}.",
                                                                                  StringUtilities::formatDimensions3D(dims), grantBytes, fixedBytes, plan.residentPrefixPlanes));
  }
  return {plan};
}

inline bool ShouldUseDanielssonResidentState(const SizeVec3& dims)
{
  return dims[2] > 1;
}

template <class T>
Result<usize> CalculateDanielssonResidentWorkingMemoryBytes(const SizeVec3& dims)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {usize{0}};
  }

  auto calculateVisitCount = [](usize length, usize& count) {
    if(length <= 1)
    {
      count = 1;
      return true;
    }
    usize doubled = 0;
    if(!DanielssonCheckedMultiply(length, usize{2}, doubled))
    {
      return false;
    }
    count = doubled - 2;
    return true;
  };

  usize sliceValues = 0;
  usize volumeValues = 0;
  usize volumeStateBytes = 0;
  usize inputPlaneBytes = 0;
  usize outputPlaneBytes = 0;
  usize xVisitCount = 0;
  usize yVisitCount = 0;
  usize zVisitCount = 0;
  usize visitCount = 0;
  usize visitBytes = 0;
  usize requiredBytes = 0;
  constexpr usize k_ResidentBytesPerValue = 3 * sizeof(int32) + sizeof(uint8);
  // Reserve both transfer planes conservatively, although the input and output transfer phases do not overlap.
  if(!DanielssonCheckedMultiply(dims[0], dims[1], sliceValues) || !DanielssonCheckedMultiply(sliceValues, dims[2], volumeValues) ||
     !DanielssonCheckedMultiply(volumeValues, k_ResidentBytesPerValue, volumeStateBytes) || !DanielssonCheckedMultiply(sliceValues, sizeof(T), inputPlaneBytes) ||
     !DanielssonCheckedMultiply(sliceValues, sizeof(float32), outputPlaneBytes) || !calculateVisitCount(dims[0], xVisitCount) || !calculateVisitCount(dims[1], yVisitCount) ||
     !calculateVisitCount(dims[2], zVisitCount) || !DanielssonCheckedAdd(xVisitCount, yVisitCount, visitCount) || !DanielssonCheckedAdd(visitCount, zVisitCount, visitCount) ||
     !DanielssonCheckedMultiply(visitCount, sizeof(AxisVisit), visitBytes) || !DanielssonCheckedAdd(volumeStateBytes, inputPlaneBytes, requiredBytes) ||
     !DanielssonCheckedAdd(requiredBytes, outputPlaneBytes, requiredBytes) || !DanielssonCheckedAdd(requiredBytes, visitBytes, requiredBytes))
  {
    return MakeErrorResult<usize>(-8389, fmt::format("Danielsson distance-map dimensions ({}) and input element size ({} bytes) overflow while sizing the resident vector map, feature mask, input "
                                                     "transfer plane, output transfer plane, and axis visits.",
                                                     StringUtilities::formatDimensions3D(dims), sizeof(T)));
  }
  return {requiredBytes};
}

template <class T>
Result<DanielssonResidentMemoryAllocation> ReserveDanielssonResidentWorkingMemory(const SizeVec3& dims)
{
  auto requiredResult = CalculateDanielssonResidentWorkingMemoryBytes<T>(dims);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<DanielssonResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {DanielssonResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

inline bool Danielsson2DPeak(usize columns, usize rows, usize inputBytes, usize& peak)
{
  usize cells = 0;
  usize core = 0;
  usize neighborRow = 0;
  usize total = 0;
  if(!DanielssonCheckedMultiply(columns, rows, cells) || !DanielssonCheckedMultiply(cells, 3 * sizeof(int32) + inputBytes + sizeof(float32), core) ||
     !DanielssonCheckedMultiply(columns, 3 * sizeof(int32), neighborRow) || !DanielssonCheckedAdd(core, neighborRow, total) || !DanielssonCheckedAdd(total, k_Danielsson2DFixedStateBytes, peak))
  {
    return false;
  }
  return true;
}

inline Danielsson2DBufferPlan BuildDanielsson2DBufferPlan(usize nx, usize ny, usize inputBytes, usize residentLimit = k_Danielsson2DResidentLimit)
{
  Danielsson2DBufferPlan plan;
  usize cellCount = 0;
  if(nx == 0 || ny == 0 || inputBytes == 0 || residentLimit == 0 || nx > k_DanielssonMaxDimension || ny > k_DanielssonMaxDimension || !DanielssonCheckedMultiply(nx, ny, cellCount))
  {
    plan.overflow = true;
    return plan;
  }

  usize minimumPeak = 0;
  if(!Danielsson2DPeak(1, 1, inputBytes, minimumPeak))
  {
    plan.overflow = true;
    return plan;
  }
  if(minimumPeak > residentLimit)
  {
    return plan;
  }

  auto largestFitting = [residentLimit](usize high, const auto& peakFunction) {
    usize low = 1;
    while(low < high)
    {
      const usize middle = low + (high - low + 1) / 2;
      usize candidatePeak = 0;
      if(peakFunction(middle, candidatePeak) && candidatePeak <= residentLimit)
      {
        low = middle;
      }
      else
      {
        high = middle - 1;
      }
    }
    return low;
  };

  plan.coreCols = largestFitting(nx, [inputBytes](usize columns, usize& peak) { return Danielsson2DPeak(columns, 1, inputBytes, peak); });
  plan.coreRows = 1;
  if(plan.coreCols == nx)
  {
    plan.coreRows = largestFitting(ny, [nx, inputBytes](usize rows, usize& peak) { return Danielsson2DPeak(nx, rows, inputBytes, peak); });
  }
  if(!Danielsson2DPeak(plan.coreCols, plan.coreRows, inputBytes, plan.residentBytes))
  {
    plan.overflow = true;
    return plan;
  }
  plan.valid = plan.residentBytes <= residentLimit;
  return plan;
}

inline bool DanielssonVisitArraysFit(int64 nx, int64 ny, int64 nz, usize visitArrayLimit = k_DanielssonVisitArrayLimit)
{
  auto visitCount = [](int64 length, usize& count) {
    if(length <= 1)
    {
      count = 1;
      return true;
    }
    const uint64 unsignedLength = static_cast<uint64>(length);
    if(unsignedLength > static_cast<uint64>(std::numeric_limits<usize>::max() / 2) + 1)
    {
      return false;
    }
    count = static_cast<usize>(2 * unsignedLength - 2);
    return true;
  };
  usize xCount = 0;
  usize yCount = 0;
  usize zCount = 0;
  usize totalCount = 0;
  usize totalBytes = 0;
  return visitCount(nx, xCount) && visitCount(ny, yCount) && visitCount(nz, zCount) && DanielssonCheckedAdd(xCount, yCount, totalCount) && DanielssonCheckedAdd(totalCount, zCount, totalCount) &&
         DanielssonCheckedMultiply(totalCount, sizeof(AxisVisit), totalBytes) && totalBytes <= visitArrayLimit;
}

/**
 * @brief Computes the squared norm of a three-component Danielsson offset vector.
 * @param vector Three offset components.
 * @param useSpacing Applies spacing when true.
 * @param spacing Spacing for each component.
 * @return Squared norm in double precision.
 *
 * The expression uses the same casts and association as each Danielsson candidate comparison.
 */
inline float64 DanielssonVectorNorm(const int32* vector, bool useSpacing, const float64 spacing[3])
{
  if(useSpacing)
  {
    const float64 h0 = static_cast<float64>(vector[0]) * spacing[0];
    const float64 h1 = static_cast<float64>(vector[1]) * spacing[1];
    const float64 h2 = static_cast<float64>(vector[2]) * spacing[2];
    return h0 * h0 + h1 * h1 + h2 * h2;
  }
  return static_cast<float64>(vector[0]) * vector[0] + static_cast<float64>(vector[1]) * vector[1] + static_cast<float64>(vector[2]) * vector[2];
}

/**
 * @brief Updates a Danielsson offset vector when a neighbor gives a smaller squared norm.
 * @param hereV Mutable current offset vector.
 * @param thereV Neighbor offset vector.
 * @param offX Candidate X offset from the neighbor.
 * @param offY Candidate Y offset from the neighbor.
 * @param offZ Candidate Z offset from the neighbor.
 * @param useSpacing Applies spacing when true.
 * @param sp Spacing for each component.
 *
 * Double-precision component products match ITK and avoid signed integer overflow. The input vectors can share a buffer.
 */
inline void DanielssonUpdateCore(int32* hereV, const int32* thereV, int32 offX, int32 offY, int32 offZ, bool useSpacing, const float64 sp[3])
{
  const int32 tx = thereV[0] + offX;
  const int32 ty = thereV[1] + offY;
  const int32 tz = thereV[2] + offZ;

  const float64 norm1 = DanielssonVectorNorm(hereV, useSpacing, sp);
  float64 norm2 = 0.0;
  if(useSpacing)
  {
    const float64 t0 = static_cast<float64>(tx) * sp[0], t1 = static_cast<float64>(ty) * sp[1], t2 = static_cast<float64>(tz) * sp[2];
    norm2 = t0 * t0 + t1 * t1 + t2 * t2;
  }
  else
  {
    norm2 = static_cast<float64>(tx) * tx + static_cast<float64>(ty) * ty + static_cast<float64>(tz) * tz;
  }

  if(norm1 > norm2)
  {
    hereV[0] = tx;
    hereV[1] = ty;
    hereV[2] = tz;
  }
}

/**
 * @brief Updates a Danielsson offset vector and its tracked squared norm.
 * @param hereV Mutable current offset vector.
 * @param hereNorm Mutable squared norm of the current vector.
 * @param thereV Neighbor offset vector.
 * @param offX Candidate X offset from the neighbor.
 * @param offY Candidate Y offset from the neighbor.
 * @param offZ Candidate Z offset from the neighbor.
 * @param useSpacing Applies spacing when true.
 * @param sp Spacing for each component.
 *
 * An accepted candidate uses its calculated norm. The tracked value is exact because recomputation uses the same operands, casts, and association.
 */
inline void DanielssonUpdateCoreTracked(int32* hereV, float64& hereNorm, const int32* thereV, int32 offX, int32 offY, int32 offZ, bool useSpacing, const float64 sp[3])
{
  const int32 tx = thereV[0] + offX;
  const int32 ty = thereV[1] + offY;
  const int32 tz = thereV[2] + offZ;

  float64 norm2 = 0.0;
  if(useSpacing)
  {
    const float64 t0 = static_cast<float64>(tx) * sp[0], t1 = static_cast<float64>(ty) * sp[1], t2 = static_cast<float64>(tz) * sp[2];
    norm2 = t0 * t0 + t1 * t1 + t2 * t2;
  }
  else
  {
    norm2 = static_cast<float64>(tx) * tx + static_cast<float64>(ty) * ty + static_cast<float64>(tz) * tz;
  }

  if(hereNorm > norm2)
  {
    hereV[0] = tx;
    hereV[1] = ty;
    hereV[2] = tz;
    hereNorm = norm2;
  }
}

/** @brief Returns true when the propagated vector identifies an input seed. */
inline bool DanielssonIsFeature(const int32* vector) noexcept
{
  return vector[0] == 0 && vector[1] == 0 && vector[2] == 0;
}

/** @brief Runs one complete reflective X sweep for a row without materializing an X visit vector. */
inline void DanielssonRowPassGenerated(int32* rowVec, const int32* yNeighborVec, const int32* zNeighborVec, int64 nX, int32 ypull, int32 zpull, bool useSpacing, const float64 sp[3])
{
  auto visit = [&](int64 coordinate, int32 xPull) {
    const usize x = static_cast<usize>(coordinate);
    int32* here = rowVec + x * 3;
    if(DanielssonIsFeature(here))
    {
      return;
    }
    float64 hereNorm = DanielssonVectorNorm(here, useSpacing, sp);
    if(xPull != 0)
    {
      DanielssonUpdateCoreTracked(here, hereNorm, rowVec + static_cast<usize>(coordinate + xPull) * 3, xPull, 0, 0, useSpacing, sp);
    }
    if(ypull != 0)
    {
      DanielssonUpdateCoreTracked(here, hereNorm, yNeighborVec + x * 3, 0, ypull, 0, useSpacing, sp);
    }
    if(zpull != 0)
    {
      DanielssonUpdateCoreTracked(here, hereNorm, zNeighborVec + x * 3, 0, 0, zpull, useSpacing, sp);
    }
  };
  if(nX <= 1)
  {
    visit(0, 0);
    return;
  }
  for(int64 x = 1; x < nX; ++x)
  {
    visit(x, -1);
  }
  for(int64 x = nX - 2; x >= 0; --x)
  {
    visit(x, 1);
  }
}

/** @brief Runs the reflective Y/X odometer for one plane without retaining X/Y visit arrays. */
inline void DanielssonPlanePassGenerated(int32* planeVec, const int32* zNeighborVec, int32 zpull, int64 nX, int64 nY, bool useSpacing, const float64 sp[3])
{
  auto visitRow = [&](int64 coordinate, int32 yPull) {
    const usize row = static_cast<usize>(coordinate);
    int32* rowVec = planeVec + row * static_cast<usize>(nX) * 3;
    const int32* yNeighborVec = yPull == 0 ? nullptr : planeVec + static_cast<usize>(coordinate + yPull) * static_cast<usize>(nX) * 3;
    const int32* zRow = zpull == 0 ? nullptr : zNeighborVec + row * static_cast<usize>(nX) * 3;
    DanielssonRowPassGenerated(rowVec, yNeighborVec, zRow, nX, yPull, zpull, useSpacing, sp);
  };
  if(nY <= 1)
  {
    visitRow(0, 0);
    return;
  }
  for(int64 y = 1; y < nY; ++y)
  {
    visitRow(y, -1);
  }
  for(int64 y = nY - 2; y >= 0; --y)
  {
    visitRow(y, 1);
  }
}

/**
 * @brief Run ITK's inner (y,x) reflective odometer over ONE z-plane of the vector map. @p planeVec is the plane's
 * mutable 3-component offset buffer (length nX*nY*3); @p neighVec is the read-only offset buffer of the z-neighbor
 * plane (only dereferenced when @p zpull != 0). The resident specialization uses @p featurePlane to identify seed
 * voxels. The bounded specialization identifies a seed from its zero offset vector. For each visited voxel it pulls
 * from the x-, y-, and (when @p zpull != 0) z-neighbor in that axis's current sweep direction, in dim order x, y, z
 * (ITK order). Because the in-core and out-of-core drivers share all update logic, the two paths are byte-identical.
 */
template <bool UseFeatureMask>
void DanielssonPlanePass(int32* planeVec, const int32* neighVec, const uint8* featurePlane, const std::vector<AxisVisit>& xVisits, const std::vector<AxisVisit>& yVisits, int32 zpull, int64 nX,
                         bool useSpacing, const float64 sp[3])
{
  for(const AxisVisit& yv : yVisits)
  {
    const int64 rowBase = yv.coord * nX;
    for(const AxisVisit& xv : xVisits)
    {
      const int64 here2d = rowBase + xv.coord;
      if constexpr(UseFeatureMask)
      {
        if(featurePlane[here2d] != 0)
        {
          continue;
        }
      }
      int32* hereV = planeVec + here2d * 3;
      if constexpr(!UseFeatureMask)
      {
        if(DanielssonIsFeature(hereV))
        {
          continue;
        }
      }
      float64 hereNorm = DanielssonVectorNorm(hereV, useSpacing, sp);
      if(xv.pull != 0)
      {
        DanielssonUpdateCoreTracked(hereV, hereNorm, planeVec + (here2d + xv.pull) * 3, xv.pull, 0, 0, useSpacing, sp);
      }
      if(yv.pull != 0)
      {
        DanielssonUpdateCoreTracked(hereV, hereNorm, planeVec + (here2d + static_cast<int64>(yv.pull) * nX) * 3, 0, yv.pull, 0, useSpacing, sp);
      }
      if(zpull != 0)
      {
        DanielssonUpdateCoreTracked(hereV, hereNorm, neighVec + here2d * 3, 0, 0, zpull, useSpacing, sp);
      }
    }
  }
}

// itk::DanielssonDistanceMapImageFilter::ComputeVoronoiMap distance for one voxel: sum of (component * spacing)^2 (or
// unweighted), then sqrt unless squared. float32 output, matching the NX wrapper (distance map only).
inline float32 DanielssonDistance(const int32* v, bool squared, bool useSpacing, const float64 sp[3])
{
  float64 distance = 0.0;
  if(useSpacing)
  {
    for(int32 i = 0; i < 3; ++i)
    {
      const float64 c = static_cast<float64>(v[i]) * sp[i];
      distance += c * c;
    }
  }
  else
  {
    for(int32 i = 0; i < 3; ++i)
    {
      distance += static_cast<float64>(v[i]) * static_cast<float64>(v[i]);
    }
  }
  return squared ? static_cast<float32>(distance) : static_cast<float32>(std::sqrt(distance));
}

// The "unreached" init offset per axis: 2*maxLength for a real axis, 0 for a size-1 axis (so a 2D image contributes no
// z term). Matches ITK's PrepareData (maxValue[j] = 2 * largest-dimension).
inline void DanielssonInitMaxValue(int64 nX, int64 nY, int64 nZ, int32 maxV[3])
{
  const int64 maxLength = std::max({nX, nY, nZ});
  maxV[0] = (nX > 1) ? static_cast<int32>(2 * maxLength) : 0;
  maxV[1] = (nY > 1) ? static_cast<int32>(2 * maxLength) : 0;
  maxV[2] = (nZ > 1) ? static_cast<int32>(2 * maxLength) : 0;
}

/**
 * @brief Selects the data format for a full-volume working store.
 *
 * OOC input takes precedence, then OOC output; resident endpoints preserve the input format.
 */
inline std::string SelectDanielssonWorkingDataFormat(IDataStore::StoreType inputStoreType, std::string inputDataFormat, IDataStore::StoreType outputStoreType, std::string outputDataFormat)
{
  if(inputStoreType == IDataStore::StoreType::OutOfCore)
  {
    return inputDataFormat;
  }
  if(outputStoreType == IDataStore::StoreType::OutOfCore)
  {
    return outputDataFormat;
  }
  return inputDataFormat;
}
} // namespace detail

/**
 * @class DanielssonDistanceInCore
 * @brief Computes the resident Danielsson 4SED distance transform.
 * @tparam T Specifies the scalar input type.
 *
 * The seed phase reads a resident span when available and otherwise reads one plane at a time. Each seed range runs in parallel.
 * The reflective vector propagation is serial and matches ITK's visit order. The distance phase runs in parallel on resident data.
 * A nonzero input value is a seed with a zero offset. The output type is float32.
 * The constructor contract matches DanielssonDistanceSlab so runtime dispatch can select either implementation.
 * @pre The input and output store shapes match the image dimensions.
 */
template <class T>
class DanielssonDistanceInCore
{
public:
  DanielssonDistanceInCore(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, SizeVec3 dims, bool squaredDistance, bool useSpacing, FloatVec3 spacing,
                           const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Squared(squaredDistance)
  , m_UseSpacing(useSpacing)
  , m_Spacing(spacing)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }
  ~DanielssonDistanceInCore() = default;
  DanielssonDistanceInCore(const DanielssonDistanceInCore&) = delete;
  DanielssonDistanceInCore(DanielssonDistanceInCore&&) noexcept = delete;
  DanielssonDistanceInCore& operator=(const DanielssonDistanceInCore&) = delete;
  DanielssonDistanceInCore& operator=(DanielssonDistanceInCore&&) noexcept = delete;

  Result<> operator()()
  {
    const int64 nX = static_cast<int64>(m_Dims[0]);
    const int64 nY = static_cast<int64>(m_Dims[1]);
    const int64 nZ = static_cast<int64>(m_Dims[2]);
    const usize slice = static_cast<usize>(nX * nY);
    const usize vol = slice * static_cast<usize>(nZ);
    if(vol == 0)
    {
      return {};
    }
    if(m_ShouldCancel)
    {
      return {};
    }

    // Prepare the vector map and feature mask before the serial reflective propagation.
    int32 maxV[3];
    detail::DanielssonInitMaxValue(nX, nY, nZ, maxV);
    auto vec = std::make_unique_for_overwrite<int32[]>(vol * 3);
    auto feature = std::make_unique_for_overwrite<uint8[]>(vol);
    const auto prepareData = [vectorValuesPtr = vec.get(), featureValuesPtr = feature.get(), maxX = maxV[0], maxY = maxV[1], maxZ = maxV[2]](const T* inputValuesPtr, usize valueOffset, usize count) {
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, count);
      parallelAlgorithm.execute([=](const Range& range) {
        for(usize localIndex = range.min(); localIndex < range.max(); ++localIndex)
        {
          const usize valueIndex = valueOffset + localIndex;
          const bool isFeature = inputValuesPtr[localIndex] != static_cast<T>(0);
          featureValuesPtr[valueIndex] = isFeature ? uint8{1} : uint8{0};
          vectorValuesPtr[valueIndex * 3] = isFeature ? 0 : maxX;
          vectorValuesPtr[valueIndex * 3 + 1] = isFeature ? 0 : maxY;
          vectorValuesPtr[valueIndex * 3 + 2] = isFeature ? 0 : maxZ;
        }
      });
    };
    if(const auto* residentInputStorePtr = dynamic_cast<const DataStore<T>*>(&m_In); residentInputStorePtr != nullptr)
    {
      const auto inputValues = residentInputStorePtr->createSpan();
      prepareData(inputValues.data(), 0, vol);
    }
    else
    {
      auto inputPlane = std::make_unique_for_overwrite<T[]>(slice);
      for(int64 z = 0; z < nZ; ++z)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize planeOffset = static_cast<usize>(z) * slice;
        if(Result<> result = m_In.copyIntoBuffer(planeOffset, nonstd::span<T>(inputPlane.get(), slice)); result.invalid())
        {
          return result;
        }
        prepareData(inputPlane.get(), planeOffset, slice);
      }
    }

    // Propagate vectors with the serial reflective odometer in Z, Y, and X order.
    const float64 sp[3] = {static_cast<float64>(m_Spacing[0]), static_cast<float64>(m_Spacing[1]), static_cast<float64>(m_Spacing[2])};
    const std::vector<detail::AxisVisit> xVisits = detail::BuildAxisVisits(nX);
    const std::vector<detail::AxisVisit> yVisits = detail::BuildAxisVisits(nY);
    const std::vector<detail::AxisVisit> zVisits = detail::BuildAxisVisits(nZ);

    for(const detail::AxisVisit& zv : zVisits)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize zBase = static_cast<usize>(zv.coord) * slice;
      int32* planeVec = vec.get() + zBase * 3;
      const int32* neighVec = (zv.pull != 0) ? (vec.get() + (static_cast<usize>(zv.coord + zv.pull) * slice) * 3) : nullptr;
      const uint8* featurePlane = feature.get() + zBase;
      detail::DanielssonPlanePass<true>(planeVec, neighVec, featurePlane, xVisits, yVisits, zv.pull, nX, m_UseSpacing, sp);
    }

    const auto computeDistances = [vectorsPtr = vec.get(), squared = m_Squared, useSpacing = m_UseSpacing, spacingPtr = sp](float32* outputValuesPtr, usize vectorOffset, usize count) {
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, count);
      parallelAlgorithm.execute([=](const Range& range) {
        for(usize localIndex = range.min(); localIndex < range.max(); ++localIndex)
        {
          outputValuesPtr[localIndex] = detail::DanielssonDistance(vectorsPtr + (vectorOffset + localIndex) * 3, squared, useSpacing, spacingPtr);
        }
      });
    };

    // Convert final vectors directly into a resident output or through one bounded output plane.
    if(auto* residentOutputStorePtr = dynamic_cast<DataStore<float32>*>(&m_Out); residentOutputStorePtr != nullptr)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      auto outputValues = residentOutputStorePtr->createSpan();
      computeDistances(outputValues.data(), 0, vol);
      return {};
    }

    auto outputPlane = std::make_unique_for_overwrite<float32[]>(slice);
    for(int64 z = 0; z < nZ; ++z)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize planeOffset = static_cast<usize>(z) * slice;
      computeDistances(outputPlane.get(), planeOffset, slice);
      if(Result<> result = m_Out.copyFromBuffer(planeOffset, nonstd::span<const float32>(outputPlane.get(), slice)); result.invalid())
      {
        return result;
      }
    }
    return {};
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<float32>& m_Out;
  SizeVec3 m_Dims;
  bool m_Squared;
  bool m_UseSpacing;
  FloatVec3 m_Spacing;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/** @brief Bounded real-OOC 2-D Danielsson consumer used by the slab dispatcher when Z is one. */
template <class T>
class DanielssonDistance2DBounded
{
public:
  DanielssonDistance2DBounded(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, bool squaredDistance, bool useSpacing, FloatVec3 spacing, const std::atomic_bool& shouldCancel,
                              usize residentLimit)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Squared(squaredDistance)
  , m_UseSpacing(useSpacing)
  , m_Spacing(spacing)
  , m_ShouldCancel(shouldCancel)
  , m_ResidentLimit2D(residentLimit)
  {
  }

  Result<> operator()(usize nx, usize ny)
  {
    return RunBounded2D(nx, ny);
  }

private:
  Result<> ReadVectorScratch(const ITemporaryRecordStore& store, uint64 recordOffset, uint64 recordCount, nonstd::span<int32> vectors, nonstd::span<float32> staging, bool compactScratch,
                             std::string_view context) const
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    try
    {
      const usize cellCount = vectors.size() / 3;
      if(compactScratch && staging.size() < cellCount)
      {
        return MakeErrorResult(-8388, fmt::format("Danielsson distance-map bulk vector-scratch read failed for {}: compact staging buffer is too small", context));
      }
      auto bytes = compactScratch ? nonstd::span<std::byte>(reinterpret_cast<std::byte*>(staging.data()), cellCount * 2 * sizeof(int16)) :
                                    nonstd::span<std::byte>(reinterpret_cast<std::byte*>(vectors.data()), vectors.size() * sizeof(int32));
      Result<uint64> result = store.read(recordOffset, recordCount, bytes, m_ShouldCancel);
      if(result.invalid())
      {
        const std::string message = result.errors().empty() ? "provider returned an unspecified error" : result.errors().front().message;
        return MakeErrorResult(-8388, fmt::format("Danielsson distance-map bulk vector-scratch read failed for {}: {}", context, message));
      }
      if(result.value() != recordCount)
      {
        return MakeErrorResult(-8388, fmt::format("Danielsson distance-map bulk vector-scratch read failed for {}: returned {} of {} records", context, result.value(), recordCount));
      }
      if(compactScratch)
      {
        const auto* encoded = reinterpret_cast<const std::byte*>(staging.data());
        for(usize index = 0; index < cellCount; ++index)
        {
          int16 x = 0;
          int16 y = 0;
          std::memcpy(&x, encoded + index * 2 * sizeof(int16), sizeof(int16));
          std::memcpy(&y, encoded + (index * 2 + 1) * sizeof(int16), sizeof(int16));
          vectors[index * 3] = x;
          vectors[index * 3 + 1] = y;
          vectors[index * 3 + 2] = 0;
        }
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8388, fmt::format("Danielsson distance-map bulk vector-scratch read failed for {}: {}", context, exception.what()));
    }
    return {};
  }

  Result<> WriteVectorScratch(ITemporaryRecordStore& store, uint64 recordOffset, uint64 recordCount, nonstd::span<const int32> vectors, nonstd::span<float32> staging, bool compactScratch,
                              std::string_view context) const
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    try
    {
      const usize cellCount = vectors.size() / 3;
      if(compactScratch && staging.size() < cellCount)
      {
        return MakeErrorResult(-8388, fmt::format("Danielsson distance-map bulk vector-scratch write failed for {}: compact staging buffer is too small", context));
      }
      if(compactScratch)
      {
        auto* encoded = reinterpret_cast<std::byte*>(staging.data());
        for(usize index = 0; index < cellCount; ++index)
        {
          const int16 x = static_cast<int16>(vectors[index * 3]);
          const int16 y = static_cast<int16>(vectors[index * 3 + 1]);
          std::memcpy(encoded + index * 2 * sizeof(int16), &x, sizeof(int16));
          std::memcpy(encoded + (index * 2 + 1) * sizeof(int16), &y, sizeof(int16));
        }
      }
      auto bytes = compactScratch ? nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(staging.data()), cellCount * 2 * sizeof(int16)) :
                                    nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(vectors.data()), vectors.size() * sizeof(int32));
      Result<> result = store.write(recordOffset, recordCount, bytes, m_ShouldCancel);
      if(result.invalid())
      {
        const std::string message = result.errors().empty() ? "provider returned an unspecified error" : result.errors().front().message;
        return MakeErrorResult(-8388, fmt::format("Danielsson distance-map bulk vector-scratch write failed for {}: {}", context, message));
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8388, fmt::format("Danielsson distance-map bulk vector-scratch write failed for {}: {}", context, exception.what()));
    }
    return {};
  }

  static void InitializeVectors(nonstd::span<const T> input, nonstd::span<int32> vectors, const int32 maxValue[3])
  {
    for(usize index = 0; index < input.size(); ++index)
    {
      const bool isFeature = input[index] != static_cast<T>(0);
      vectors[index * 3] = isFeature ? 0 : maxValue[0];
      vectors[index * 3 + 1] = isFeature ? 0 : maxValue[1];
      vectors[index * 3 + 2] = isFeature ? 0 : maxValue[2];
    }
  }

  void FillDistances(nonstd::span<const int32> vectors, nonstd::span<float32> distances, const float64 spacing[3]) const
  {
    for(usize index = 0; index < distances.size(); ++index)
    {
      distances[index] = detail::DanielssonDistance(vectors.data() + index * 3, m_Squared, m_UseSpacing, spacing);
    }
  }

  Result<> RunFullWidth2D(ITemporaryRecordStore& vecStore, usize nx, usize ny, usize coreRows, usize outputChunkRows, bool compactScratch, const int32 maxValue[3], const float64 spacing[3])
  {
    if(ny == 1)
    {
      std::vector<T> input(nx);
      std::vector<int32> vectors(nx * 3);
      std::vector<float32> output(nx);
      if(Result<> result = m_In.copyIntoBuffer(0, nonstd::span<T>(input.data(), input.size())); result.invalid())
      {
        return result;
      }
      InitializeVectors(input, vectors, maxValue);
      detail::DanielssonRowPassGenerated(vectors.data(), static_cast<const int32*>(nullptr), static_cast<const int32*>(nullptr), static_cast<int64>(nx), 0, 0, m_UseSpacing, spacing);
      FillDistances(vectors, output, spacing);
      return m_Out.copyFromBuffer(0, nonstd::span<const float32>(output.data(), output.size()));
    }

    std::vector<int32> carryVectors(nx * 3);
    std::vector<float32> outputBlock(coreRows * nx);
    {
      std::vector<T> carryInput(nx);
      if(Result<> result = m_In.copyIntoBuffer(0, nonstd::span<T>(carryInput.data(), carryInput.size())); result.invalid())
      {
        return result;
      }
      InitializeVectors(carryInput, carryVectors, maxValue);
    }
    if(Result<> result = WriteVectorScratch(vecStore, 0, 1, nonstd::span<const int32>(carryVectors.data(), carryVectors.size()), outputBlock, compactScratch, "2D lower endpoint initialization");
       result.invalid())
    {
      return result;
    }

    std::vector<T> inputBlock(coreRows * nx);
    std::vector<int32> vectorBlock(coreRows * nx * 3);

    // Fuse initialization with the forward Y sweep. Row zero is the untouched lower endpoint and the carry into row one.
    for(usize yBegin = 1; yBegin < ny; yBegin += coreRows)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize rowCount = std::min(coreRows, ny - yBegin);
      const usize cellCount = rowCount * nx;
      if(Result<> result = m_In.copyIntoBuffer(yBegin * nx, nonstd::span<T>(inputBlock.data(), cellCount)); result.invalid())
      {
        return result;
      }
      InitializeVectors(nonstd::span<const T>(inputBlock.data(), cellCount), nonstd::span<int32>(vectorBlock.data(), cellCount * 3), maxValue);
      for(usize localRow = 0; localRow < rowCount; ++localRow)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        int32* currentRow = vectorBlock.data() + localRow * nx * 3;
        const int32* neighborRow = localRow == 0 ? carryVectors.data() : vectorBlock.data() + (localRow - 1) * nx * 3;
        detail::DanielssonRowPassGenerated(currentRow, neighborRow, static_cast<const int32*>(nullptr), static_cast<int64>(nx), -1, 0, m_UseSpacing, spacing);
      }
      if(Result<> result = WriteVectorScratch(vecStore, yBegin, rowCount, nonstd::span<const int32>(vectorBlock.data(), cellCount * 3), nonstd::span<float32>(outputBlock.data(), cellCount),
                                              compactScratch, "2D forward row block");
         result.invalid())
      {
        return result;
      }
      std::copy_n(vectorBlock.data() + (rowCount - 1) * nx * 3, nx * 3, carryVectors.data());
    }

    // The upper endpoint is not revisited by the backward Y sweep. Retain it as the first neighbor carry.
    if(Result<> result = ReadVectorScratch(vecStore, ny - 1, 1, nonstd::span<int32>(carryVectors.data(), carryVectors.size()), outputBlock, compactScratch, "2D upper endpoint output");
       result.invalid())
    {
      return result;
    }

    // Read each forward-state block once, finish the backward Y sweep, and emit final distances without rewriting scratch.
    auto processBackwardBlock = [&](usize yBegin, usize rowCount) -> Result<> {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize cellCount = rowCount * nx;
      if(Result<> result = ReadVectorScratch(vecStore, yBegin, rowCount, nonstd::span<int32>(vectorBlock.data(), cellCount * 3), nonstd::span<float32>(outputBlock.data(), cellCount), compactScratch,
                                             "2D backward row block");
         result.invalid())
      {
        return result;
      }
      for(usize localRow = rowCount; localRow-- > 0;)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        int32* currentRow = vectorBlock.data() + localRow * nx * 3;
        const int32* neighborRow = localRow + 1 == rowCount ? carryVectors.data() : vectorBlock.data() + (localRow + 1) * nx * 3;
        detail::DanielssonRowPassGenerated(currentRow, neighborRow, static_cast<const int32*>(nullptr), static_cast<int64>(nx), 1, 0, m_UseSpacing, spacing);
      }
      FillDistances(nonstd::span<const int32>(vectorBlock.data(), cellCount * 3), nonstd::span<float32>(outputBlock.data(), cellCount), spacing);
      return {};
    };

    usize yEnd = ny - 1;
    if(outputChunkRows > 0)
    {
      const usize edgeChunkBegin = ((ny - 1) / outputChunkRows) * outputChunkRows;
      const usize rowCount = yEnd - edgeChunkBegin;
      const usize cellCount = rowCount * nx;
      if(rowCount > 0)
      {
        if(Result<> result = processBackwardBlock(edgeChunkBegin, rowCount); result.invalid())
        {
          return result;
        }
      }
      FillDistances(carryVectors, nonstd::span<float32>(outputBlock.data() + cellCount, nx), spacing);
      if(Result<> result = m_Out.copyFromBuffer(edgeChunkBegin * nx, nonstd::span<const float32>(outputBlock.data(), (rowCount + 1) * nx)); result.invalid())
      {
        return result;
      }
      if(rowCount > 0)
      {
        std::copy_n(vectorBlock.data(), nx * 3, carryVectors.data());
      }
      yEnd = edgeChunkBegin;
    }
    else
    {
      FillDistances(carryVectors, nonstd::span<float32>(outputBlock.data(), nx), spacing);
      if(Result<> result = m_Out.copyFromBuffer((ny - 1) * nx, nonstd::span<const float32>(outputBlock.data(), nx)); result.invalid())
      {
        return result;
      }
    }

    while(yEnd > 0)
    {
      const usize rowCount = std::min(coreRows, yEnd);
      const usize yBegin = yEnd - rowCount;
      const usize cellCount = rowCount * nx;
      if(Result<> result = processBackwardBlock(yBegin, rowCount); result.invalid())
      {
        return result;
      }
      if(Result<> result = m_Out.copyFromBuffer(yBegin * nx, nonstd::span<const float32>(outputBlock.data(), cellCount)); result.invalid())
      {
        return result;
      }
      std::copy_n(vectorBlock.data(), nx * 3, carryVectors.data());
      yEnd = yBegin;
    }
    return {};
  }

  Result<> InitializeTiledRow(ITemporaryRecordStore& vecStore, usize nx, usize y, usize coreCols, usize tileCount, bool compactScratch, const int32 maxValue[3])
  {
    std::vector<T> input(coreCols);
    std::vector<int32> vectors(coreCols * 3);
    std::vector<float32> staging(coreCols);
    for(usize xBegin = 0; xBegin < nx; xBegin += coreCols)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize columnCount = std::min(coreCols, nx - xBegin);
      if(Result<> result = m_In.copyIntoBuffer(y * nx + xBegin, nonstd::span<T>(input.data(), columnCount)); result.invalid())
      {
        return result;
      }
      InitializeVectors(nonstd::span<const T>(input.data(), columnCount), nonstd::span<int32>(vectors.data(), columnCount * 3), maxValue);
      const usize recordOffset = y * tileCount + xBegin / coreCols;
      if(Result<> result = WriteVectorScratch(vecStore, recordOffset, 1, nonstd::span<const int32>(vectors.data(), vectors.size()), staging, compactScratch, "2D tiled row initialization");
         result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  Result<> WriteTiledDistanceRow(const ITemporaryRecordStore& vecStore, usize nx, usize y, usize coreCols, usize tileCount, bool compactScratch, const float64 spacing[3])
  {
    std::vector<int32> vectors(coreCols * 3);
    std::vector<float32> output(coreCols);
    for(usize xBegin = 0; xBegin < nx; xBegin += coreCols)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize columnCount = std::min(coreCols, nx - xBegin);
      const usize recordOffset = y * tileCount + xBegin / coreCols;
      if(Result<> result = ReadVectorScratch(vecStore, recordOffset, 1, nonstd::span<int32>(vectors.data(), vectors.size()), output, compactScratch, "2D tiled endpoint output"); result.invalid())
      {
        return result;
      }
      FillDistances(nonstd::span<const int32>(vectors.data(), columnCount * 3), nonstd::span<float32>(output.data(), columnCount), spacing);
      if(Result<> result = m_Out.copyFromBuffer(y * nx + xBegin, nonstd::span<const float32>(output.data(), columnCount)); result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  Result<> ProcessTiledRow(ITemporaryRecordStore& vecStore, usize nx, usize y, int32 yPull, usize coreCols, usize tileCount, bool initializeCurrent, bool writeDistance, bool compactScratch,
                           const int32 maxValue[3], const float64 spacing[3])
  {
    std::vector<T> input(coreCols);
    std::vector<int32> current(coreCols * 3);
    std::vector<int32> neighbor(coreCols * 3);
    std::vector<float32> output(coreCols);
    std::array<int32, 3> xCarry{};
    const usize neighborY = yPull == 0 ? y : static_cast<usize>(static_cast<int64>(y) + yPull);

    if(nx == 1)
    {
      if(initializeCurrent)
      {
        if(Result<> result = m_In.copyIntoBuffer(y, nonstd::span<T>(input.data(), 1)); result.invalid())
        {
          return result;
        }
        InitializeVectors(nonstd::span<const T>(input.data(), 1), nonstd::span<int32>(current.data(), 3), maxValue);
      }
      else if(Result<> result = ReadVectorScratch(vecStore, y * tileCount, 1, nonstd::span<int32>(current.data(), current.size()), output, compactScratch, "2D one-column current row");
              result.invalid())
      {
        return result;
      }
      if(yPull != 0)
      {
        if(Result<> result = ReadVectorScratch(vecStore, neighborY * tileCount, 1, nonstd::span<int32>(neighbor.data(), neighbor.size()), output, compactScratch, "2D one-column neighbor row");
           result.invalid())
        {
          return result;
        }
      }
      detail::DanielssonRowPassGenerated(current.data(), yPull == 0 ? nullptr : neighbor.data(), static_cast<const int32*>(nullptr), 1, yPull, 0, m_UseSpacing, spacing);
      if(Result<> result = WriteVectorScratch(vecStore, y * tileCount, 1, nonstd::span<const int32>(current.data(), current.size()), output, compactScratch, "2D one-column current row");
         result.invalid())
      {
        return result;
      }
      if(writeDistance)
      {
        FillDistances(nonstd::span<const int32>(current.data(), 3), nonstd::span<float32>(output.data(), 1), spacing);
        return m_Out.copyFromBuffer(y, nonstd::span<const float32>(output.data(), 1));
      }
      return {};
    }

    // X-forward visits. The first block retains x=0 as the untouched endpoint; later blocks use the prior carry.
    for(usize xBegin = 0; xBegin < nx; xBegin += coreCols)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize columnCount = std::min(coreCols, nx - xBegin);
      if(initializeCurrent)
      {
        if(Result<> result = m_In.copyIntoBuffer(y * nx + xBegin, nonstd::span<T>(input.data(), columnCount)); result.invalid())
        {
          return result;
        }
        InitializeVectors(nonstd::span<const T>(input.data(), columnCount), nonstd::span<int32>(current.data(), columnCount * 3), maxValue);
      }
      else if(Result<> result =
                  ReadVectorScratch(vecStore, y * tileCount + xBegin / coreCols, 1, nonstd::span<int32>(current.data(), current.size()), output, compactScratch, "2D tiled X-forward current row");
              result.invalid())
      {
        return result;
      }
      if(yPull != 0)
      {
        if(Result<> result = ReadVectorScratch(vecStore, neighborY * tileCount + xBegin / coreCols, 1, nonstd::span<int32>(neighbor.data(), neighbor.size()), output, compactScratch,
                                               "2D tiled X-forward neighbor row");
           result.invalid())
        {
          return result;
        }
      }
      const usize localBegin = xBegin == 0 ? 1 : 0;
      for(usize localX = localBegin; localX < columnCount; ++localX)
      {
        int32* here = current.data() + localX * 3;
        if(detail::DanielssonIsFeature(here))
        {
          continue;
        }
        const int32* left = localX == 0 ? xCarry.data() : current.data() + (localX - 1) * 3;
        detail::DanielssonUpdateCore(here, left, -1, 0, 0, m_UseSpacing, spacing);
        if(yPull != 0)
        {
          detail::DanielssonUpdateCore(here, neighbor.data() + localX * 3, 0, yPull, 0, m_UseSpacing, spacing);
        }
      }
      if(Result<> result =
             WriteVectorScratch(vecStore, y * tileCount + xBegin / coreCols, 1, nonstd::span<const int32>(current.data(), current.size()), output, compactScratch, "2D tiled X-forward current row");
         result.invalid())
      {
        return result;
      }
      std::copy_n(current.data() + (columnCount - 1) * 3, 3, xCarry.data());
    }

    // X-backward visits. The first block retains x=nx-1 as the untouched endpoint; lower blocks use the right carry.
    bool firstBlock = true;
    for(usize reverseTile = tileCount; reverseTile-- > 0;)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize tileIndex = reverseTile;
      const usize xBegin = tileIndex * coreCols;
      const usize columnCount = std::min(coreCols, nx - xBegin);
      if(Result<> result = ReadVectorScratch(vecStore, y * tileCount + tileIndex, 1, nonstd::span<int32>(current.data(), current.size()), output, compactScratch, "2D tiled X-backward current row");
         result.invalid())
      {
        return result;
      }
      if(yPull != 0)
      {
        if(Result<> result =
               ReadVectorScratch(vecStore, neighborY * tileCount + tileIndex, 1, nonstd::span<int32>(neighbor.data(), neighbor.size()), output, compactScratch, "2D tiled X-backward neighbor row");
           result.invalid())
        {
          return result;
        }
      }
      int64 localX = firstBlock ? static_cast<int64>(columnCount) - 2 : static_cast<int64>(columnCount) - 1;
      for(; localX >= 0; --localX)
      {
        const usize localIndex = static_cast<usize>(localX);
        int32* here = current.data() + localIndex * 3;
        if(detail::DanielssonIsFeature(here))
        {
          continue;
        }
        const int32* right = localIndex + 1 == columnCount ? xCarry.data() : current.data() + (localIndex + 1) * 3;
        detail::DanielssonUpdateCore(here, right, 1, 0, 0, m_UseSpacing, spacing);
        if(yPull != 0)
        {
          detail::DanielssonUpdateCore(here, neighbor.data() + localIndex * 3, 0, yPull, 0, m_UseSpacing, spacing);
        }
      }
      if(Result<> result =
             WriteVectorScratch(vecStore, y * tileCount + tileIndex, 1, nonstd::span<const int32>(current.data(), current.size()), output, compactScratch, "2D tiled X-backward current row");
         result.invalid())
      {
        return result;
      }
      if(writeDistance)
      {
        FillDistances(nonstd::span<const int32>(current.data(), columnCount * 3), nonstd::span<float32>(output.data(), columnCount), spacing);
        if(Result<> result = m_Out.copyFromBuffer(y * nx + xBegin, nonstd::span<const float32>(output.data(), columnCount)); result.invalid())
        {
          return result;
        }
      }
      std::copy_n(current.data(), 3, xCarry.data());
      firstBlock = false;
    }
    return {};
  }

  Result<> RunTiled2D(ITemporaryRecordStore& vecStore, usize nx, usize ny, usize coreCols, usize tileCount, bool compactScratch, const int32 maxValue[3], const float64 spacing[3])
  {
    if(ny == 1)
    {
      return ProcessTiledRow(vecStore, nx, 0, 0, coreCols, tileCount, true, true, compactScratch, maxValue, spacing);
    }
    if(Result<> result = InitializeTiledRow(vecStore, nx, 0, coreCols, tileCount, compactScratch, maxValue); result.invalid())
    {
      return result;
    }
    for(usize y = 1; y < ny; ++y)
    {
      if(Result<> result = ProcessTiledRow(vecStore, nx, y, -1, coreCols, tileCount, true, false, compactScratch, maxValue, spacing); result.invalid())
      {
        return result;
      }
    }
    if(Result<> result = WriteTiledDistanceRow(vecStore, nx, ny - 1, coreCols, tileCount, compactScratch, spacing); result.invalid())
    {
      return result;
    }
    for(usize y = ny - 1; y-- > 0;)
    {
      if(Result<> result = ProcessTiledRow(vecStore, nx, y, 1, coreCols, tileCount, false, true, compactScratch, maxValue, spacing); result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  Result<> RunBounded2D(usize nx, usize ny)
  {
    const detail::Danielsson2DBufferPlan plan = detail::BuildDanielsson2DBufferPlan(nx, ny, sizeof(T), m_ResidentLimit2D);
    if(plan.overflow)
    {
      return MakeErrorResult(-8385, fmt::format("Danielsson distance-map 2D buffer plan cannot represent dimensions {} x {} due to arithmetic overflow.", nx, ny));
    }
    if(!plan.valid)
    {
      return MakeErrorResult(
          -8386, fmt::format("Danielsson distance-map 2D buffer plan exceeds the {}-byte resident limit for dimensions {} x {} and {}-byte input values.", m_ResidentLimit2D, nx, ny, sizeof(T)));
    }

    const usize maxDimension = std::max(nx, ny);
    const bool compactScratch = maxDimension <= static_cast<usize>(std::numeric_limits<int16>::max() / 2);
    const usize scratchComponents = compactScratch ? 2 : 3;
    const usize scratchValueBytes = compactScratch ? sizeof(int16) : sizeof(int32);
    const usize tileCount = 1 + (nx - 1) / plan.coreCols;
    usize recordValues = 0;
    usize recordSize = 0;
    usize recordCount = 0;
    if(!detail::DanielssonCheckedMultiply(plan.coreCols, scratchComponents, recordValues) || !detail::DanielssonCheckedMultiply(recordValues, scratchValueBytes, recordSize) ||
       !detail::DanielssonCheckedMultiply(ny, tileCount, recordCount) || recordSize > std::numeric_limits<uint64>::max() || recordCount > std::numeric_limits<uint64>::max())
    {
      return MakeErrorResult(-8385, fmt::format("Danielsson distance-map 2D vector-scratch layout cannot represent dimensions {} x {} due to arithmetic overflow.", nx, ny));
    }
    TemporaryRecordStoreConfig config;
    config.recordSize = static_cast<uint64>(recordSize);
    config.maxRecordsPerBatch = static_cast<uint64>(plan.coreCols == nx ? plan.coreRows : 1);
    config.initialRecordCount = static_cast<uint64>(recordCount);
    std::unique_ptr<ITemporaryRecordStore> vecStore;
    try
    {
      auto storeResult = DataStoreUtilities::CreateTemporaryRecordStore(config);
      if(storeResult.invalid())
      {
        const std::string message = storeResult.errors().empty() ? "provider returned an unspecified error" : storeResult.errors().front().message;
        return MakeErrorResult(-8387, fmt::format("Danielsson distance-map failed to create bounded 2D fixed-record vector scratch: {}", message));
      }
      vecStore = std::move(storeResult.value());
    } catch(const std::bad_alloc& exception)
    {
      return MakeErrorResult(-8387, fmt::format("Danielsson distance-map failed to create bounded 2D fixed-record vector scratch: {}", exception.what()));
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-8387, fmt::format("Danielsson distance-map failed to create bounded 2D fixed-record vector scratch: {}", exception.what()));
    }
    if(vecStore == nullptr)
    {
      return MakeErrorResult(-8387, "Danielsson distance-map failed to create bounded 2D fixed-record vector scratch: provider returned a null store");
    }

    int32 maxValue[3];
    detail::DanielssonInitMaxValue(static_cast<int64>(nx), static_cast<int64>(ny), 1, maxValue);
    const float64 spacing[3] = {static_cast<float64>(m_Spacing[0]), static_cast<float64>(m_Spacing[1]), static_cast<float64>(m_Spacing[2])};
    if(plan.coreCols == nx)
    {
      usize blockRows = plan.coreRows;
      usize outputChunkRows = 0;
      const std::optional<ShapeType> outputChunkShape = m_Out.getChunkShape();
      if(outputChunkShape.has_value() && outputChunkShape->size() >= 3 && (*outputChunkShape)[0] == 1 && (*outputChunkShape)[1] > 0 && (*outputChunkShape)[1] <= blockRows &&
         (*outputChunkShape)[2] == nx)
      {
        outputChunkRows = (*outputChunkShape)[1];
        blockRows = (blockRows / outputChunkRows) * outputChunkRows;
      }
      return RunFullWidth2D(*vecStore, nx, ny, blockRows, outputChunkRows, compactScratch, maxValue, spacing);
    }
    return RunTiled2D(*vecStore, nx, ny, plan.coreCols, tileCount, compactScratch, maxValue, spacing);
  }

  const AbstractDataStore<T>& m_In;
  AbstractDataStore<float32>& m_Out;
  bool m_Squared;
  bool m_UseSpacing;
  FloatVec3 m_Spacing;
  const std::atomic_bool& m_ShouldCancel;
  usize m_ResidentLimit2D = detail::k_Danielsson2DResidentLimit;
};

/**
 * @brief Out-of-core Danielsson distance transform: the SAME reflective odometer, streamed plane-by-plane in z. ITK's
 * reflective scan is a reflected odometer with z outermost, so the z-cycle visits planes 1..nZ-1 (z-forward, pulling
 * the z-neighbor at z-1) then nZ-2..0 (z-backward, pulling z+1). Initialization is fused with the forward pass. The
 * last two forward states stay in the two live vector-plane buffers at turnaround. Only older forward states use the
 * disk-backed vector store, and each of those planes has one write and one read. Bounded memory: a couple of z-planes
 * of the 3-component int32 vector map plus the input/output planes. Produces output BYTE-IDENTICAL to
 * @ref DanielssonDistanceInCore (the D3 gate). Identical constructor signature so both drop into DispatchAlgorithm.
 */
template <class T>
class DanielssonDistanceSlab
{
public:
  DanielssonDistanceSlab(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, SizeVec3 dims, bool squaredDistance, bool useSpacing, FloatVec3 spacing,
                         const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize residentLimit2D = detail::k_Danielsson2DResidentLimit,
                         usize visitArrayLimit = detail::k_DanielssonVisitArrayLimit, usize residentPrefixPlanes = 0, detail::DanielssonVectorTransferStats* transferStats = nullptr)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Squared(squaredDistance)
  , m_UseSpacing(useSpacing)
  , m_Spacing(spacing)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_ResidentLimit2D(residentLimit2D)
  , m_VisitArrayLimit(visitArrayLimit)
  , m_ResidentPrefixPlanes(residentPrefixPlanes)
  , m_TransferStats(transferStats)
  {
  }
  ~DanielssonDistanceSlab() = default;
  DanielssonDistanceSlab(const DanielssonDistanceSlab&) = delete;
  DanielssonDistanceSlab(DanielssonDistanceSlab&&) noexcept = delete;
  DanielssonDistanceSlab& operator=(const DanielssonDistanceSlab&) = delete;
  DanielssonDistanceSlab& operator=(DanielssonDistanceSlab&&) noexcept = delete;

  Result<> operator()()
  {
    const int64 nX = static_cast<int64>(m_Dims[0]);
    const int64 nY = static_cast<int64>(m_Dims[1]);
    const int64 nZ = static_cast<int64>(m_Dims[2]);
    const usize slice = static_cast<usize>(nX * nY);
    const usize vol = slice * static_cast<usize>(nZ);
    if(vol == 0)
    {
      return {};
    }
    if(m_ShouldCancel)
    {
      return {};
    }

    const bool hasOutOfCoreEndpoint = m_In.getStoreType() == IDataStore::StoreType::OutOfCore || m_Out.getStoreType() == IDataStore::StoreType::OutOfCore;
    if(nZ == 1 && hasOutOfCoreEndpoint)
    {
      DanielssonDistance2DBounded<T> bounded2D(m_In, m_Out, m_Squared, m_UseSpacing, m_Spacing, m_ShouldCancel, m_ResidentLimit2D);
      return bounded2D(static_cast<usize>(nX), static_cast<usize>(nY));
    }

    // A disk-backed endpoint makes the full-volume vector map disk-backed too, whatever its resolved data format.
    // A raw fixed-record store (one int32, or one checked int16, per vector component) keeps that traffic off the
    // deflate codec entirely: the fused forward/backward Z sweep below writes each older forward plane once and
    // reads it back once. When neither endpoint is out-of-core, the vector map is plain resident memory instead, so
    // a disk-backed raw store would add pointless I/O; that route keeps the resolved in-core data format, matching
    // the prior behavior.
    const bool useRawScratch = hasOutOfCoreEndpoint;
    const std::string workingDataFormat = detail::SelectDanielssonWorkingDataFormat(m_In.getStoreType(), m_In.getDataFormat(), m_Out.getStoreType(), m_Out.getDataFormat());
    const usize storableForwardPlanes = nZ > 2 ? static_cast<usize>(nZ - 2) : 0;
    const usize prefixPlanes = std::min(m_ResidentPrefixPlanes, storableForwardPlanes);
    usize prefixValues = 0;
    if(!detail::DanielssonCheckedMultiply(prefixPlanes, slice, prefixValues) || !detail::DanielssonCheckedMultiply(prefixValues, usize{3}, prefixValues) ||
       prefixValues > std::numeric_limits<usize>::max() / sizeof(int32))
    {
      return MakeErrorResult(-8390, fmt::format("Danielsson resident vector prefix overflows for {} prefix planes and dimensions {}.", prefixPlanes, StringUtilities::formatDimensions3D(m_Dims)));
    }
    const bool compactScratch = detail::DanielssonCanUseCompactVectorScratch(m_Dims);
    const usize vectorPlaneValues = slice * 3;
    std::vector<int32> residentPrefix(prefixValues);
    std::shared_ptr<AbstractDataStore<int32>> fullVecStore;
    std::shared_ptr<AbstractDataStore<int16>> compactVecStore;
    std::unique_ptr<detail::SweepTemporaryStore<int32>> fullVecScratch;
    std::unique_ptr<detail::SweepTemporaryStore<int16>> compactVecScratch;
    std::vector<int16> compactStaging;
    std::optional<detail::DanielssonHybridVectorPlaneStore<AbstractDataStore<int32>>> fullVectorPlanesCompressed;
    std::optional<detail::DanielssonHybridVectorPlaneStore<detail::SweepTemporaryStore<int32>>> fullVectorPlanesRaw;
    std::optional<detail::DanielssonCompactHybridVectorPlaneStore<AbstractDataStore<int16>>> compactVectorPlanesCompressed;
    std::optional<detail::DanielssonCompactHybridVectorPlaneStore<detail::SweepTemporaryStore<int16>>> compactVectorPlanesRaw;
    if(compactScratch)
    {
      compactStaging.resize(vectorPlaneValues);
      if(useRawScratch)
      {
        auto scratchResult = detail::CreateSweepTemporaryStore<int16>(vol * 3, vectorPlaneValues, m_ShouldCancel, "Danielsson distance-map 3D compact vector scratch");
        if(scratchResult.invalid())
        {
          return ConvertResult(std::move(scratchResult));
        }
        compactVecScratch = std::move(scratchResult.value());
        compactVectorPlanesRaw.emplace(*compactVecScratch, residentPrefix, compactStaging, prefixPlanes, vectorPlaneValues, m_ShouldCancel, m_TransferStats);
      }
      else
      {
        compactVecStore =
            DataStoreUtilities::CreateDataStoreWithFormat<int16>(workingDataFormat, std::vector<usize>{static_cast<usize>(nZ), static_cast<usize>(nY), static_cast<usize>(nX)}, std::vector<usize>{3});
        compactVectorPlanesCompressed.emplace(*compactVecStore, residentPrefix, compactStaging, prefixPlanes, vectorPlaneValues, m_ShouldCancel, m_TransferStats);
      }
    }
    else if(useRawScratch)
    {
      auto scratchResult = detail::CreateSweepTemporaryStore<int32>(vol * 3, vectorPlaneValues, m_ShouldCancel, "Danielsson distance-map 3D vector scratch");
      if(scratchResult.invalid())
      {
        return ConvertResult(std::move(scratchResult));
      }
      fullVecScratch = std::move(scratchResult.value());
      fullVectorPlanesRaw.emplace(*fullVecScratch, residentPrefix, prefixPlanes, vectorPlaneValues, m_ShouldCancel, m_TransferStats);
    }
    else
    {
      fullVecStore =
          DataStoreUtilities::CreateDataStoreWithFormat<int32>(workingDataFormat, std::vector<usize>{static_cast<usize>(nZ), static_cast<usize>(nY), static_cast<usize>(nX)}, std::vector<usize>{3});
      fullVectorPlanesCompressed.emplace(*fullVecStore, residentPrefix, prefixPlanes, vectorPlaneValues, m_ShouldCancel, m_TransferStats);
    }

    int32 maxV[3];
    detail::DanielssonInitMaxValue(nX, nY, nZ, maxV);
    const float64 sp[3] = {static_cast<float64>(m_Spacing[0]), static_cast<float64>(m_Spacing[1]), static_cast<float64>(m_Spacing[2])};
    usize axisVisitBytes = 0;
    const bool useVisitArrays = detail::DanielssonXYVisitArraysFit(static_cast<usize>(nX), static_cast<usize>(nY), m_VisitArrayLimit, axisVisitBytes);
    std::vector<detail::AxisVisit> xVisits;
    std::vector<detail::AxisVisit> yVisits;
    if(useVisitArrays)
    {
      xVisits = detail::BuildAxisVisits(nX);
      yVisits = detail::BuildAxisVisits(nY);
    }

    std::vector<T> inPlane(slice);
    std::vector<int32> vecPlane(slice * 3);
    std::vector<int32> neighPlane(slice * 3);
    std::vector<float32> outPlane(slice);

    const auto readVectorPlane = [&](usize planeIndex, nonstd::span<int32> values) {
      if(compactScratch)
      {
        return useRawScratch ? compactVectorPlanesRaw->readPlane(planeIndex, values) : compactVectorPlanesCompressed->readPlane(planeIndex, values);
      }
      return useRawScratch ? fullVectorPlanesRaw->readPlane(planeIndex, values) : fullVectorPlanesCompressed->readPlane(planeIndex, values);
    };
    const auto writeVectorPlane = [&](usize planeIndex, nonstd::span<const int32> values) {
      if(compactScratch)
      {
        return useRawScratch ? compactVectorPlanesRaw->writePlane(planeIndex, values) : compactVectorPlanesCompressed->writePlane(planeIndex, values);
      }
      return useRawScratch ? fullVectorPlanesRaw->writePlane(planeIndex, values) : fullVectorPlanesCompressed->writePlane(planeIndex, values);
    };

    const auto initializePlane = [&](int64 z) -> Result<> {
      if(m_ShouldCancel)
      {
        return {};
      }
      if(Result<> result = m_In.copyIntoBuffer(static_cast<usize>(z) * slice, nonstd::span<T>(inPlane.data(), slice)); result.invalid())
      {
        return result;
      }
      const T* const inputValuesPtr = inPlane.data();
      int32* const vectorValuesPtr = vecPlane.data();
      const int32 maxX = maxV[0];
      const int32 maxY = maxV[1];
      const int32 maxZ = maxV[2];
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, slice);
      parallelAlgorithm.execute([=](const Range& range) {
        for(usize planeIndex = range.min(); planeIndex < range.max(); ++planeIndex)
        {
          const bool isFeature = inputValuesPtr[planeIndex] != static_cast<T>(0);
          vectorValuesPtr[planeIndex * 3] = isFeature ? 0 : maxX;
          vectorValuesPtr[planeIndex * 3 + 1] = isFeature ? 0 : maxY;
          vectorValuesPtr[planeIndex * 3 + 2] = isFeature ? 0 : maxZ;
        }
      });
      return {};
    };
    const auto passPlane = [&](int32* current, const int32* neighbor, int32 zPull) {
      if(useVisitArrays)
      {
        detail::DanielssonPlanePass<false>(current, neighbor, nullptr, xVisits, yVisits, zPull, nX, m_UseSpacing, sp);
      }
      else
      {
        detail::DanielssonPlanePassGenerated(current, neighbor, zPull, nX, nY, m_UseSpacing, sp);
      }
    };
    const auto writeDistances = [&](int64 z, nonstd::span<const int32> vectors) -> Result<> {
      const int32* const vectorValuesPtr = vectors.data();
      float32* const outputValuesPtr = outPlane.data();
      const bool squared = m_Squared;
      const bool useSpacing = m_UseSpacing;
      const float64* const spacingPtr = sp;
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, slice);
      parallelAlgorithm.execute([=](const Range& range) {
        for(usize planeIndex = range.min(); planeIndex < range.max(); ++planeIndex)
        {
          outputValuesPtr[planeIndex] = detail::DanielssonDistance(vectorValuesPtr + planeIndex * 3, squared, useSpacing, spacingPtr);
        }
      });
      return m_Out.copyFromBuffer(static_cast<usize>(z) * slice, nonstd::span<const float32>(outPlane.data(), slice));
    };

    // Fuse initialization and the forward Z sweep. Only forward states that will be revisited are stored. The final
    // forward plane stays resident at the turnaround, so it neither reaches scratch nor needs a final scratch read.
    if(Result<> result = initializePlane(0); result.invalid())
    {
      return result;
    }
    if(nZ == 1)
    {
      passPlane(vecPlane.data(), nullptr, 0);
      return writeDistances(0, vecPlane);
    }
    if(nZ > 2)
    {
      if(Result<> result = writeVectorPlane(0, nonstd::span<const int32>(vecPlane.data(), slice * 3)); result.invalid())
      {
        return result;
      }
    }
    std::swap(vecPlane, neighPlane);
    for(int64 z = 1; z < nZ; ++z)
    {
      if(Result<> result = initializePlane(z); result.invalid())
      {
        return result;
      }
      passPlane(vecPlane.data(), neighPlane.data(), -1);
      if(z + 1 < nZ)
      {
        if(z < nZ - 2)
        {
          if(Result<> result = writeVectorPlane(static_cast<usize>(z), nonstd::span<const int32>(vecPlane.data(), slice * 3)); result.invalid())
          {
            return result;
          }
        }
        std::swap(vecPlane, neighPlane);
      }
    }

    if(Result<> result = writeDistances(nZ - 1, vecPlane); result.invalid())
    {
      return result;
    }
    // The second-to-last forward state remains in neighPlane at the turnaround. Finish and output it before reading
    // the older forward states from scratch. This removes one complete scratch transfer pair.
    passPlane(neighPlane.data(), vecPlane.data(), 1);
    if(Result<> result = writeDistances(nZ - 2, neighPlane); result.invalid())
    {
      return result;
    }
    std::swap(vecPlane, neighPlane);
    for(int64 z = nZ - 3; z >= 0; --z)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      if(Result<> result = readVectorPlane(static_cast<usize>(z), nonstd::span<int32>(neighPlane.data(), slice * 3)); result.invalid())
      {
        return result;
      }
      passPlane(neighPlane.data(), vecPlane.data(), 1);
      if(Result<> result = writeDistances(z, neighPlane); result.invalid())
      {
        return result;
      }
      std::swap(vecPlane, neighPlane);
    }
    return {};
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<float32>& m_Out;
  SizeVec3 m_Dims;
  bool m_Squared;
  bool m_UseSpacing;
  FloatVec3 m_Spacing;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  usize m_ResidentLimit2D = detail::k_Danielsson2DResidentLimit;
  usize m_VisitArrayLimit = detail::k_DanielssonVisitArrayLimit;
  usize m_ResidentPrefixPlanes = 0;
  detail::DanielssonVectorTransferStats* m_TransferStats = nullptr;
};

template <class T>
class DanielssonDistanceWorkingMemory
{
public:
  DanielssonDistanceWorkingMemory(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, SizeVec3 dims, bool squaredDistance, bool useSpacing, FloatVec3 spacing,
                                  const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Squared(squaredDistance)
  , m_UseSpacing(useSpacing)
  , m_Spacing(spacing)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  Result<> operator()()
  {
    if(detail::ShouldUseDanielssonResidentState(m_Dims))
    {
      auto allocationResult = detail::ReserveDanielssonResidentWorkingMemory<T>(m_Dims);
      if(allocationResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(allocationResult));
      }
      auto allocation = std::move(allocationResult.value());
      if(allocation.holdsCompleteState())
      {
        try
        {
          return DanielssonDistanceInCore<T>{m_In, m_Out, m_Dims, m_Squared, m_UseSpacing, m_Spacing, m_ShouldCancel, m_MessageHandler}();
        } catch(const std::bad_alloc&)
        {
          // Release the complete-state reservation before entering the bounded fallback.
        }
      }
      if(allocation.reservation.sizeBytes() > 0 && allocation.reservation.sizeBytes() <= std::numeric_limits<usize>::max())
      {
        auto prefixPlanResult = detail::CreateDanielssonResidentVectorPrefixPlan<T>(m_Dims, static_cast<usize>(allocation.reservation.sizeBytes()));
        if(prefixPlanResult.invalid())
        {
          return ConvertInvalidResult<void>(std::move(prefixPlanResult));
        }
        const detail::DanielssonResidentVectorPrefixPlan& prefixPlan = prefixPlanResult.value();
        if(prefixPlan.residentPrefixPlanes > 0)
        {
          allocation.reservation.shrinkTo(static_cast<uint64>(prefixPlan.residentBytes));
          return DanielssonDistanceSlab<T>{m_In,
                                           m_Out,
                                           m_Dims,
                                           m_Squared,
                                           m_UseSpacing,
                                           m_Spacing,
                                           m_ShouldCancel,
                                           m_MessageHandler,
                                           detail::k_Danielsson2DResidentLimit,
                                           detail::k_DanielssonVisitArrayLimit,
                                           prefixPlan.residentPrefixPlanes}();
        }
      }
    }
    return DanielssonDistanceSlab<T>{m_In, m_Out, m_Dims, m_Squared, m_UseSpacing, m_Spacing, m_ShouldCancel, m_MessageHandler}();
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<float32>& m_Out;
  SizeVec3 m_Dims;
  bool m_Squared;
  bool m_UseSpacing;
  FloatVec3 m_Spacing;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @brief Danielsson distance transform, dispatched by whether the input/output arrays are disk-backed. In-core arrays
 * use the resident algorithm directly. The OOC wrapper uses it only after a complete shared working-memory reservation
 * and otherwise retains the bounded slab engine. Output is float32 regardless of input type @p T. @p inputIsBinary is
 * accepted for interface/SIMPL parity but does NOT affect the distance output (ITK's InputIsBinary only alters the
 * unemitted Voronoi label map), so it is not forwarded to the engine.
 */
template <class T>
Result<> ApplyDanielssonDistanceMap(const AbstractDataStore<T>& inStore, AbstractDataStore<float32>& outStore, const SizeVec3& dims, bool /*inputIsBinary*/, bool squaredDistance, bool useSpacing,
                                    FloatVec3 spacing, const IDataArray& inArray, IDataArray& outArray, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  if(std::max({dims[0], dims[1], dims[2]}) > detail::k_DanielssonMaxDimension)
  {
    return MakeErrorResult(-8384, fmt::format("Danielsson distance-map dimensions exceed the signed 32-bit offset range. Dimensions: {} x {} x {}; maximum supported axis length: {}.", dims[0],
                                              dims[1], dims[2], detail::k_DanielssonMaxDimension));
  }
  return DispatchAlgorithm<DanielssonDistanceInCore<T>, DanielssonDistanceWorkingMemory<T>>({&inArray, &outArray}, inStore, outStore, dims, squaredDistance, useSpacing, spacing, shouldCancel,
                                                                                            messageHandler);
}
} // namespace nx::core::ImageProcessing
