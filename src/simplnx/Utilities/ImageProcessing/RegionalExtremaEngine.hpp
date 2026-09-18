#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionOffsets.hpp" // detail::ReconOffset(s) + MakeReconstructionOffsets
#include "simplnx/Utilities/ImageProcessing/SweepTemporaryStore.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
/**
 * @brief Regional-extrema direction. Maxima keeps regional maxima (flat zones with no strictly-greater neighbor);
 *        Minima keeps regional minima. The non-extrema are set to the marker value (type lowest for maxima, type
 *        max for minima).
 */
enum class RegionalExtremaOp
{
  Maxima,
  Minima
};

namespace detail
{
struct RegionalExtremaWorkingMemoryPlan
{
  usize maxSlabValues = 0;
  usize fixedWorkValues = 0;
  usize fixedBufferValues = 0;
  usize residentBytes = 0;
};

struct RegionalExtremaWorkingMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  RegionalExtremaWorkingMemoryPlan plan;
};

inline constexpr uint32 k_RegionalExtrema3DPreferredUsefulNumerator = 7;
inline constexpr uint32 k_RegionalExtrema3DPreferredUsefulDenominator = 16;
inline constexpr uint32 k_RegionalExtrema2DPreferredUsefulNumerator = 1;
inline constexpr uint32 k_RegionalExtrema2DPreferredUsefulDenominator = 1;

inline bool TryMultiplyRegionalExtremaSize(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline bool TryAddRegionalExtremaSize(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

template <class T>
Result<usize> CalculateRegionalExtremaUsefulWorkingMemoryBytes(const SizeVec3& dims)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  if(dimX == 0 || dimY == 0 || dimZ == 0)
  {
    return {usize{0}};
  }

  usize planeValues = 0;
  usize volumeValues = 0;
  usize volumeBytes = 0;
  if(!TryMultiplyRegionalExtremaSize(dimX, dimY, planeValues) || !TryMultiplyRegionalExtremaSize(planeValues, dimZ, volumeValues) ||
     !TryMultiplyRegionalExtremaSize(volumeValues, sizeof(T), volumeBytes))
  {
    return MakeErrorResult<usize>(-8664, fmt::format("Regional extrema dimensions ({}) overflow while sizing useful working memory.", StringUtilities::formatDimensions3D(dims)));
  }

  if(dimZ == 1)
  {
    usize haloValues = 0;
    usize fullBufferValues = 0;
    usize fullBufferBytes = 0;
    usize bothBufferBytes = 0;
    usize usefulBytes = 0;
    if(!TryMultiplyRegionalExtremaSize(dimX, usize{2}, haloValues) || !TryAddRegionalExtremaSize(volumeValues, haloValues, fullBufferValues) ||
       !TryMultiplyRegionalExtremaSize(fullBufferValues, sizeof(T), fullBufferBytes) || !TryMultiplyRegionalExtremaSize(fullBufferBytes, usize{2}, bothBufferBytes) ||
       !TryAddRegionalExtremaSize(volumeBytes, bothBufferBytes, usefulBytes))
    {
      return MakeErrorResult<usize>(-8664, fmt::format("Regional extrema dimensions ({}) overflow while sizing useful 2-D working memory.", StringUtilities::formatDimensions3D(dims)));
    }
    return {usefulBytes};
  }

  usize usefulBytes = 0;
  if(!TryMultiplyRegionalExtremaSize(volumeBytes, usize{2}, usefulBytes))
  {
    return MakeErrorResult<usize>(-8664, fmt::format("Regional extrema dimensions ({}) overflow while sizing useful 3-D working memory.", StringUtilities::formatDimensions3D(dims)));
  }
  return {usefulBytes};
}

template <class T>
Result<RegionalExtremaWorkingMemoryPlan> CreateRegionalExtremaWorkingMemoryPlan(const SizeVec3& dims, usize targetBytes)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  if(dimX == 0 || dimY == 0 || dimZ == 0)
  {
    return {RegionalExtremaWorkingMemoryPlan{}};
  }

  auto usefulResult = CalculateRegionalExtremaUsefulWorkingMemoryBytes<T>(dims);
  if(usefulResult.invalid())
  {
    return ConvertInvalidResult<RegionalExtremaWorkingMemoryPlan>(std::move(usefulResult));
  }
  targetBytes = std::min(targetBytes, usefulResult.value());

  usize planeValues = 0;
  usize volumeValues = 0;
  usize volumeBytes = 0;
  if(!TryMultiplyRegionalExtremaSize(dimX, dimY, planeValues) || !TryMultiplyRegionalExtremaSize(planeValues, dimZ, volumeValues) ||
     !TryMultiplyRegionalExtremaSize(volumeValues, sizeof(T), volumeBytes))
  {
    return MakeErrorResult<RegionalExtremaWorkingMemoryPlan>(
        -8664, fmt::format("Regional extrema dimensions ({}) overflow while creating a {}-byte working-memory plan.", StringUtilities::formatDimensions3D(dims), targetBytes));
  }

  RegionalExtremaWorkingMemoryPlan plan;
  if(dimZ == 1)
  {
    usize haloValues = 0;
    usize fullUsefulBufferValues = 0;
    if(!TryMultiplyRegionalExtremaSize(dimX, usize{2}, haloValues) || !TryAddRegionalExtremaSize(volumeValues, haloValues, fullUsefulBufferValues))
    {
      return MakeErrorResult<RegionalExtremaWorkingMemoryPlan>(
          -8664, fmt::format("Regional extrema dimensions ({}) overflow while sizing the 2-D full-width buffer.", StringUtilities::formatDimensions3D(dims)));
    }

    if(targetBytes > volumeBytes)
    {
      const usize fixedBufferValues = std::min(fullUsefulBufferValues, ((targetBytes - volumeBytes) / 2) / sizeof(T));
      auto fixedPlanResult = CreateSweep2DPlan(dimX, dimY, fixedBufferValues, /*fullWidthHaloRows=*/2, /*tiledRows=*/3);
      if(fixedPlanResult.valid() && fixedPlanResult.value().fullWidth)
      {
        plan.maxSlabValues = fixedBufferValues;
        plan.fixedWorkValues = volumeValues;
        plan.fixedBufferValues = fixedBufferValues;
        plan.residentBytes = volumeBytes + 2 * fixedBufferValues * sizeof(T);
        return {plan};
      }
    }

    const usize bufferValues = std::min(fullUsefulBufferValues, (targetBytes / 2) / sizeof(T));
    auto planResult = CreateSweep2DPlan(dimX, dimY, bufferValues, /*fullWidthHaloRows=*/2, /*tiledRows=*/3);
    if(planResult.invalid())
    {
      return MakeErrorResult<RegionalExtremaWorkingMemoryPlan>(-8664, fmt::format("Regional extrema's {}-byte working-memory grant cannot hold one valid 2-D row block or tile for dimensions ({}).",
                                                                                  targetBytes, StringUtilities::formatDimensions3D(dims)));
    }
    plan.maxSlabValues = bufferValues;
    plan.residentBytes = 2 * bufferValues * sizeof(T);
    return {plan};
  }

  usize planeBytes = 0;
  usize minimumPlanes = std::min<usize>(dimZ, 3);
  usize minimumBytes = 0;
  if(!TryMultiplyRegionalExtremaSize(planeValues, sizeof(T), planeBytes) || !TryMultiplyRegionalExtremaSize(planeBytes, minimumPlanes, minimumBytes) ||
     !TryMultiplyRegionalExtremaSize(minimumBytes, usize{2}, minimumBytes) || targetBytes < minimumBytes)
  {
    return MakeErrorResult<RegionalExtremaWorkingMemoryPlan>(
        -8664, fmt::format("Regional extrema's {}-byte working-memory grant cannot hold two checked halo slabs for dimensions ({}).", targetBytes, StringUtilities::formatDimensions3D(dims)));
  }

  const usize maximumPlanesPerBuffer = std::min(dimZ, (targetBytes / 2) / planeBytes);
  const usize maximumCorePlanes = maximumPlanesPerBuffer >= dimZ ? dimZ : std::max<usize>(1, maximumPlanesPerBuffer > 2 ? maximumPlanesPerBuffer - 2 : 1);
  const usize batchCount = 1 + (dimZ - 1) / maximumCorePlanes;
  const usize corePlanes = 1 + (dimZ - 1) / batchCount;
  const usize planesPerBuffer = corePlanes >= dimZ ? dimZ : std::min(dimZ, corePlanes + 2);
  plan.maxSlabValues = planesPerBuffer * planeValues;
  plan.residentBytes = 2 * plan.maxSlabValues * sizeof(T);
  return {plan};
}

template <class T>
Result<usize> CalculateRegionalExtremaMinimumWorkingMemoryBytes(const SizeVec3& dims)
{
  auto usefulResult = CalculateRegionalExtremaUsefulWorkingMemoryBytes<T>(dims);
  if(usefulResult.invalid() || usefulResult.value() == 0)
  {
    return usefulResult;
  }
  if(dims[2] == 1)
  {
    usize minimumBytes = 0;
    if(!TryMultiplyRegionalExtremaSize(usize{18}, sizeof(T), minimumBytes))
    {
      return MakeErrorResult<usize>(-8664, fmt::format("Regional extrema dimensions ({}) overflow while sizing the minimum 2-D working-memory plan.", StringUtilities::formatDimensions3D(dims)));
    }
    return {std::min(minimumBytes, usefulResult.value())};
  }

  usize planeValues = 0;
  usize planeBytes = 0;
  if(!TryMultiplyRegionalExtremaSize(dims[0], dims[1], planeValues) || !TryMultiplyRegionalExtremaSize(planeValues, sizeof(T), planeBytes))
  {
    return MakeErrorResult<usize>(-8664, fmt::format("Regional extrema dimensions ({}) overflow while sizing the minimum working-memory plan.", StringUtilities::formatDimensions3D(dims)));
  }
  const usize minimumPlanes = std::min<usize>(dims[2], 3);
  usize minimumBytes = 0;
  if(!TryMultiplyRegionalExtremaSize(minimumPlanes, planeBytes, minimumBytes) || !TryMultiplyRegionalExtremaSize(minimumBytes, usize{2}, minimumBytes))
  {
    return MakeErrorResult<usize>(-8664, fmt::format("Regional extrema dimensions ({}) overflow while sizing the minimum working-memory plan.", StringUtilities::formatDimensions3D(dims)));
  }
  return {std::min(minimumBytes, usefulResult.value())};
}

template <class T>
Result<RegionalExtremaWorkingMemoryAllocation> ReserveRegionalExtremaWorkingMemoryPlan(const SizeVec3& dims, uint32 preferredNumerator = 0, uint32 preferredDenominator = 0)
{
  auto usefulResult = CalculateRegionalExtremaUsefulWorkingMemoryBytes<T>(dims);
  if(usefulResult.invalid())
  {
    return ConvertInvalidResult<RegionalExtremaWorkingMemoryAllocation>(std::move(usefulResult));
  }
  if(usefulResult.value() == 0)
  {
    return {RegionalExtremaWorkingMemoryAllocation{}};
  }
  auto minimumResult = CalculateRegionalExtremaMinimumWorkingMemoryBytes<T>(dims);
  if(minimumResult.invalid())
  {
    return ConvertInvalidResult<RegionalExtremaWorkingMemoryAllocation>(std::move(minimumResult));
  }

  if(preferredDenominator == 0)
  {
    preferredNumerator = dims[2] == 1 ? k_RegionalExtrema2DPreferredUsefulNumerator : k_RegionalExtrema3DPreferredUsefulNumerator;
    preferredDenominator = dims[2] == 1 ? k_RegionalExtrema2DPreferredUsefulDenominator : k_RegionalExtrema3DPreferredUsefulDenominator;
  }

  auto reservation = ReserveWorkingMemoryFraction(usefulResult.value(), preferredNumerator, preferredDenominator, minimumResult.value());
  if(reservation.sizeBytes() == 0 || reservation.sizeBytes() > std::numeric_limits<usize>::max())
  {
    return MakeErrorResult<RegionalExtremaWorkingMemoryAllocation>(
        -8664, fmt::format("Regional extrema could not reserve working memory for dimensions ({}). Useful bytes: {}; minimum bytes: {}; granted bytes: {}.", StringUtilities::formatDimensions3D(dims),
                           usefulResult.value(), minimumResult.value(), reservation.sizeBytes()));
  }
  auto planResult = CreateRegionalExtremaWorkingMemoryPlan<T>(dims, static_cast<usize>(reservation.sizeBytes()));
  if(planResult.invalid())
  {
    return ConvertInvalidResult<RegionalExtremaWorkingMemoryAllocation>(std::move(planResult));
  }
  reservation.shrinkTo(static_cast<uint64>(planResult.value().residentBytes));
  return {RegionalExtremaWorkingMemoryAllocation{std::move(reservation), std::move(planResult.value())}};
}
} // namespace detail

/**
 * @brief In-core valued regional extrema (ITK's ValuedRegionalExtrema stack flood). A pixel keeps its value iff it
 *        belongs to a regional extremum (a flat zone with no strictly-more-extreme neighbor); every other pixel is
 *        set to the marker value (type lowest for maxima, type max for minima). Out-of-bounds neighbors are skipped
 *        (ITK's marker-value boundary can never be "more extreme"). Pulls the input + a work buffer into flat RAM
 *        (2*volume in-core, like ReconstructVincent) plus an explicit DFS stack that is O(volume) worst case (a
 *        single large flat zone), so the peak is 2*volume + the stack. The comparator is fixed at compile time by @p op.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
class RegionalExtremaFlood
{
public:
  RegionalExtremaFlood(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, RegionalExtremaOp op, bool fullyConnected, const std::atomic_bool& shouldCancel,
                       const IFilter::MessageHandler& messageHandler)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Op(op)
  , m_FullyConnected(fullyConnected)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }
  ~RegionalExtremaFlood() = default;
  RegionalExtremaFlood(const RegionalExtremaFlood&) = delete;
  RegionalExtremaFlood(RegionalExtremaFlood&&) noexcept = delete;
  RegionalExtremaFlood& operator=(const RegionalExtremaFlood&) = delete;
  RegionalExtremaFlood& operator=(RegionalExtremaFlood&&) noexcept = delete;

  Result<> operator()();

private:
  /// @brief Runs the stack-flood for a fixed direction: @c Maxima selects maxima (true) or minima (false), so the
  ///        hot loops carry no runtime branch on the direction.
  template <bool Maxima>
  Result<> runImpl();

  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  RegionalExtremaOp m_Op;
  bool m_FullyConnected;
  const std::atomic_bool& m_ShouldCancel;
  // Retained so both engines share the identical constructor signature DispatchAlgorithm requires. Progress is not
  // emitted at the engine level (consistent with the other ImageProcessing engines; the calling filter reports status).
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @brief Out-of-core valued regional extrema: a streamed iterate-to-stability sweep. Seeds every pixel that has a
 *        strictly-more-extreme input neighbor to the marker value, then propagates the marker value through
 *        equal-input-value connected zones (a pixel becomes the marker value if a same-input-value neighbor is
 *        already the marker value) with alternating forward/reverse plane sweeps until a full pass makes no change.
 *        Monotone (the marker value only spreads) -> terminates; reaches the same unique fixpoint as the flood.
 *        Bounded memory (consecutive Z-plane slabs with one halo plane on each side). Both implementations have
 *        identical required constructor arguments for DispatchAlgorithm.
 *
 * @note
 * Production dispatch converts a shared total-byte reservation into checked slab and fixed-work capacities. The
 * optional value budget remains a low-level test seam.
 *
 * @tparam T scalar voxel type.
 */
template <class T>
class RegionalExtremaSweep
{
public:
  static constexpr usize k_DefaultSlabValues = std::max<usize>(1, (16ULL * 1024ULL * 1024ULL) / sizeof(T));
  static constexpr usize k_DefaultFixed2DWorkValues = std::max<usize>(1, (36ULL * 1024ULL * 1024ULL) / sizeof(T));
  static constexpr usize k_DefaultFixed2DBufferValues = std::max<usize>(1, (14ULL * 1024ULL * 1024ULL) / sizeof(T));

  RegionalExtremaSweep(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, RegionalExtremaOp op, bool fullyConnected, const std::atomic_bool& shouldCancel,
                       const IFilter::MessageHandler& messageHandler, usize maxSlabValues = k_DefaultSlabValues, usize fixed2DWorkValues = k_DefaultFixed2DWorkValues,
                       usize fixed2DBufferValues = k_DefaultFixed2DBufferValues)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Op(op)
  , m_FullyConnected(fullyConnected)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_MaxSlabValues(std::max<usize>(1, maxSlabValues))
  , m_Fixed2DWorkValues(fixed2DWorkValues)
  , m_Fixed2DBufferValues(fixed2DBufferValues)
  {
  }
  ~RegionalExtremaSweep() = default;
  RegionalExtremaSweep(const RegionalExtremaSweep&) = delete;
  RegionalExtremaSweep(RegionalExtremaSweep&&) noexcept = delete;
  RegionalExtremaSweep& operator=(const RegionalExtremaSweep&) = delete;
  RegionalExtremaSweep& operator=(RegionalExtremaSweep&&) noexcept = delete;

  Result<> operator()();

private:
  /// @brief Runs one direction (@c Maxima) of the out-of-core streamed sweep: seeds every pixel with a
  ///        strictly-more-extreme neighbor to the marker value, then propagates the marker value through
  ///        equal-input-value connected zones until a full pass changes nothing. Routes to the 2-D full-width,
  ///        2-D tiled, or 3-D slab sweep depending on @c m_Dims and how much working memory is available; see
  ///        the class documentation above for the full routing description.
  template <bool Maxima>
  Result<> runImpl();

  /// @brief The 2-D (single-plane, dimZ == 1) seed-and-propagate sweep, specialized for either the full-width
  ///        row-block layout or the tiled-column layout chosen by detail::CreateSweep2DPlan.
  template <bool Maxima, class WorkStore>
  Result<> run2D(WorkStore& workStore, usize dimX, usize dimY, usize maxBufferValues);

  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  RegionalExtremaOp m_Op;
  bool m_FullyConnected;
  const std::atomic_bool& m_ShouldCancel;
  // Retained so both engines share the identical constructor signature DispatchAlgorithm requires. Progress is not
  // emitted at the engine level (consistent with the other ImageProcessing engines; the calling filter reports status).
  const IFilter::MessageHandler& m_MessageHandler;
  const usize m_MaxSlabValues;
  const usize m_Fixed2DWorkValues;
  const usize m_Fixed2DBufferValues;
};

// -----------------------------------------------------------------------------
// Declare our extern templates. RegionalExtremaFlood<T>::operator() and RegionalExtremaSweep<T>::operator() are
// defined out-of-line in RegionalExtremaFlood.cpp and RegionalExtremaSweep.cpp respectively; these declarations
// tell every other translation unit that includes this header not to implicitly instantiate those specializations
// locally, matching the explicit instantiation definitions provided by the two .cpp files.
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<int8>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<uint8>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<int16>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<uint16>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<int32>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<uint32>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<int64>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<uint64>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<float32>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<float64>;

extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<int8>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<uint8>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<int16>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<uint16>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<int32>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<uint32>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<int64>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<uint64>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<float32>;
extern template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<float64>;

template <class T>
class RegionalExtremaSweepWithWorkingMemory
{
public:
  RegionalExtremaSweepWithWorkingMemory(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, RegionalExtremaOp op, bool fullyConnected,
                                        const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  : m_In(inStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Op(op)
  , m_FullyConnected(fullyConnected)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  Result<> operator()()
  {
    auto allocationResult = detail::ReserveRegionalExtremaWorkingMemoryPlan<T>(m_Dims);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    const auto& plan = allocation.plan;
    return RegionalExtremaSweep<T>{m_In, m_Out, m_Dims, m_Op, m_FullyConnected, m_ShouldCancel, m_MessageHandler, plan.maxSlabValues, plan.fixedWorkValues, plan.fixedBufferValues}();
  }

private:
  const AbstractDataStore<T>& m_In;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  RegionalExtremaOp m_Op;
  bool m_FullyConnected;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

/**
 * @brief Public entry: valued regional extrema of @p inStore into @p outStore, routing the in-core flood vs the
 *        out-of-core streamed sweep via DispatchAlgorithm (both produce the identical unique fixpoint). @p op
 *        selects direction; @p fullyConnected the connectivity.
 *
 * @tparam T scalar type.
 */
template <class T>
Result<> ApplyValuedRegionalExtrema(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, RegionalExtremaOp op, bool fullyConnected, const IDataArray& inArray,
                                    const IDataArray& outArray, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  return DispatchAlgorithm<RegionalExtremaFlood<T>, RegionalExtremaSweepWithWorkingMemory<T>>({&inArray, &outArray}, inStore, outStore, dims, op, fullyConnected, shouldCancel, messageHandler);
}
} // namespace nx::core::ImageProcessing
