#include "ComputeBoundingBoxStatsDirect.hpp"

#include "ComputeBoundingBoxStats.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_MinXIndex = 0;
constexpr usize k_MinYIndex = 1;
constexpr usize k_MinZIndex = 2;
constexpr usize k_MaxXIndex = 3;
constexpr usize k_MaxYIndex = 4;
constexpr usize k_MaxZIndex = 5;
constexpr usize k_FrequencyTableCapacity = 32;

/* clang-format off */
template <class T>
concept ArithmeticNotBool = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;
/* clang-format on */

/**
 * @brief Converts one physical bounding box to half-open voxel indices.
 * @param unifiedBounds Contains six values for each bounding box.
 * @param targetBoundsIndex Selects the bounding box.
 * @param image Supplies the origin, spacing, and dimensions.
 * @return Clipped minimum and maximum voxel indices in X-Y-Z order.
 * @pre Preflight has verified nonzero image spacing.
 */
std::array<usize, 6> GetVoxelIndices(nonstd::span<const float32> unifiedBounds, usize targetBoundsIndex, const ImageGeom& image)
{
  std::array<usize, 6> voxelIndices = {};

  // Preflight verifies nonzero spacing, so this function does not repeat that check.
  FloatVec3 spacing = image.getSpacing();
  FloatVec3 origin = image.getOrigin();
  SizeVec3 dims = image.getDimensions();

  // The lower bound from input is expected to be the lower corner of the voxel
  for(usize i = 0; i < 3; i++)
  {
    float32 minVoxel = unifiedBounds[(targetBoundsIndex * 6) + i];
    float32 maxDim = (spacing[i] * static_cast<float32>(dims[i])) + origin[i];
    if(minVoxel < origin[i])
    {
      voxelIndices[i] = 0;
    }
    else if(minVoxel > maxDim)
    {
      voxelIndices[i] = std::floor((maxDim - origin[i]) / spacing[i]);
    }
    else
    {
      voxelIndices[i] = std::floor((minVoxel - origin[i]) / spacing[i]);
    }
  }

  // The upper bound from input is expected to be the upper corner of the voxel
  for(usize i = 0; i < 3; i++)
  {
    usize offset = i + 3;
    float32 maxVoxel = unifiedBounds[(targetBoundsIndex * 6) + offset];
    float32 maxDim = (spacing[i] * static_cast<float32>(dims[i])) + origin[i];
    if(maxVoxel < origin[i])
    {
      voxelIndices[offset] = 0;
    }
    else if(maxVoxel > maxDim)
    {
      voxelIndices[offset] = std::floor((maxDim - origin[i]) / spacing[i]);
    }
    else
    {
      voxelIndices[offset] = std::floor((maxVoxel - origin[i]) / spacing[i]);
    }
  }

  return voxelIndices;
}

/**
 * @class ContiguousInputAccessor
 * @brief Reads immutable in-memory data directly during parallel execution.
 * @tparam T Input value type.
 *
 * Each worker reads shared contiguous storage and writes only its bound-local
 * cache entry.
 */
template <typename T>
class ContiguousInputAccessor
{
public:
  explicit ContiguousInputAccessor(const T* inputValues)
  : m_InputValues(inputValues)
  {
  }

  T value(usize index) const
  {
    return m_InputValues[index];
  }

private:
  const T* m_InputValues = nullptr;
};

/**
 * @class AbstractInputAccessor
 * @brief Reads values through the generic DataStore interface.
 * @tparam T Input value type.
 *
 * Callers use this accessor serially because generic DataStore access does not
 * guarantee thread safety.
 */
template <typename T>
class AbstractInputAccessor
{
public:
  explicit AbstractInputAccessor(const AbstractDataStore<T>& inputStore)
  : m_InputStore(inputStore)
  {
  }

  T value(usize index) const
  {
    return m_InputStore.getValue(index);
  }

private:
  const AbstractDataStore<T>& m_InputStore;
};

/**
 * @struct StatsCache
 * @brief Stores statistics that one input pass can collect.
 * @tparam T Input value type.
 *
 * Mode storage exists only when mode is requested. Standard deviation uses a
 * second pass, so this cache does not reserve storage for either result.
 */
template <typename T>
struct StatsCache
{
  using value_type = T;
  T minValue = std::numeric_limits<T>::quiet_NaN();
  T maxValue = std::numeric_limits<T>::quiet_NaN();
  usize count = 0;
  T summationValue = static_cast<T>(0);
};

/**
 * @struct CompleteStatsCache
 * @brief Extends the base cache with frequency-derived statistics.
 * @tparam T Input value type.
 */
template <typename T>
struct CompleteStatsCache : StatsCache<T>
{
  float32 medianValue = std::numeric_limits<float32>::quiet_NaN();
  usize uniqueValCount = 0;
};

/**
 * @brief Compares values with the ordering used by the frequency algorithms.
 * @tparam T Input value type.
 * @param lhs First value.
 * @param rhs Second value.
 * @return True when the values compare equal or neither value orders first.
 * @note Unordered floating-point pairs compare as equivalent. This preserves
 *       the legacy frequency behavior.
 */
template <typename T>
bool Equivalent(const T& lhs, const T& rhs)
{
  if(lhs == rhs)
  {
    return true;
  }
  if constexpr(std::is_floating_point_v<T>)
  {
    return !(lhs < rhs) && !(rhs < lhs);
  }
  return false;
}

/**
 * @struct FrequencyEntry
 * @brief Stores one candidate value and its observed frequency.
 * @tparam T Input value type.
 */
template <typename T>
struct FrequencyEntry
{
  T value = {};
  uint64 count = 0;
};

/**
 * @class FixedFrequencyTable
 * @brief Fixed-capacity frequency table for the common low-cardinality case.
 * @tparam T Input value type.
 *
 * A flat stack table avoids a tree lookup and allocation for each distinct
 * value. Fixed capacity keeps frequency scratch independent of the cell count.
 * Higher-cardinality boxes use repeated bounded scans.
 */
template <typename T>
class FixedFrequencyTable
{
public:
  bool add(const T& value)
  {
    if(incrementIfPresent(value))
    {
      return true;
    }
    if(m_Size == k_FrequencyTableCapacity)
    {
      return false;
    }

    m_Entries[m_Size] = {value, 1};
    m_LastIndex = m_Size;
    m_Size++;
    return true;
  }

  bool incrementIfPresent(const T& value)
  {
    if(m_Size == 0)
    {
      return false;
    }
    if(Equivalent(m_Entries[m_LastIndex].value, value))
    {
      m_Entries[m_LastIndex].count++;
      return true;
    }

    for(usize index = 0; index < m_Size; index++)
    {
      if(index != m_LastIndex && Equivalent(m_Entries[index].value, value))
      {
        m_Entries[index].count++;
        m_LastIndex = index;
        return true;
      }
    }
    return false;
  }

  void insertCandidate(const T& value)
  {
    for(usize index = 0; index < m_Size; index++)
    {
      if(Equivalent(m_Entries[index].value, value))
      {
        return;
      }
    }

    if(m_Size < k_FrequencyTableCapacity)
    {
      m_Entries[m_Size++] = {value, 0};
      return;
    }

    auto largestIter = std::max_element(m_Entries.begin(), m_Entries.begin() + m_Size, [](const auto& lhs, const auto& rhs) { return lhs.value < rhs.value; });
    if(value < largestIter->value)
    {
      *largestIter = {value, 0};
    }
  }

  void sort()
  {
    std::sort(m_Entries.begin(), m_Entries.begin() + m_Size, [](const auto& lhs, const auto& rhs) { return lhs.value < rhs.value; });
    m_LastIndex = 0;
  }

  usize size() const
  {
    return m_Size;
  }

  const FrequencyEntry<T>& operator[](usize index) const
  {
    return m_Entries[index];
  }

private:
  std::array<FrequencyEntry<T>, k_FrequencyTableCapacity> m_Entries = {};
  usize m_Size = 0;
  usize m_LastIndex = 0;
};

/**
 * @brief Applies a function to each value in one clipped bounding box.
 * @tparam InputAccessorT Input accessor type.
 * @tparam FunctionT Callback type.
 * @param imageGeom Supplies the image dimensions.
 * @param inputValues Reads input values.
 * @param voxelIndices Contains half-open voxel bounds.
 * @param function Receives each value in Z-Y-X order.
 */
template <class InputAccessorT, class FunctionT>
void ForEachBoxValue(const ImageGeom& imageGeom, const InputAccessorT& inputValues, const std::array<usize, 6>& voxelIndices, FunctionT&& function)
{
  const usize xPoints = imageGeom.getNumXCells();
  const usize yPoints = imageGeom.getNumYCells();
  for(usize zIndex = voxelIndices[k_MinZIndex]; zIndex < voxelIndices[k_MaxZIndex]; zIndex++)
  {
    const usize zStride = zIndex * xPoints * yPoints;
    for(usize yIndex = voxelIndices[k_MinYIndex]; yIndex < voxelIndices[k_MaxYIndex]; yIndex++)
    {
      const usize yStride = yIndex * xPoints;
      for(usize xIndex = voxelIndices[k_MinXIndex]; xIndex < voxelIndices[k_MaxXIndex]; xIndex++)
      {
        function(inputValues.value(zStride + yStride + xIndex));
      }
    }
  }
}

/**
 * @struct FrequencySummaryState
 * @brief Tracks ordered frequency statistics across bounded batches.
 * @tparam T Input value type.
 */
template <typename T>
struct FrequencySummaryState
{
  usize cumulativeFrequency = 0;
  std::optional<T> previousValue;
  uint64 maxFrequency = 0;
  bool medianFound = false;
};

/**
 * @brief Adds one ordered frequency batch to the summary state.
 * @tparam T Input value type.
 * @param frequencies Supplies one sorted batch of distinct values.
 * @param stats Receives median and unique-value statistics.
 * @param state Preserves cumulative state between batches.
 * @param modes Receives tied modes, or is null when mode is not requested.
 */
template <typename T>
void AccumulateFrequencyBatch(const FixedFrequencyTable<T>& frequencies, CompleteStatsCache<T>& stats, FrequencySummaryState<T>& state, std::vector<T>* modes)
{
  const usize medianPosition = (stats.count / 2) + 1;
  for(usize index = 0; index < frequencies.size(); index++)
  {
    const auto& entry = frequencies[index];
    stats.uniqueValCount++;
    state.cumulativeFrequency += entry.count;
    if(!state.medianFound && state.cumulativeFrequency >= medianPosition)
    {
      if(stats.count % 2 == 0 && state.cumulativeFrequency == medianPosition && state.previousValue.has_value())
      {
        stats.medianValue = (static_cast<float32>(state.previousValue.value()) + static_cast<float32>(entry.value)) / 2.0f;
      }
      else
      {
        stats.medianValue = static_cast<float32>(entry.value);
      }
      state.medianFound = true;
    }

    if(modes != nullptr)
    {
      if(entry.count > state.maxFrequency)
      {
        state.maxFrequency = entry.count;
        modes->clear();
        modes->push_back(entry.value);
      }
      else if(entry.count == state.maxFrequency)
      {
        modes->push_back(entry.value);
      }
    }
    state.previousValue = entry.value;
  }
}

/**
 * @brief Applies the legacy modal-frequency width rule.
 * @tparam T Input value type.
 * @param state Supplies the maximum frequency.
 * @param modes Supplies the candidate modes, or is null.
 *
 * The legacy output uses an int frequency. Modes are cleared when the maximum
 * frequency cannot be represented by that type.
 */
template <typename T>
void FinalizeMode(const FrequencySummaryState<T>& state, std::vector<T>* modes)
{
  if(modes == nullptr)
  {
    return;
  }

  // Preserve the legacy narrowing before comparing modal frequencies.
  const int modalCount = state.maxFrequency;
  if(static_cast<uint64>(modalCount) != state.maxFrequency)
  {
    modes->clear();
  }
}

/**
 * @brief Derives ordered frequency statistics from one complete table.
 * @tparam T Input value type.
 * @param frequencies Supplies distinct values and counts.
 * @param stats Receives median and unique-value statistics.
 * @param modes Receives tied modes, or is null when mode is not requested.
 */
template <typename T>
void CalculateFrequencyStats(FixedFrequencyTable<T>& frequencies, CompleteStatsCache<T>& stats, std::vector<T>* modes)
{
  frequencies.sort();
  FrequencySummaryState<T> state;
  AccumulateFrequencyBatch(frequencies, stats, state, modes);
  FinalizeMode(state, modes);
}

/**
 * @brief Uses bounded frequency scratch when the flat table overflows.
 * @tparam T Input value type.
 * @tparam InputAccessorT Input accessor type.
 * @param imageGeom Supplies the image dimensions.
 * @param inputValues Reads input values.
 * @param voxelIndices Contains half-open voxel bounds.
 * @param stats Receives median and unique-value statistics.
 * @param modes Receives sorted tied modes, or is null when mode is not needed.
 *
 * The function selects and counts distinct values in ordered fixed-size
 * batches. It trades additional direct scans for constant frequency scratch.
 */
template <typename T, class InputAccessorT>
void CalculateFrequencyStatsBounded(const ImageGeom& imageGeom, const InputAccessorT& inputValues, const std::array<usize, 6>& voxelIndices, CompleteStatsCache<T>& stats, std::vector<T>* modes)
{
  FrequencySummaryState<T> state;
  std::optional<T> lowerExclusive;
  while(true)
  {
    FixedFrequencyTable<T> frequencies;
    ForEachBoxValue(imageGeom, inputValues, voxelIndices, [&](const T& value) {
      if(!lowerExclusive.has_value() || lowerExclusive.value() < value)
      {
        frequencies.insertCandidate(value);
      }
    });
    if(frequencies.size() == 0)
    {
      break;
    }

    frequencies.sort();
    ForEachBoxValue(imageGeom, inputValues, voxelIndices, [&](const T& value) { frequencies.incrementIfPresent(value); });
    AccumulateFrequencyBatch(frequencies, stats, state, modes);
    lowerExclusive = frequencies[frequencies.size() - 1].value;
  }
  FinalizeMode(state, modes);
}

/**
 * @class ComputeBaseStatsImpl
 * @brief Computes count, extrema, and sum for independent bounding boxes.
 * @tparam T Input value type.
 * @tparam InputAccessorT Input accessor type.
 * @pre The cache vector has one entry for each unified-bounds tuple.
 *
 * Parallel workers write disjoint cache entries. The input accessor determines
 * whether parallel execution is permitted.
 */
template <typename T, class InputAccessorT>
class ComputeBaseStatsImpl
{
public:
  ComputeBaseStatsImpl(const ImageGeom& geom, InputAccessorT inputValues, nonstd::span<const float32> unifiedBounds, std::vector<StatsCache<T>>& statsVector)
  : m_Geom(geom)
  , m_InputValues(inputValues)
  , m_UnifiedBounds(unifiedBounds)
  , m_StatsVector(statsVector)
  {
  }
  ~ComputeBaseStatsImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    usize xPoints = m_Geom.getNumXCells();
    usize yPoints = m_Geom.getNumYCells();

    for(usize targetBoundsIndex = start; targetBoundsIndex < end; targetBoundsIndex++)
    {
      std::array<usize, 6> voxelIndices = GetVoxelIndices(m_UnifiedBounds, targetBoundsIndex, m_Geom);

      // Local primitives reduce writes to the shared cache vector.
      usize count = 0;
      T minValue = std::numeric_limits<T>::max();
      T maxValue = std::numeric_limits<T>::lowest();
      T summationValue = static_cast<T>(0);

      usize zStride = 0, yStride = 0;
      for(usize zIndex = voxelIndices[k_MinZIndex]; zIndex < voxelIndices[k_MaxZIndex]; zIndex++)
      {
        zStride = zIndex * xPoints * yPoints;
        for(usize yIndex = voxelIndices[k_MinYIndex]; yIndex < voxelIndices[k_MaxYIndex]; yIndex++)
        {
          yStride = yIndex * xPoints;
          for(usize xIndex = voxelIndices[k_MinXIndex]; xIndex < voxelIndices[k_MaxXIndex]; xIndex++)
          {
            usize tup = zStride + yStride + xIndex;
            T value = m_InputValues.value(tup);
            count++;
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
            summationValue += value;
          }
        }
      }

      if(count == 0)
      {
        minValue = std::numeric_limits<T>::quiet_NaN();
        maxValue = std::numeric_limits<T>::quiet_NaN();
      }

      // Publish one complete cache entry after the bound traversal.
      m_StatsVector[targetBoundsIndex].count = count;
      m_StatsVector[targetBoundsIndex].minValue = minValue;
      m_StatsVector[targetBoundsIndex].maxValue = maxValue;
      m_StatsVector[targetBoundsIndex].summationValue = summationValue;
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const ImageGeom& m_Geom;
  InputAccessorT m_InputValues;
  nonstd::span<const float32> m_UnifiedBounds;
  std::vector<StatsCache<T>>& m_StatsVector;
};

/**
 * @class ComputeAllStatsImpl
 * @brief Computes base, frequency, and mode statistics for each bounding box.
 * @tparam T Input value type. Boolean input is not supported.
 * @tparam InputAccessorT Input accessor type.
 * @tparam CollectBaseStatsV Enables extrema and sum collection.
 * @pre Cache and mode vectors have one entry for each bounds tuple.
 *
 * Parallel workers write disjoint cache and mode-vector entries. A box with
 * more than 32 distinct values uses repeated scans with fixed frequency scratch.
 * The staged mode output can grow with the number of tied modes.
 */
template <typename T, class InputAccessorT, bool CollectBaseStatsV>
class ComputeAllStatsImpl
{
public:
  ComputeAllStatsImpl(const ImageGeom& geom, InputAccessorT inputValues, nonstd::span<const float32> unifiedBounds, std::vector<CompleteStatsCache<T>>& statsVector,
                      std::vector<std::shared_ptr<std::vector<T>>>& modes)
  : m_Geom(geom)
  , m_InputValues(inputValues)
  , m_UnifiedBounds(unifiedBounds)
  , m_StatsVector(statsVector)
  , m_Modes(modes)
  {
  }
  ~ComputeAllStatsImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    usize xPoints = m_Geom.getNumXCells();
    usize yPoints = m_Geom.getNumYCells();

    for(usize targetBoundsIndex = start; targetBoundsIndex < end; targetBoundsIndex++)
    {
      std::array<usize, 6> voxelIndices = GetVoxelIndices(m_UnifiedBounds, targetBoundsIndex, m_Geom);

      // Local primitives reduce writes to the shared cache vector.
      usize count = 0;
      [[maybe_unused]] T minValue = std::numeric_limits<T>::max();
      [[maybe_unused]] T maxValue = std::numeric_limits<T>::lowest();
      [[maybe_unused]] T summationValue = static_cast<T>(0);

      FixedFrequencyTable<T> frequencies;
      bool frequencyOverflow = false;

      usize zStride = 0, yStride = 0;
      for(usize zIndex = voxelIndices[k_MinZIndex]; zIndex < voxelIndices[k_MaxZIndex]; zIndex++)
      {
        zStride = zIndex * xPoints * yPoints;
        for(usize yIndex = voxelIndices[k_MinYIndex]; yIndex < voxelIndices[k_MaxYIndex]; yIndex++)
        {
          yStride = yIndex * xPoints;
          for(usize xIndex = voxelIndices[k_MinXIndex]; xIndex < voxelIndices[k_MaxXIndex]; xIndex++)
          {
            usize tup = zStride + yStride + xIndex;
            T value = m_InputValues.value(tup);
            count++;
            if constexpr(CollectBaseStatsV)
            {
              minValue = std::min(minValue, value);
              maxValue = std::max(maxValue, value);
              summationValue += value;
            }

            if(!frequencyOverflow && !frequencies.add(value))
            {
              frequencyOverflow = true;
            }
          }
        }
      }

      if constexpr(CollectBaseStatsV)
      {
        if(count == 0)
        {
          minValue = std::numeric_limits<T>::quiet_NaN();
          maxValue = std::numeric_limits<T>::quiet_NaN();
        }

        m_StatsVector[targetBoundsIndex].minValue = minValue;
        m_StatsVector[targetBoundsIndex].maxValue = maxValue;
        m_StatsVector[targetBoundsIndex].summationValue = summationValue;
      }

      m_StatsVector[targetBoundsIndex].count = count;

      if(count == 0)
      {
        continue;
      }

      if(frequencyOverflow)
      {
        CalculateFrequencyStatsBounded<T>(m_Geom, m_InputValues, voxelIndices, m_StatsVector[targetBoundsIndex], m_Modes[targetBoundsIndex].get());
      }
      else
      {
        CalculateFrequencyStats<T>(frequencies, m_StatsVector[targetBoundsIndex], m_Modes[targetBoundsIndex].get());
      }
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const ImageGeom& m_Geom;
  InputAccessorT m_InputValues;
  nonstd::span<const float32> m_UnifiedBounds;
  std::vector<CompleteStatsCache<T>>& m_StatsVector;
  std::vector<std::shared_ptr<std::vector<T>>>& m_Modes;
};

/**
 * @class ComputeBasicAndFrequencyStatsImpl
 * @brief Computes base and frequency statistics without mode output.
 * @tparam T Input value type. Boolean input is not supported.
 * @tparam InputAccessorT Input accessor type.
 * @tparam CollectBaseStatsV Enables extrema and sum collection.
 * @pre The cache vector has one entry for each bounds tuple.
 */
template <typename T, class InputAccessorT, bool CollectBaseStatsV>
class ComputeBasicAndFrequencyStatsImpl
{
public:
  ComputeBasicAndFrequencyStatsImpl(const ImageGeom& geom, InputAccessorT inputValues, nonstd::span<const float32> unifiedBounds, std::vector<CompleteStatsCache<T>>& statsVector)
  : m_Geom(geom)
  , m_InputValues(inputValues)
  , m_UnifiedBounds(unifiedBounds)
  , m_StatsVector(statsVector)
  {
  }
  ~ComputeBasicAndFrequencyStatsImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    usize xPoints = m_Geom.getNumXCells();
    usize yPoints = m_Geom.getNumYCells();

    for(usize targetBoundsIndex = start; targetBoundsIndex < end; targetBoundsIndex++)
    {
      std::array<usize, 6> voxelIndices = GetVoxelIndices(m_UnifiedBounds, targetBoundsIndex, m_Geom);

      // Local primitives reduce writes to the shared cache vector.
      usize count = 0;
      [[maybe_unused]] T minValue = std::numeric_limits<T>::max();
      [[maybe_unused]] T maxValue = std::numeric_limits<T>::lowest();
      [[maybe_unused]] T summationValue = static_cast<T>(0);

      FixedFrequencyTable<T> frequencies;
      bool frequencyOverflow = false;

      usize zStride = 0, yStride = 0;
      for(usize zIndex = voxelIndices[k_MinZIndex]; zIndex < voxelIndices[k_MaxZIndex]; zIndex++)
      {
        zStride = zIndex * xPoints * yPoints;
        for(usize yIndex = voxelIndices[k_MinYIndex]; yIndex < voxelIndices[k_MaxYIndex]; yIndex++)
        {
          yStride = yIndex * xPoints;
          for(usize xIndex = voxelIndices[k_MinXIndex]; xIndex < voxelIndices[k_MaxXIndex]; xIndex++)
          {
            usize tup = zStride + yStride + xIndex;
            T value = m_InputValues.value(tup);
            count++;
            if constexpr(CollectBaseStatsV)
            {
              minValue = std::min(minValue, value);
              maxValue = std::max(maxValue, value);
              summationValue += value;
            }
            if(!frequencyOverflow && !frequencies.add(value))
            {
              frequencyOverflow = true;
            }
          }
        }
      }

      if constexpr(CollectBaseStatsV)
      {
        if(count == 0)
        {
          minValue = std::numeric_limits<T>::quiet_NaN();
          maxValue = std::numeric_limits<T>::quiet_NaN();
        }

        m_StatsVector[targetBoundsIndex].minValue = minValue;
        m_StatsVector[targetBoundsIndex].maxValue = maxValue;
        m_StatsVector[targetBoundsIndex].summationValue = summationValue;
      }

      m_StatsVector[targetBoundsIndex].count = count;

      if(count == 0)
      {
        continue;
      }

      if(frequencyOverflow)
      {
        CalculateFrequencyStatsBounded<T>(m_Geom, m_InputValues, voxelIndices, m_StatsVector[targetBoundsIndex], nullptr);
      }
      else
      {
        CalculateFrequencyStats<T>(frequencies, m_StatsVector[targetBoundsIndex], nullptr);
      }
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const ImageGeom& m_Geom;
  InputAccessorT m_InputValues;
  nonstd::span<const float32> m_UnifiedBounds;
  std::vector<CompleteStatsCache<T>>& m_StatsVector;
};

template <class Cache>
concept CacheType = std::is_base_of_v<StatsCache<typename Cache::value_type>, Cache>;

/**
 * @class ComputeStdDevImpl
 * @brief Computes population standard deviation from cached base statistics.
 * @tparam T Input value type.
 * @tparam CacheT Base-statistics cache type.
 * @tparam InputAccessorT Input accessor type.
 * @pre Cache and output vectors have one entry for each bounds tuple.
 *
 * The second input pass preserves the existing population-deviation formula.
 * Parallel workers write disjoint output-vector entries.
 */
template <typename T, CacheType CacheT, class InputAccessorT>
class ComputeStdDevImpl
{
public:
  ComputeStdDevImpl(const ImageGeom& geom, InputAccessorT inputValues, nonstd::span<const float32> unifiedBounds, const std::vector<CacheT>& statsVector, std::vector<float32>& stdDevValues)
  : m_Geom(geom)
  , m_InputValues(inputValues)
  , m_UnifiedBounds(unifiedBounds)
  , m_StatsVector(statsVector)
  , m_StdDevValues(stdDevValues)
  {
  }
  ~ComputeStdDevImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    usize xPoints = m_Geom.getNumXCells();
    usize yPoints = m_Geom.getNumYCells();

    for(usize targetBoundsIndex = start; targetBoundsIndex < end; targetBoundsIndex++)
    {
      // Skip empty bounds to prevent division by zero.
      if(m_StatsVector[targetBoundsIndex].count == 0)
      {
        continue;
      }

      std::array<usize, 6> voxelIndices = GetVoxelIndices(m_UnifiedBounds, targetBoundsIndex, m_Geom);

      // Accumulate locally and publish one value after the traversal.
      float64 sumOfDiffs = 0.0f;
      float32 meanValue = 0.0f;
      meanValue = m_StatsVector[targetBoundsIndex].summationValue / static_cast<float32>(m_StatsVector[targetBoundsIndex].count);

      usize zStride = 0, yStride = 0;
      for(usize zIndex = voxelIndices[k_MinZIndex]; zIndex < voxelIndices[k_MaxZIndex]; zIndex++)
      {
        zStride = zIndex * xPoints * yPoints;
        for(usize yIndex = voxelIndices[k_MinYIndex]; yIndex < voxelIndices[k_MaxYIndex]; yIndex++)
        {
          yStride = yIndex * xPoints;
          for(usize xIndex = voxelIndices[k_MinXIndex]; xIndex < voxelIndices[k_MaxXIndex]; xIndex++)
          {
            usize tup = zStride + yStride + xIndex;
            T value = m_InputValues.value(tup);
            sumOfDiffs += static_cast<float64>((value - meanValue) * (value - meanValue));
          }
        }
      }

      m_StdDevValues[targetBoundsIndex] = static_cast<float32>(std::sqrt(sumOfDiffs / static_cast<float64>(m_StatsVector[targetBoundsIndex].count)));
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const ImageGeom& m_Geom;
  InputAccessorT m_InputValues;
  nonstd::span<const float32> m_UnifiedBounds;
  const std::vector<CacheT>& m_StatsVector;
  std::vector<float32>& m_StdDevValues;
};

/**
 * @brief Computes and serially stores population standard deviations.
 * @tparam T Input value type.
 * @tparam StatsCacheT Base-statistics cache type.
 * @tparam InputAccessorT Input accessor type.
 * @param dataAlg Configures the bound-level execution range and concurrency.
 * @param imageGeom Supplies the image dimensions.
 * @param inputAccessor Reads input values.
 * @param unifiedBounds Contains six values for each bounding box.
 * @param statsVector Supplies counts and sums.
 * @param dataStructure Contains the standard-deviation output.
 * @param filterValues Identifies the output path.
 *
 * Workers write a temporary vector. Serial DataStore writes occur after they
 * join because the generic output interface does not guarantee thread safety.
 */
template <typename T, CacheType StatsCacheT, class InputAccessorT>
void ComputeAndStoreStdDeviation(ParallelDataAlgorithm& dataAlg, const ImageGeom& imageGeom, InputAccessorT inputAccessor, nonstd::span<const float32> unifiedBounds,
                                 const std::vector<StatsCacheT>& statsVector, DataStructure& dataStructure, const ComputeBoundingBoxStatsInputValues* filterValues)
{
  std::vector<float32> stdDevValues(statsVector.size(), 0.0f);
  dataAlg.execute(ComputeStdDevImpl<T, StatsCacheT, InputAccessorT>(imageGeom, inputAccessor, unifiedBounds, statsVector, stdDevValues));

  auto& stdDevArray = dataStructure.getDataRefAs<Float32Array>(filterValues->StdDevPath).getDataStoreRef();
  for(usize index = 0; index < statsVector.size(); index++)
  {
    if(statsVector[index].count > 0)
    {
      stdDevArray.setValue(index, stdDevValues[index]);
    }
  }
}

/**
 * @brief Writes completed cache values to selected framework outputs.
 * @tparam T Input value type.
 * @tparam StatsCacheT Statistics cache type.
 * @param statsVector Supplies completed values for all boxes.
 * @param dataStructure Contains output arrays.
 * @param inputValues Selects outputs and identifies their paths.
 * @return Success, or an output-store error.
 *
 * Serial writes avoid relying on generic DataStore thread safety.
 */
template <typename T, CacheType StatsCacheT>
Result<> FillStatsArrays(const std::vector<StatsCacheT>& statsVector, DataStructure& dataStructure, const ComputeBoundingBoxStatsInputValues* inputValues)
{
  AbstractDataStore<bool>* boundsHasDataArray = dataStructure.getDataRefAs<BoolArray>(inputValues->BoundsHasDataPath).getDataStore();
  if(boundsHasDataArray == nullptr)
  {
    return MakeErrorResult(-69309, fmt::format("Bounds Has Data array from path {} invalid", inputValues->BoundsHasDataPath.toString()));
  }

  AbstractDataStore<uint64>* lengthArray = nullptr;
  AbstractDataStore<T>* minArray = nullptr;
  AbstractDataStore<T>* maxArray = nullptr;
  AbstractDataStore<float32>* summationArray = nullptr;
  AbstractDataStore<float32>* meanArray = nullptr;
  AbstractDataStore<float32>* medianArray = nullptr;
  AbstractDataStore<int32>* numUniqueValuesArray = nullptr;

  if(inputValues->CalculateLength)
  {
    lengthArray = dataStructure.getDataRefAs<UInt64Array>(inputValues->LengthPath).getDataStore();
    if(lengthArray == nullptr)
    {
      return MakeErrorResult(-69310, fmt::format("Count array from path {} invalid", inputValues->LengthPath.toString()));
    }
  }
  if(inputValues->CalculateMin)
  {
    minArray = dataStructure.getDataRefAs<DataArray<T>>(inputValues->MinPath).getDataStore();
    if(minArray == nullptr)
    {
      return MakeErrorResult(-69311, fmt::format("Min array from path {} invalid", inputValues->MinPath.toString()));
    }
  }
  if(inputValues->CalculateMax)
  {
    maxArray = dataStructure.getDataRefAs<DataArray<T>>(inputValues->MaxPath).getDataStore();
    if(maxArray == nullptr)
    {
      return MakeErrorResult(-69312, fmt::format("Max array from path {} invalid", inputValues->MaxPath.toString()));
    }
  }
  if(inputValues->CalculateSummation)
  {
    summationArray = dataStructure.getDataRefAs<DataArray<float32>>(inputValues->SummationPath).getDataStore();
    if(summationArray == nullptr)
    {
      return MakeErrorResult(-69313, fmt::format("Summation array from path {} invalid", inputValues->SummationPath.toString()));
    }
  }
  if(inputValues->CalculateMean)
  {
    meanArray = dataStructure.getDataRefAs<Float32Array>(inputValues->MeanPath).getDataStore();
    if(meanArray == nullptr)
    {
      return MakeErrorResult(-69314, fmt::format("Mean array from path {} invalid", inputValues->MeanPath.toString()));
    }
  }
  if constexpr(std::is_same_v<StatsCacheT, CompleteStatsCache<typename StatsCacheT::value_type>>)
  {
    if(inputValues->CalculateMedian)
    {
      medianArray = dataStructure.getDataRefAs<Float32Array>(inputValues->MedianPath).getDataStore();
      if(meanArray == nullptr)
      {
        return MakeErrorResult(-69315, fmt::format("Median array from path {} invalid", inputValues->MedianPath.toString()));
      }
    }
    if(inputValues->CalculateNumUniqueValues)
    {
      numUniqueValuesArray = dataStructure.getDataRefAs<Int32Array>(inputValues->NumUniqueValuesPath).getDataStore();
      if(numUniqueValuesArray == nullptr)
      {
        return MakeErrorResult(-69316, fmt::format("Number of Unique Value array from path {} invalid", inputValues->MedianPath.toString()));
      }
    }
  }

  for(usize i = 0; i < statsVector.size(); i++)
  {
    if(statsVector[i].count > 0) // This guards against dividing by zero
    {
      boundsHasDataArray->setValue(i, true);
      if(lengthArray != nullptr)
      {
        lengthArray->setValue(i, statsVector[i].count);
      }
      if(minArray != nullptr)
      {
        minArray->setValue(i, statsVector[i].minValue);
      }
      if(maxArray != nullptr)
      {
        maxArray->setValue(i, statsVector[i].maxValue);
      }
      if(summationArray != nullptr)
      {
        summationArray->setValue(i, statsVector[i].summationValue);
      }
      if(meanArray != nullptr)
      {
        float32 meanValue = 0.0f;
        meanValue = statsVector[i].summationValue / static_cast<float32>(statsVector[i].count);
        meanArray->setValue(i, meanValue);
      }
      if constexpr(std::is_same_v<StatsCacheT, CompleteStatsCache<typename StatsCacheT::value_type>>)
      {
        if(medianArray != nullptr)
        {
          medianArray->setValue(i, statsVector[i].medianValue);
        }
        if(numUniqueValuesArray != nullptr)
        {
          numUniqueValuesArray->setValue(i, statsVector[i].uniqueValCount);
        }
      }
    }
  }

  return {};
}

/**
 * @brief Computes independent bounds using bound-local state.
 * @tparam UseModeV Enables mode output.
 * @tparam T Input value type.
 * @tparam InputAccessorT Input accessor type.
 * @param dataStructure Contains output arrays.
 * @param inputValues Selects statistics and identifies output paths.
 * @param imageGeom Supplies the image dimensions.
 * @param unifiedBounds Contains six values for each bounding box.
 * @param inputAccessor Reads input values.
 * @param parallelize Enables parallel bound processing.
 * @return Success, or an output-store error.
 *
 * Contiguous input enables parallel direct reads. Framework output stores are
 * populated serially after the join because their generic interfaces do not
 * guarantee concurrent access safety.
 */
template <bool UseModeV, typename T, class InputAccessorT>
Result<> ComputeBoundsStats(DataStructure& dataStructure, const ComputeBoundingBoxStatsInputValues* inputValues, const ImageGeom& imageGeom, nonstd::span<const float32> unifiedBounds,
                            InputAccessorT inputAccessor, bool parallelize)
{
  const usize numBounds = unifiedBounds.size() / 6;

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, numBounds);
  dataAlg.setParallelizationEnabled(parallelize);

  const bool collectBaseStats = inputValues->CalculateMin || inputValues->CalculateMax || inputValues->CalculateSummation || inputValues->CalculateMean || inputValues->CalculateStdDev;

  if constexpr(UseModeV)
  {
    std::vector<CompleteStatsCache<T>> statsVector(numBounds);
    std::vector<std::shared_ptr<std::vector<T>>> modes(numBounds);
    for(auto& mode : modes)
    {
      mode = std::make_shared<std::vector<T>>();
    }

    if(collectBaseStats)
    {
      dataAlg.execute(ComputeAllStatsImpl<T, InputAccessorT, true>(imageGeom, inputAccessor, unifiedBounds, statsVector, modes));
    }
    else
    {
      dataAlg.execute(ComputeAllStatsImpl<T, InputAccessorT, false>(imageGeom, inputAccessor, unifiedBounds, statsVector, modes));
    }

    auto& modeList = dataStructure.getDataRefAs<NeighborList<T>>(inputValues->ModePath);
    for(usize index = 0; index < modes.size(); index++)
    {
      modeList.setList(static_cast<int32>(index), modes[index]);
    }

    if(inputValues->CalculateStdDev)
    {
      ComputeAndStoreStdDeviation<T>(dataAlg, imageGeom, inputAccessor, unifiedBounds, statsVector, dataStructure, inputValues);
    }

    return FillStatsArrays<T>(statsVector, dataStructure, inputValues);
  }
  else if(inputValues->CalculateMedian || inputValues->CalculateNumUniqueValues)
  {
    std::vector<CompleteStatsCache<T>> statsVector(numBounds);
    if(collectBaseStats)
    {
      dataAlg.execute(ComputeBasicAndFrequencyStatsImpl<T, InputAccessorT, true>(imageGeom, inputAccessor, unifiedBounds, statsVector));
    }
    else
    {
      dataAlg.execute(ComputeBasicAndFrequencyStatsImpl<T, InputAccessorT, false>(imageGeom, inputAccessor, unifiedBounds, statsVector));
    }

    if(inputValues->CalculateStdDev)
    {
      ComputeAndStoreStdDeviation<T>(dataAlg, imageGeom, inputAccessor, unifiedBounds, statsVector, dataStructure, inputValues);
    }

    return FillStatsArrays<T>(statsVector, dataStructure, inputValues);
  }
  else
  {
    std::vector<StatsCache<T>> statsVector(numBounds);
    dataAlg.execute(ComputeBaseStatsImpl<T, InputAccessorT>(imageGeom, inputAccessor, unifiedBounds, statsVector));

    if(inputValues->CalculateStdDev)
    {
      ComputeAndStoreStdDeviation<T>(dataAlg, imageGeom, inputAccessor, unifiedBounds, statsVector, dataStructure, inputValues);
    }

    return FillStatsArrays<T>(statsVector, dataStructure, inputValues);
  }
}

/**
 * @struct ExecuteBoundsStatsCalculations
 * @brief Dispatches the direct calculation for one runtime value type.
 * @tparam UseModeV Enables type-dispatched NeighborList mode output.
 */
template <bool UseModeV = false>
struct ExecuteBoundsStatsCalculations
{
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const ComputeBoundingBoxStatsInputValues* inputValues, const ImageGeom& imageGeom, const Float32AbstractDataStore& unifiedBounds,
                      const IDataArray& inputIDataArray)
  {
    std::vector<float32> unifiedBoundsValues(unifiedBounds.getSize());
    Result<> boundsResult = unifiedBounds.copyIntoBuffer(0, nonstd::span<float32>(unifiedBoundsValues.data(), unifiedBoundsValues.size()));
    if(boundsResult.invalid())
    {
      return boundsResult;
    }
    const nonstd::span<const float32> unifiedBoundsSpan(unifiedBoundsValues.data(), unifiedBoundsValues.size());

    const auto& inputStore = dynamic_cast<const DataArray<T>&>(inputIDataArray).getDataStoreRef();
    const auto* inMemoryStore = dynamic_cast<const DataStore<T>*>(&inputStore);
    if(inMemoryStore != nullptr)
    {
      return ComputeBoundsStats<UseModeV, T>(dataStructure, inputValues, imageGeom, unifiedBoundsSpan, ContiguousInputAccessor<T>(inMemoryStore->data()), true);
    }

    return ComputeBoundsStats<UseModeV, T>(dataStructure, inputValues, imageGeom, unifiedBoundsSpan, AbstractInputAccessor<T>(inputStore), false);
  }
};
} // namespace

// -----------------------------------------------------------------------------
ComputeBoundingBoxStatsDirect::ComputeBoundingBoxStatsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             const ComputeBoundingBoxStatsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundingBoxStatsDirect::~ComputeBoundingBoxStatsDirect() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeBoundingBoxStatsDirect::operator()()
{
  const auto& geom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GeometryPath);
  auto& unifiedArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->UnifiedPath).getDataStoreRef();
  auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputPath);
  if(inputArray.getDataType() == DataType::boolean)
  {
    return MakeErrorResult(-98500, "Boolean arrays cannot be used as inputs to this filter.");
  }
  if(m_InputValues->CalculateMode)
  {
    return ExecuteNeighborFunction(ExecuteBoundsStatsCalculations<true>{}, inputArray.getDataType(), m_DataStructure, m_InputValues, geom, unifiedArray, inputArray);
  }

  return ExecuteDataFunctionNoBool(ExecuteBoundsStatsCalculations<false>{}, inputArray.getDataType(), m_DataStructure, m_InputValues, geom, unifiedArray, inputArray);
}
