#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionOffsets.hpp"
#include "simplnx/Utilities/ImageProcessing/SweepTemporaryStore.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

namespace nx::core::ImageProcessing
{
/**
 * @brief The morphological reconstruction direction. Reconstruction-by-dilation grows the marker toward maxima,
 *        bounded above by the mask (requires marker <= mask); reconstruction-by-erosion shrinks toward minima,
 *        bounded below by the mask (requires marker >= mask).
 */
enum class ReconstructOp
{
  Dilation,
  Erosion
};

enum class ReconstructionMarkerSource
{
  Provided,
  MaskMinusHeight,
  MaskPlusHeight,
  MaskWithMinimumInterior,
  MaskWithMaximumInterior
};

struct ReconstructionMarkerOptions
{
  ReconstructionMarkerSource source = ReconstructionMarkerSource::Provided;
  float64 height = 0.0;
};

namespace detail
{
template <class T>
T SaturateReconstructionMarkerValue(float64 value)
{
  if constexpr(std::is_integral_v<T>)
  {
    if(std::isnan(value))
    {
      return T{0};
    }
    constexpr int k_Bits = 8 * static_cast<int>(sizeof(T));
    const float64 lowestExact = static_cast<float64>(std::numeric_limits<T>::lowest());
    const float64 upperExclusive = std::is_signed_v<T> ? std::ldexp(1.0, k_Bits - 1) : std::ldexp(1.0, k_Bits);
    if(value < lowestExact)
    {
      return std::numeric_limits<T>::lowest();
    }
    if(value >= upperExclusive)
    {
      return std::numeric_limits<T>::max();
    }
    return static_cast<T>(value);
  }
  else
  {
    return static_cast<T>(value);
  }
}

template <class T>
void ApplyReconstructionMarkerOptions(nonstd::span<T> values, usize start, const SizeVec3& dims, const ReconstructionMarkerOptions& options)
{
  if(options.source == ReconstructionMarkerSource::Provided)
  {
    return;
  }
  if(options.source == ReconstructionMarkerSource::MaskMinusHeight || options.source == ReconstructionMarkerSource::MaskPlusHeight)
  {
    const float64 signedHeight = options.source == ReconstructionMarkerSource::MaskMinusHeight ? -options.height : options.height;
    for(T& value : values)
    {
      value = SaturateReconstructionMarkerValue<T>(static_cast<float64>(value) + signedHeight);
    }
    return;
  }

  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const T interiorValue = options.source == ReconstructionMarkerSource::MaskWithMinimumInterior ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
  for(usize localIndex = 0; localIndex < values.size(); ++localIndex)
  {
    const usize globalIndex = start + localIndex;
    const usize x = globalIndex % dimX;
    const usize yz = globalIndex / dimX;
    const usize y = yz % dimY;
    const usize z = yz / dimY;
    const bool zBorder = dimZ > 1 && (z == 0 || z + 1 == dimZ);
    const bool border = x == 0 || x + 1 == dimX || y == 0 || y + 1 == dimY || zBorder;
    if(!border)
    {
      values[localIndex] = interiorValue;
    }
  }
}

/**
 * @brief Compile-time comparator + fold + mask-clamp for the two reconstruction directions, so the hot loops
 *        carry no runtime branch on the op (mirrors MorphDirect's runImpl<Dilate>). @c compare(a,b) is a>b for
 *        dilation / a<b for erosion; @c fold is max/min; @c clampToMask is min(value,mask)/max(value,mask).
 */
template <class T, bool Dilation>
struct ReconTraits
{
  static bool compare(T a, T b)
  {
    if constexpr(Dilation)
    {
      return a > b;
    }
    else
    {
      return a < b;
    }
  }
  static T fold(T acc, T v)
  {
    if constexpr(Dilation)
    {
      return (v > acc) ? v : acc;
    }
    else
    {
      return (v < acc) ? v : acc;
    }
  }
  static T clampToMask(T value, T mask)
  {
    if constexpr(Dilation)
    {
      return (value > mask) ? mask : value;
    }
    else
    {
      return (value < mask) ? mask : value;
    }
  }
};

template <class IndexT>
class BoundedReconstructionQueue
{
public:
  explicit BoundedReconstructionQueue(usize capacity)
  : m_Values(capacity == 0 ? nullptr : std::unique_ptr<IndexT[]>(new IndexT[capacity]))
  , m_Capacity(capacity)
  {
  }

  bool tryPush(IndexT value)
  {
    if(m_Size == m_Capacity)
    {
      return false;
    }
    m_Values[m_Tail] = value;
    m_Tail++;
    if(m_Tail == m_Capacity)
    {
      m_Tail = 0;
    }
    m_Size++;
    return true;
  }

  IndexT front() const
  {
    return m_Values[m_Head];
  }

  void pop()
  {
    m_Head++;
    if(m_Head == m_Capacity)
    {
      m_Head = 0;
    }
    m_Size--;
  }

  bool empty() const
  {
    return m_Size == 0;
  }

  usize capacity() const
  {
    return m_Capacity;
  }

private:
  std::unique_ptr<IndexT[]> m_Values;
  usize m_Capacity = 0;
  usize m_Head = 0;
  usize m_Tail = 0;
  usize m_Size = 0;
};

struct ReconstructionWorkingMemoryPlan
{
  usize maxSlabValues = 0;
  usize fixedWorkValues = 0;
  usize fixedBufferValues = 0;
  usize residentBytes = 0;
  bool useResidentFullSweep = false;
};

struct ReconstructionWorkingMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  ReconstructionWorkingMemoryPlan plan;
};

struct ReconstructionPersistentPrefixPlan
{
  usize residentPlanes = 0;
  usize residentValues = 0;
  usize residentBytes = 0;
};

inline constexpr uint32 k_ReconstructionPreferredUsefulNumerator = 1;
inline constexpr uint32 k_ReconstructionPreferredUsefulDenominator = 1;
inline constexpr uint32 k_ReconstructionDerivedMarker3DPreferredUsefulNumerator = 1;
inline constexpr uint32 k_ReconstructionDerivedMarker3DPreferredUsefulDenominator = 4;

struct ReconstructionWorkingMemoryFraction
{
  uint32 numerator = k_ReconstructionPreferredUsefulNumerator;
  uint32 denominator = k_ReconstructionPreferredUsefulDenominator;
};

inline ReconstructionWorkingMemoryFraction SelectReconstructionWorkingMemoryFraction(const SizeVec3& dims, ReconstructionMarkerSource source)
{
  if(dims[2] > 1 && (source == ReconstructionMarkerSource::MaskMinusHeight || source == ReconstructionMarkerSource::MaskPlusHeight))
  {
    return {k_ReconstructionDerivedMarker3DPreferredUsefulNumerator, k_ReconstructionDerivedMarker3DPreferredUsefulDenominator};
  }
  return {};
}

inline bool TryMultiplyReconstructionSize(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline bool TryAddReconstructionSize(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

inline Result<usize> CalculateReconstructionFrontierWorkingMemoryBytes(const SizeVec3& dims)
{
  usize planeValues = 0;
  usize volumeValues = 0;
  if(!TryMultiplyReconstructionSize(dims[0], dims[1], planeValues) || !TryMultiplyReconstructionSize(planeValues, dims[2], volumeValues))
  {
    return MakeErrorResult<usize>(-8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing the bounded frontier queue.", StringUtilities::formatDimensions3D(dims)));
  }
  if(volumeValues == 0)
  {
    return {usize{0}};
  }

  // One compact entry per eight voxels covered the measured 256M frontier. The queue is optional and may still
  // overflow for other data. In that case, the resident sweep completes the same monotone reconstruction.
  constexpr usize k_VoxelsPerFrontierEntry = 8;
  const usize frontierValues = volumeValues / k_VoxelsPerFrontierEntry + static_cast<usize>(volumeValues % k_VoxelsPerFrontierEntry != 0);
  usize frontierBytes = 0;
  if(!TryMultiplyReconstructionSize(frontierValues, sizeof(uint64), frontierBytes))
  {
    return MakeErrorResult<usize>(-8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing the bounded frontier queue.", StringUtilities::formatDimensions3D(dims)));
  }
  return {frontierBytes};
}

template <class T>
Result<ReconstructionPersistentPrefixPlan> CreateReconstructionPersistentPrefixPlan(const SizeVec3& dims, usize maxSlabValues)
{
  usize planeValues = 0;
  if(!TryMultiplyReconstructionSize(dims[0], dims[1], planeValues))
  {
    return MakeErrorResult<ReconstructionPersistentPrefixPlan>(
        -8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing the persistent work region.", StringUtilities::formatDimensions3D(dims)));
  }
  if(planeValues == 0 || dims[2] <= 1)
  {
    return {ReconstructionPersistentPrefixPlan{}};
  }

  const usize slabPlanes = maxSlabValues / planeValues;
  if(slabPlanes < 2)
  {
    return {ReconstructionPersistentPrefixPlan{}};
  }

  usize doubledSlabPlanes = 0;
  if(!TryMultiplyReconstructionSize(slabPlanes, usize{2}, doubledSlabPlanes))
  {
    return MakeErrorResult<ReconstructionPersistentPrefixPlan>(
        -8643, fmt::format("Morphological reconstruction's slab capacity ({} planes) overflowed while sizing the persistent work region for dimensions ({}).", slabPlanes,
                           StringUtilities::formatDimensions3D(dims)));
  }

  ReconstructionPersistentPrefixPlan plan;
  plan.residentPlanes = std::min(dims[2] - 1, doubledSlabPlanes - 2);
  usize totalBufferPlanes = 0;
  usize totalBufferValues = 0;
  if(!TryMultiplyReconstructionSize(plan.residentPlanes, planeValues, plan.residentValues) || !TryAddReconstructionSize(plan.residentPlanes, usize{3}, totalBufferPlanes) ||
     !TryMultiplyReconstructionSize(totalBufferPlanes, planeValues, totalBufferValues) || !TryMultiplyReconstructionSize(totalBufferValues, sizeof(T), plan.residentBytes))
  {
    return MakeErrorResult<ReconstructionPersistentPrefixPlan>(-8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing {} persistent planes and three streaming planes.",
                                                                                  StringUtilities::formatDimensions3D(dims), plan.residentPlanes));
  }
  return {plan};
}

template <class T>
Result<usize> CalculateReconstructionUsefulWorkingMemoryBytes(const SizeVec3& dims)
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
  if(!TryMultiplyReconstructionSize(dimX, dimY, planeValues) || !TryMultiplyReconstructionSize(planeValues, dimZ, volumeValues) || !TryMultiplyReconstructionSize(volumeValues, sizeof(T), volumeBytes))
  {
    return MakeErrorResult<usize>(-8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing useful working memory.", StringUtilities::formatDimensions3D(dims)));
  }

  if(dimZ == 1)
  {
    usize fullBufferValues = 0;
    usize fullBufferBytes = 0;
    usize bothBufferBytes = 0;
    usize usefulBytes = 0;
    if(!TryAddReconstructionSize(volumeValues, dimX, fullBufferValues) || !TryMultiplyReconstructionSize(fullBufferValues, sizeof(T), fullBufferBytes) ||
       !TryMultiplyReconstructionSize(fullBufferBytes, usize{2}, bothBufferBytes) || !TryAddReconstructionSize(volumeBytes, bothBufferBytes, usefulBytes))
    {
      return MakeErrorResult<usize>(-8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing useful 2-D working memory.", StringUtilities::formatDimensions3D(dims)));
    }
    return {usefulBytes};
  }

  usize planeBytes = 0;
  usize bothVolumeBytes = 0;
  usize usefulBytes = 0;
  if(!TryMultiplyReconstructionSize(planeValues, sizeof(T), planeBytes) || !TryMultiplyReconstructionSize(volumeBytes, usize{2}, bothVolumeBytes) ||
     !TryAddReconstructionSize(bothVolumeBytes, planeBytes, usefulBytes))
  {
    return MakeErrorResult<usize>(-8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing useful 3-D working memory.", StringUtilities::formatDimensions3D(dims)));
  }
  return {usefulBytes};
}

template <class T>
Result<ReconstructionWorkingMemoryPlan> CreateReconstructionWorkingMemoryPlan(const SizeVec3& dims, usize targetBytes)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  if(dimX == 0 || dimY == 0 || dimZ == 0)
  {
    return {ReconstructionWorkingMemoryPlan{}};
  }

  auto usefulResult = CalculateReconstructionUsefulWorkingMemoryBytes<T>(dims);
  if(usefulResult.invalid())
  {
    return ConvertInvalidResult<ReconstructionWorkingMemoryPlan>(std::move(usefulResult));
  }
  targetBytes = std::min(targetBytes, usefulResult.value());

  usize planeValues = 0;
  usize volumeValues = 0;
  usize volumeBytes = 0;
  if(!TryMultiplyReconstructionSize(dimX, dimY, planeValues) || !TryMultiplyReconstructionSize(planeValues, dimZ, volumeValues) || !TryMultiplyReconstructionSize(volumeValues, sizeof(T), volumeBytes))
  {
    return MakeErrorResult<ReconstructionWorkingMemoryPlan>(
        -8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while creating a {}-byte working-memory plan.", StringUtilities::formatDimensions3D(dims), targetBytes));
  }

  ReconstructionWorkingMemoryPlan plan;
  if(dimZ == 1)
  {
    usize fullUsefulBufferValues = 0;
    if(!TryAddReconstructionSize(volumeValues, dimX, fullUsefulBufferValues))
    {
      return MakeErrorResult<ReconstructionWorkingMemoryPlan>(
          -8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing the 2-D full-width buffer.", StringUtilities::formatDimensions3D(dims)));
    }

    if(targetBytes > volumeBytes)
    {
      const usize fixedBufferValues = std::min(fullUsefulBufferValues, ((targetBytes - volumeBytes) / 2) / sizeof(T));
      auto fixedPlanResult = CreateSweep2DPlan(dimX, dimY, fixedBufferValues, /*fullWidthHaloRows=*/1, /*tiledRows=*/1);
      if(fixedPlanResult.valid() && fixedPlanResult.value().fullWidth)
      {
        plan.maxSlabValues = fixedBufferValues;
        plan.fixedWorkValues = volumeValues;
        plan.fixedBufferValues = fixedBufferValues;
        plan.residentBytes = volumeBytes + 2 * fixedBufferValues * sizeof(T);
        return {plan};
      }
    }

    const usize fullWidthBufferValues = std::min(fullUsefulBufferValues, (targetBytes / 2) / sizeof(T));
    auto fullWidthPlanResult = CreateSweep2DPlan(dimX, dimY, fullWidthBufferValues, /*fullWidthHaloRows=*/1, /*tiledRows=*/1);
    if(fullWidthPlanResult.valid() && fullWidthPlanResult.value().fullWidth)
    {
      const usize maximumReadRows = std::min(dimY, fullWidthPlanResult.value().coreRows + 1);
      plan.maxSlabValues = fullWidthBufferValues;
      plan.residentBytes = (maximumReadRows + fullWidthPlanResult.value().coreRows) * dimX * sizeof(T);
      return {plan};
    }

    const usize targetValues = targetBytes / sizeof(T);
    const usize tiledBufferValues = std::min(fullUsefulBufferValues, targetValues / 3 + ((targetValues % 3) + 4) / 3);
    auto tiledPlanResult = CreateSweep2DPlan(dimX, dimY, tiledBufferValues, /*fullWidthHaloRows=*/1, /*tiledRows=*/1);
    if(tiledPlanResult.invalid() || tiledPlanResult.value().fullWidth)
    {
      return MakeErrorResult<ReconstructionWorkingMemoryPlan>(
          -8643, fmt::format("Morphological reconstruction's {}-byte working-memory grant cannot hold one valid 2-D row block or tile for dimensions ({}).", targetBytes,
                             StringUtilities::formatDimensions3D(dims)));
    }
    plan.maxSlabValues = tiledBufferValues;
    plan.residentBytes = (3 * tiledPlanResult.value().coreColumns + 2) * sizeof(T);
    return {plan};
  }

  usize planeBytes = 0;
  usize minimumBytes = 0;
  if(!TryMultiplyReconstructionSize(planeValues, sizeof(T), planeBytes) || !TryMultiplyReconstructionSize(planeBytes, usize{3}, minimumBytes) || targetBytes < minimumBytes)
  {
    return MakeErrorResult<ReconstructionWorkingMemoryPlan>(
        -8643, fmt::format("Morphological reconstruction's {}-byte working-memory grant cannot hold two working planes and one boundary plane for dimensions ({}).", targetBytes,
                           StringUtilities::formatDimensions3D(dims)));
  }

  const usize bytesPerSlab = (targetBytes - planeBytes) / 2;
  const usize slabPlanes = std::min(dimZ, bytesPerSlab / planeBytes);
  plan.maxSlabValues = slabPlanes * planeValues;
  plan.residentBytes = (2 * plan.maxSlabValues + planeValues) * sizeof(T);
  plan.useResidentFullSweep = plan.maxSlabValues == volumeValues;
  return {plan};
}

template <class T>
Result<usize> CalculateReconstructionMinimumWorkingMemoryBytes(const SizeVec3& dims)
{
  auto usefulResult = CalculateReconstructionUsefulWorkingMemoryBytes<T>(dims);
  if(usefulResult.invalid() || usefulResult.value() == 0)
  {
    return usefulResult;
  }
  if(dims[2] == 1)
  {
    usize minimumBytes = 0;
    if(!TryMultiplyReconstructionSize(usize{5}, sizeof(T), minimumBytes))
    {
      return MakeErrorResult<usize>(-8643,
                                    fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing the minimum 2-D working-memory plan.", StringUtilities::formatDimensions3D(dims)));
    }
    return {std::min(minimumBytes, usefulResult.value())};
  }

  usize planeValues = 0;
  usize planeBytes = 0;
  if(!TryMultiplyReconstructionSize(dims[0], dims[1], planeValues) || !TryMultiplyReconstructionSize(planeValues, sizeof(T), planeBytes))
  {
    return MakeErrorResult<usize>(-8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing the minimum working-memory plan.", StringUtilities::formatDimensions3D(dims)));
  }
  usize minimumBytes = 0;
  if(!TryMultiplyReconstructionSize(planeBytes, usize{3}, minimumBytes))
  {
    return MakeErrorResult<usize>(-8643, fmt::format("Morphological reconstruction dimensions ({}) overflow while sizing the minimum working-memory plan.", StringUtilities::formatDimensions3D(dims)));
  }
  return {std::min(minimumBytes, usefulResult.value())};
}

template <class T>
Result<ReconstructionWorkingMemoryAllocation> ReserveReconstructionWorkingMemoryPlan(const SizeVec3& dims, uint32 preferredNumerator = 0, uint32 preferredDenominator = 0)
{
  auto usefulResult = CalculateReconstructionUsefulWorkingMemoryBytes<T>(dims);
  if(usefulResult.invalid())
  {
    return ConvertInvalidResult<ReconstructionWorkingMemoryAllocation>(std::move(usefulResult));
  }
  if(usefulResult.value() == 0)
  {
    return {ReconstructionWorkingMemoryAllocation{}};
  }
  auto minimumResult = CalculateReconstructionMinimumWorkingMemoryBytes<T>(dims);
  if(minimumResult.invalid())
  {
    return ConvertInvalidResult<ReconstructionWorkingMemoryAllocation>(std::move(minimumResult));
  }

  if(preferredDenominator == 0)
  {
    preferredNumerator = k_ReconstructionPreferredUsefulNumerator;
    preferredDenominator = k_ReconstructionPreferredUsefulDenominator;
  }

  auto reservation = ReserveWorkingMemoryFraction(usefulResult.value(), preferredNumerator, preferredDenominator, minimumResult.value());
  if(reservation.sizeBytes() == 0 || reservation.sizeBytes() > std::numeric_limits<usize>::max())
  {
    return MakeErrorResult<ReconstructionWorkingMemoryAllocation>(
        -8643, fmt::format("Morphological reconstruction could not reserve working memory for dimensions ({}). Useful bytes: {}; minimum bytes: {}; granted bytes: {}.",
                           StringUtilities::formatDimensions3D(dims), usefulResult.value(), minimumResult.value(), reservation.sizeBytes()));
  }
  auto planResult = CreateReconstructionWorkingMemoryPlan<T>(dims, static_cast<usize>(reservation.sizeBytes()));
  if(planResult.invalid())
  {
    return ConvertInvalidResult<ReconstructionWorkingMemoryAllocation>(std::move(planResult));
  }
  return {ReconstructionWorkingMemoryAllocation{std::move(reservation), std::move(planResult.value())}};
}
} // namespace detail

/**
 * @brief In-core morphological reconstruction: ITK's Vincent 3-phase hybrid (fast path). Pulls the marker (as the
 *        working buffer) + mask into flat RAM buffers, reconstructs in place, and writes the result to @p outStore.
 *        This engine's own footprint is 2*volume (work + mask) plus a bounded phase-3 frontier. The default frontier
 *        holds one 64-bit index per eight voxels. If it fills, resident raster sweeps complete the reconstruction;
 *        the calling filter additionally holds the input, the output, and (for the marker-building filters) a
 *        full-volume marker scratch, so the filter-level peak is higher than 2*volume. The algorithm is genuinely
 *        whole-image. Out-of-bounds neighbors are
 *        skipped, equivalent to ITK's ∓∞ boundary fill (for a max-fold a -inf neighbor never wins; for a min-fold
 *        +inf never wins). The comparator is fixed at compile time by @p op via ReconTraits.
 *
 * @pre scalar (1 component); marker/mask/out all hold dimX*dimY*dimZ values; the marker satisfies the direction
 *      precondition (dilation marker<=mask, erosion marker>=mask) -- the callers guarantee it by construction.
 * @tparam T scalar voxel type.
 */
template <class T>
class ReconstructVincent
{
public:
  ReconstructVincent(const AbstractDataStore<T>& markerStore, const AbstractDataStore<T>& maskStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, ReconstructOp op, bool fullyConnected,
                     const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, ReconstructionMarkerOptions markerOptions = {},
                     usize frontierCapacity = std::numeric_limits<usize>::max())
  : m_Marker(markerStore)
  , m_Mask(maskStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Op(op)
  , m_FullyConnected(fullyConnected)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_MarkerOptions(markerOptions)
  , m_FrontierCapacity(frontierCapacity)
  {
  }
  ~ReconstructVincent() = default;
  ReconstructVincent(const ReconstructVincent&) = delete;
  ReconstructVincent(ReconstructVincent&&) noexcept = delete;
  ReconstructVincent& operator=(const ReconstructVincent&) = delete;
  ReconstructVincent& operator=(ReconstructVincent&&) noexcept = delete;

  Result<> operator()();

private:
  /// @brief Runs the Vincent hybrid for a fixed direction: @c Dilation selects dilation (true) or erosion (false)
  ///        via detail::ReconTraits, so the hot loops carry no runtime branch on the direction.
  template <bool Dilation>
  Result<> runImpl();

  const AbstractDataStore<T>& m_Marker;
  const AbstractDataStore<T>& m_Mask;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  ReconstructOp m_Op;
  bool m_FullyConnected;
  const std::atomic_bool& m_ShouldCancel;
  // Retained so both engines share the identical constructor signature DispatchAlgorithm requires. Progress is not
  // emitted at the engine level (consistent with the other ImageProcessing engines; the calling filter reports status).
  const IFilter::MessageHandler& m_MessageHandler;
  ReconstructionMarkerOptions m_MarkerOptions;
  usize m_FrontierCapacity = std::numeric_limits<usize>::max();
};

template <class T, bool UseTemporaryWork>
class ReconstructSweep;

template <class T>
class ReconstructSweepWithMarkerOptions
{
public:
  ReconstructSweepWithMarkerOptions(const AbstractDataStore<T>& markerStore, const AbstractDataStore<T>& maskStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, ReconstructOp op,
                                    bool fullyConnected, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, ReconstructionMarkerOptions markerOptions)
  : m_Marker(markerStore)
  , m_Mask(maskStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Op(op)
  , m_FullyConnected(fullyConnected)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_MarkerOptions(markerOptions)
  {
  }

  Result<> operator()()
  {
    const detail::ReconstructionWorkingMemoryFraction preferredFraction = detail::SelectReconstructionWorkingMemoryFraction(m_Dims, m_MarkerOptions.source);
    auto allocationResult = detail::ReserveReconstructionWorkingMemoryPlan<T>(m_Dims, preferredFraction.numerator, preferredFraction.denominator);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    const auto& plan = allocation.plan;
    CacheMemoryBudgetManager::WorkingMemoryReservation frontierReservation;
    usize frontierCapacity = 0;
    const bool useBorderFrontier = m_MarkerOptions.source == ReconstructionMarkerSource::MaskWithMinimumInterior || m_MarkerOptions.source == ReconstructionMarkerSource::MaskWithMaximumInterior;
    if(plan.useResidentFullSweep && useBorderFrontier)
    {
      auto frontierBytesResult = detail::CalculateReconstructionFrontierWorkingMemoryBytes(m_Dims);
      if(frontierBytesResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(frontierBytesResult));
      }
      const usize requestedFrontierBytes = frontierBytesResult.value();
      frontierReservation = ReserveWorkingMemory(requestedFrontierBytes, requestedFrontierBytes);
      if(frontierReservation.sizeBytes() == requestedFrontierBytes)
      {
        frontierCapacity = requestedFrontierBytes / sizeof(uint64);
      }
      else
      {
        frontierReservation = {};
      }
    }
    return ReconstructSweep<T, true>{m_Marker,
                                     m_Mask,
                                     m_Out,
                                     m_Dims,
                                     m_Op,
                                     m_FullyConnected,
                                     m_ShouldCancel,
                                     m_MessageHandler,
                                     plan.maxSlabValues,
                                     std::numeric_limits<usize>::max(),
                                     m_MarkerOptions,
                                     plan.fixedWorkValues,
                                     plan.fixedBufferValues,
                                     plan.useResidentFullSweep,
                                     frontierCapacity}();
  }

private:
  const AbstractDataStore<T>& m_Marker;
  const AbstractDataStore<T>& m_Mask;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  ReconstructOp m_Op;
  bool m_FullyConnected;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  ReconstructionMarkerOptions m_MarkerOptions;
};

/**
 * @brief Out-of-core morphological reconstruction (streamed): iterate {forward raster sweep, reverse anti-raster
 *        sweep} over the working store IN PLACE until a full pair of sweeps changes nothing. This is Vincent's
 *        SEQUENTIAL reconstruction iterated to stability; it converges to the identical unique fixpoint as the
 *        in-core hybrid, so its output is BYTE-IDENTICAL to @ref ReconstructVincent (the D3 cross-validation
 *        gate). Reconstruction is monotone (dilation only raises values toward the mask, erosion only lowers),
 *        so the iteration is guaranteed to terminate.
 *
 * Each sweep streams the volume in scan-order slabs. Voxel updates
 * within a slab remain strictly sequential in forward-raster or
 * reverse-anti-raster order. Same-plane neighbors see earlier
 * updates
 * in the slab. Cross-plane neighbors see either an updated plane in
 * the slab or a one-plane boundary buffer. Production dispatch converts a
 * shared total-byte reservation into checked per-array slab sizes. The direct
 * constructor retains a 16 MiB-per-array default as a low-level test seam.
 * At least one plane is always processed, so a single plane may exceed
 * that target. This preserves exact Gauss-Seidel semantics while
 * reducing backing-store calls from one
 * pair per plane to one pair per
 * slab.
 *
 * Worst-case sweep count is O(propagation distance). Memory remains
 * bounded, but pathological inputs may require many full-volume
 * passes. The
 * in-core Vincent implementation remains the fast resident
 * default. Identical required constructor arguments let both
 * implementations use DispatchAlgorithm; optional limits provide test seams.
 *
 * @pre scalar (1 component); marker/mask/out all hold dimX*dimY*dimZ values; the marker satisfies the direction
 *      precondition (dilation marker<=mask, erosion marker>=mask). @p outStore MUST be distinct from @p maskStore:
 *      later slabs re-read the mask after earlier output slabs have
 *      been
 * overwritten, so aliasing those stores would corrupt the
 *      constraint values. The output MAY alias @p markerStore because
 *      initialization reads each marker value before writing the
 * same
 *      position, and the marker is never read again after
 *      initialization. The facade nevertheless supplies a freshly
 *      created output distinct from both inputs.
 * @tparam T
 * scalar voxel type.
 */
template <class T, bool UseTemporaryWork = false>
class ReconstructSweep
{
public:
  static constexpr usize k_DefaultSlabValues = std::max<usize>(1, (16ULL * 1024ULL * 1024ULL) / sizeof(T));
  static constexpr usize k_DefaultFixed2DWorkValues = std::max<usize>(1, (48ULL * 1024ULL * 1024ULL) / sizeof(T));
  static constexpr usize k_DefaultFixed2DBufferValues = std::max<usize>(1, (8ULL * 1024ULL * 1024ULL) / sizeof(T));

  ReconstructSweep(const AbstractDataStore<T>& markerStore, const AbstractDataStore<T>& maskStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, ReconstructOp op, bool fullyConnected,
                   const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler, usize maxSlabValues = k_DefaultSlabValues,
                   usize maxSweepPairs = std::numeric_limits<usize>::max(), ReconstructionMarkerOptions markerOptions = {}, usize fixed2DWorkValues = k_DefaultFixed2DWorkValues,
                   usize fixed2DBufferValues = k_DefaultFixed2DBufferValues, bool useResidentFullSweep = false, usize residentFrontierValues = 0)
  : m_Marker(markerStore)
  , m_Mask(maskStore)
  , m_Out(outStore)
  , m_Dims(dims)
  , m_Op(op)
  , m_FullyConnected(fullyConnected)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  , m_MaxSlabValues(std::max<usize>(1, maxSlabValues))
  , m_MaxSweepPairs(maxSweepPairs)
  , m_MarkerOptions(markerOptions)
  , m_Fixed2DWorkValues(fixed2DWorkValues)
  , m_Fixed2DBufferValues(fixed2DBufferValues)
  , m_UseResidentFullSweep(useResidentFullSweep)
  , m_ResidentFrontierValues(residentFrontierValues)
  {
  }
  ~ReconstructSweep() = default;
  ReconstructSweep(const ReconstructSweep&) = delete;
  ReconstructSweep(ReconstructSweep&&) noexcept = delete;
  ReconstructSweep& operator=(const ReconstructSweep&) = delete;
  ReconstructSweep& operator=(ReconstructSweep&&) noexcept = delete;

  Result<> operator()()
  {
    return (m_Op == ReconstructOp::Dilation) ? runImpl<true>() : runImpl<false>();
  }

private:
  /// @brief Runs one direction (@c Dilation) of the out-of-core sweep, routing to the fully resident, serial
  ///        2-D/3-D sweeps or to the wavefront-parallel streamed sweep depending on how much working memory is available;
  ///        see the class documentation above for the full routing description.
  template <bool Dilation>
  Result<> runImpl();

  /// @brief The wavefront-parallel forward/reverse sweep pair over a resident prefix of planes held fully in memory
  ///        (residentWork) plus a streamed tail of planes read and written one at a time, used when the marker is border-derived
  ///        (MaskWithMinimumInterior / MaskWithMaximumInterior) and enough working memory is available to keep a prefix resident.
  template <bool Dilation, class WorkStore, class MaskStore>
  Result<> reconstructPersistentPrefix(WorkStore& workStore, const MaskStore& maskStore, usize dimX, usize dimY, usize dimZ, usize sliceValues, usize vol,
                                       const detail::ReconstructionPersistentPrefixPlan& plan, bool initializeWork);

  /// @brief The serial Vincent-style FIFO frontier flood over a fully resident work/mask pair, used by runResident3D
  ///        as its fast path. Returns false if the bounded frontier queue overflows, signaling the caller to fall back to the
  ///        plain resident sweep pair.
  template <bool Dilation>
  bool runResidentFrontier(T* work, const T* mask, usize dimX, usize dimY, usize dimZ, usize sliceValues, detail::BoundedReconstructionQueue<uint64>& frontier);

  /// @brief The serial, fully resident forward/reverse sweep pair used when the whole volume fits in the active
  ///        working-memory reservation (UseTemporaryWork == true). Tries runResidentFrontier first when a bounded frontier
  ///        queue was reserved, falling back to the plain sweep pair if the queue overflows or allocation fails.
  template <bool Dilation>
  Result<> runResident3D(usize dimX, usize dimY, usize dimZ, usize sliceValues, usize vol);

  /// @brief Wavefront-parallel counterpart of runResident3D: identical setup and identical runResidentFrontier
  ///        fast-path call, but the resident forward/reverse sweep pair's per-plane row/column double loop runs
  ///        through detail::RunPlaneWavefront's block schedule (defined in MorphologicalReconstructionWavefront.hpp,
  ///        included by the streamed translation unit that defines this member) instead of a single-threaded
  ///        nested loop. Used for BOTH connectivities here -- unlike ReconstructVincent's one-pass phases, which
  ///        keep the serial raster for full connectivity -- because this sweep always loops forward/reverse pairs
  ///        to convergence: RunPlaneWavefront's documentation (its mode (b)) proves a stale in-plane read, under
  ///        either connectivity, can only delay a value's propagation within one pass and can never move the
  ///        volume away from the fixed point the same forward/reverse loop would reach serially, and a pass that
  ///        makes no change is itself proof nothing was still stale.
  template <bool Dilation>
  Result<> runResident3DWavefront(usize dimX, usize dimY, usize dimZ, usize sliceValues, usize vol);

  template <class SourceStore, class DestinationStore>
  Result<> copyStore(const SourceStore& sourceStore, DestinationStore& destinationStore, usize valueCount, usize maxBatchValues)
  {
    auto buffer = std::make_unique<T[]>(maxBatchValues);
    for(usize start = 0; start < valueCount; start += maxBatchValues)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize count = std::min(maxBatchValues, valueCount - start);
      if(Result<> r = sourceStore.copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
      {
        return r;
      }
      if(Result<> r = destinationStore.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }

  template <class SourceStore, class DestinationStore>
  Result<> copyStoreRange(const SourceStore& sourceStore, DestinationStore& destinationStore, usize startIndex, usize valueCount, usize maxBatchValues)
  {
    if(valueCount == 0)
    {
      return {};
    }
    const usize batchCapacity = std::min(valueCount, maxBatchValues);
    auto buffer = std::make_unique<T[]>(batchCapacity);
    for(usize copiedValues = 0; copiedValues < valueCount; copiedValues += batchCapacity)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize count = std::min(batchCapacity, valueCount - copiedValues);
      const usize currentStart = startIndex + copiedValues;
      if(Result<> result = sourceStore.copyIntoBuffer(currentStart, nonstd::span<T>(buffer.get(), count)); result.invalid())
      {
        return result;
      }
      if(Result<> result = destinationStore.copyFromBuffer(currentStart, nonstd::span<const T>(buffer.get(), count)); result.invalid())
      {
        return result;
      }
    }
    return {};
  }

  Result<> loadMarker(usize start, nonstd::span<T> values) const
  {
    const AbstractDataStore<T>& sourceStore = m_MarkerOptions.source == ReconstructionMarkerSource::Provided ? m_Marker : m_Mask;
    if(Result<> result = sourceStore.copyIntoBuffer(start, values); result.invalid())
    {
      return result;
    }
    if(m_MarkerOptions.source == ReconstructionMarkerSource::Provided)
    {
      return {};
    }
    detail::ApplyReconstructionMarkerOptions<T>(values, start, m_Dims, m_MarkerOptions);
    return {};
  }

  /// @brief The serial 2-D (single-plane, dimZ == 1) sweep pair, specialized for either the full-width row-block
  ///        layout or the tiled-column layout chosen by detail::CreateSweep2DPlan.
  template <bool Dilation, class WorkStore, class MaskStore>
  Result<> run2D(WorkStore& workStore, const MaskStore& maskStore, usize dimX, usize dimY, usize vol, usize maxBufferValues);

  /// @brief The serial, in-place forward/reverse sweep pair over the output store itself (UseTemporaryWork == false): no scratch
  ///        working store is allocated, so the marker is loaded directly into @c m_Out and every sweep reads and writes @c m_Out in place.
  template <bool Dilation>
  Result<> runDirect(usize dimX, usize dimY, usize dimZ, usize sliceValues, usize vol);

  /// @brief Wavefront-parallel counterpart of runDirect: identical slab tiling, in-place store I/O, and
  ///        boundary-plane carry (@c adjacent), but each plane's row/column double loop runs through
  ///        detail::RunPlaneWavefront's block schedule instead of a single-threaded nested loop. Used for BOTH
  ///        connectivities for the same convergence-loop reason documented on runResident3DWavefront above (this
  ///        sweep also always loops forward/reverse pairs to a fixed point).
  template <bool Dilation>
  Result<> runDirectWavefront(usize dimX, usize dimY, usize dimZ, usize sliceValues, usize vol);

  const AbstractDataStore<T>& m_Marker;
  const AbstractDataStore<T>& m_Mask;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  ReconstructOp m_Op;
  bool m_FullyConnected;
  const std::atomic_bool& m_ShouldCancel;
  // Retained so both engines share the identical constructor signature DispatchAlgorithm requires. Progress is not
  // emitted at the engine level (consistent with the other ImageProcessing engines; the calling filter reports status).
  const IFilter::MessageHandler& m_MessageHandler;
  const usize m_MaxSlabValues;
  const usize m_MaxSweepPairs;
  ReconstructionMarkerOptions m_MarkerOptions;
  const usize m_Fixed2DWorkValues;
  const usize m_Fixed2DBufferValues;
  const bool m_UseResidentFullSweep;
  const usize m_ResidentFrontierValues;
};

template <class T, bool UseTemporaryWork = false>
class ReconstructSweepWithWorkingMemory
{
public:
  ReconstructSweepWithWorkingMemory(const AbstractDataStore<T>& markerStore, const AbstractDataStore<T>& maskStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, ReconstructOp op,
                                    bool fullyConnected, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  : m_Marker(markerStore)
  , m_Mask(maskStore)
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
    auto allocationResult = detail::ReserveReconstructionWorkingMemoryPlan<T>(m_Dims);
    if(allocationResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(allocationResult));
    }
    auto allocation = std::move(allocationResult.value());
    const auto& plan = allocation.plan;
    return ReconstructSweep<T, UseTemporaryWork>{m_Marker,
                                                 m_Mask,
                                                 m_Out,
                                                 m_Dims,
                                                 m_Op,
                                                 m_FullyConnected,
                                                 m_ShouldCancel,
                                                 m_MessageHandler,
                                                 plan.maxSlabValues,
                                                 std::numeric_limits<usize>::max(),
                                                 {},
                                                 plan.fixedWorkValues,
                                                 plan.fixedBufferValues,
                                                 plan.useResidentFullSweep,
                                                 0}();
  }

private:
  const AbstractDataStore<T>& m_Marker;
  const AbstractDataStore<T>& m_Mask;
  AbstractDataStore<T>& m_Out;
  SizeVec3 m_Dims;
  ReconstructOp m_Op;
  bool m_FullyConnected;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

// -----------------------------------------------------------------------------
// Declare our extern templates. ReconstructVincent<T>::operator() and ReconstructSweep<T, UseTemporaryWork>'s
// non-template members are defined out-of-line in MorphologicalReconstructionVincent.cpp and
// MorphologicalReconstructionSweepStreamed.cpp respectively; these declarations tell every other translation unit
// that includes this header not to implicitly instantiate those specializations locally, matching the explicit
// instantiation definitions provided by the two .cpp files.
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<int8>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<uint8>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<int16>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<uint16>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<int32>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<uint32>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<int64>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<uint64>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<float32>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructVincent<float64>;

extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int8, false>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int8, true>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint8, false>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint8, true>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int16, false>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int16, true>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint16, false>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint16, true>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int32, false>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int32, true>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint32, false>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint32, true>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int64, false>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<int64, true>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint64, false>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<uint64, true>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<float32, false>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<float32, true>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<float64, false>;
extern template class SIMPLNX_TEMPLATE_EXPORT ReconstructSweep<float64, true>;

/**
 * @brief Public entry: morphological reconstruction of @p markerStore under @p maskStore (dilation or erosion),
 *        written to @p outStore. Routes the in-core Vincent hybrid vs the OOC streamed sweep via DispatchAlgorithm
 *        (both produce the identical unique fixpoint). @p op selects direction; @p fullyConnected the connectivity.
 *
 * The marker is typically a scratch store (no IDataArray), so OOC detection uses the real mask(input) + out
 * arrays -- exactly how the morphology composites detect OOC for their scratch. @pre marker satisfies the
 * direction precondition. @tparam T scalar type.
 */
template <class T>
Result<> ApplyMorphologicalReconstruction(const AbstractDataStore<T>& markerStore, const AbstractDataStore<T>& maskStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, ReconstructOp op,
                                          bool fullyConnected, const IDataArray& maskArray, const IDataArray& outArray, const std::atomic_bool& shouldCancel,
                                          const IFilter::MessageHandler& messageHandler, bool useTemporaryWork = false, ReconstructionMarkerOptions markerOptions = {})
{
  if(markerOptions.source != ReconstructionMarkerSource::Provided)
  {
    return DispatchAlgorithm<ReconstructVincent<T>, ReconstructSweepWithMarkerOptions<T>>({&maskArray, &outArray}, markerStore, maskStore, outStore, dims, op, fullyConnected, shouldCancel,
                                                                                          messageHandler, markerOptions);
  }
  if(useTemporaryWork)
  {
    return DispatchAlgorithm<ReconstructVincent<T>, ReconstructSweepWithWorkingMemory<T, true>>({&maskArray, &outArray}, markerStore, maskStore, outStore, dims, op, fullyConnected, shouldCancel,
                                                                                                messageHandler);
  }
  if(dims[2] == 1)
  {
    return DispatchAlgorithm<ReconstructVincent<T>, ReconstructSweepWithWorkingMemory<T, true>>({&maskArray, &outArray}, markerStore, maskStore, outStore, dims, op, fullyConnected, shouldCancel,
                                                                                                messageHandler);
  }
  return DispatchAlgorithm<ReconstructVincent<T>, ReconstructSweepWithWorkingMemory<T>>({&maskArray, &outArray}, markerStore, maskStore, outStore, dims, op, fullyConnected, shouldCancel,
                                                                                        messageHandler);
}
} // namespace nx::core::ImageProcessing
