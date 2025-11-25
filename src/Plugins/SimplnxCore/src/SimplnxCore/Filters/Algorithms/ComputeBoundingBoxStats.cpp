#include "ComputeBoundingBoxStats.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
constexpr usize k_MinXIndex = 0;
constexpr usize k_MinYIndex = 1;
constexpr usize k_MinZIndex = 2;
constexpr usize k_MaxXIndex = 3;
constexpr usize k_MaxYIndex = 4;
constexpr usize k_MaxZIndex = 5;

template <class T>
concept ArithmeticNotBool = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;

std::array<usize, 6> GetVoxelIndices(const Float32AbstractDataStore& unifiedBounds, usize targetBoundsIndex, const ImageGeom& image)
{
  std::array<usize, 6> voxelIndices = {};

  // Preflight handles checking that we don't divide by 0 by validating spacing, cutting extra checks here
  FloatVec3 spacing = image.getSpacing();
  FloatVec3 origin = image.getOrigin();
  SizeVec3 dims = image.getDimensions();

  // The lower bound from input is expected to be the lower corner of the voxel
  for(usize i = 0; i < 3; i++)
  {
    float32 minVoxel = unifiedBounds.getValue((targetBoundsIndex * 6) + i);
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
    float32 maxVoxel = unifiedBounds.getValue((targetBoundsIndex * 6) + offset);
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

/** Mode and Std dev are left out of cache intentionally, every other stat can be derived from these.
 * Reasoning:
 * 1. In order to calculate mode you must create a data container to keep track of instances of a value,
 * this would massively bloat memory cost if mode is not selected, thus it cannot be included. Due to
 * the nature mode can be found in the first pass so a specialized function will handle it.
 * 2. The std-deviation requires a second pass, so there is no need to store it a separate function will
 * be run after this cache is calculated upon user request.
 **/
template <typename T>
struct StatsCache
{
  using value_type = T;
  T minValue = std::numeric_limits<T>::quiet_NaN();
  T maxValue = std::numeric_limits<T>::quiet_NaN();
  usize count = 0;
  T summationValue = static_cast<T>(0);
};

template <typename T>
struct CompleteStatsCache : StatsCache<T>
{
  float32 medianValue = std::numeric_limits<float32>::quiet_NaN();
  usize uniqueValCount = 0;
};

/**
 * @brief This computes the basic stats by bounding box that can be derived in a single pass aside from mode
 * @tparam T the type of data for the stats to work from
 * @warning Class assumes that the size of statsVector is equivalent to numTuples in unifiedBounds to maintain parallel nature
 */
template <typename T>
class ComputeBaseStatsImpl
{
public:
  // It is expected that the size of statsVector is equivalent to numTuples in unifiedBounds
  ComputeBaseStatsImpl(const ImageGeom& geom, const AbstractDataStore<T>& inputArray, const Float32AbstractDataStore& unifiedBounds, std::vector<StatsCache<T>>& statsVector)
  : m_Geom(geom)
  , m_InputArray(inputArray)
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

      // We are working with primitives here for their trivially copyable nature, this lets us cut accesses to output vector
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
            T value = m_InputArray.getValue(tup);
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

      // Copy primitives of base stats in the output vector
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
  const AbstractDataStore<T>& m_InputArray;
  const Float32AbstractDataStore& m_UnifiedBounds;
  std::vector<StatsCache<T>>& m_StatsVector;
};

/**
 * @brief This computes the basic stats, frequency map stats, and mode by bounding box that can be derived in a single pass
 * @tparam T the type of data for the stats to work from (bool invalid since we are working with NeighborList as a variable type)
 * @warning Class assumes that the size of statsVector and modesList is equivalent to numTuples in unifiedBounds to maintain parallel nature
 */
template <typename T>
class ComputeAllStatsImpl
{
public:
  // It is expected that the size of statsVector is equivalent to numTuples in unifiedBounds
  ComputeAllStatsImpl(const ImageGeom& geom, const AbstractDataStore<T>& inputArray, const Float32AbstractDataStore& unifiedBounds, std::vector<CompleteStatsCache<T>>& statsVector,
                      NeighborList<T>& modesList)
  : m_Geom(geom)
  , m_InputArray(inputArray)
  , m_UnifiedBounds(unifiedBounds)
  , m_StatsVector(statsVector)
  , m_ModesList(modesList)
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

      // We are working with primitives here for their trivially copyable nature, this lets us cut accesses to output vector
      usize count = 0;
      T minValue = std::numeric_limits<T>::max();
      T maxValue = std::numeric_limits<T>::lowest();
      T summationValue = static_cast<T>(0);

      // specialization also calculates value based statistics
      std::map<T, uint64> frequencyMap = {};

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
            T value = m_InputArray.getValue(tup);
            count++;
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
            summationValue += value;

            // modes
            frequencyMap[value]++;
          }
        }
      }

      if(count == 0)
      {
        minValue = std::numeric_limits<T>::quiet_NaN();
        maxValue = std::numeric_limits<T>::quiet_NaN();
      }

      // Copy primitives of base stats in the output vector
      m_StatsVector[targetBoundsIndex].count = count;
      m_StatsVector[targetBoundsIndex].minValue = minValue;
      m_StatsVector[targetBoundsIndex].maxValue = maxValue;
      m_StatsVector[targetBoundsIndex].summationValue = summationValue;

      if(frequencyMap.empty())
      {
        continue;
      }

      // Find Number of Unique Values from Frequency Map
      m_StatsVector[targetBoundsIndex].uniqueValCount = frequencyMap.size();

      // Calculate the median
      usize medianPosition = (count / 2) + 1;
      usize cumulativeFrequency = 0;
      for(auto it = frequencyMap.begin(); it != frequencyMap.end(); ++it)
      {
        cumulativeFrequency += it->second;

        // DO NOT TOUCH LESS THAN CHECK, basis of assumption for next frequency ifs
        if(cumulativeFrequency < medianPosition)
        {
          continue;
        }

        if(count % 2 == 0 && cumulativeFrequency == medianPosition)
        {
          // If we reached this point the frequency of this number is 1,
          // meaning the previous key contains the n-1 value
          auto upper = static_cast<float32>(it->first);
          --it;
          m_StatsVector[targetBoundsIndex].medianValue = (static_cast<float32>(it->first) + upper) / 2.0f;
        }
        else
        {
          // If we reached this point the number of values is either
          // - uneven, meaning this is the exact median
          // - even, but the frequency of the key value is greater than 1
          // meaning that both n and n-1 are the same number
          m_StatsVector[targetBoundsIndex].medianValue = static_cast<float32>(it->first);
        }

        break;
      }

      // Find the maximum occurrence
      auto pr = std::max_element(frequencyMap.begin(), frequencyMap.end(), [](const auto& x, const auto& y) { return x.second < y.second; });
      int maxCount = pr->second;

      // Store all values that have this maximum occurrence under the proper feature id
      for(const auto& modalPair : frequencyMap)
      {
        if(modalPair.second == maxCount)
        {
          m_ModesList.addEntry(targetBoundsIndex, modalPair.first);
        }
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
  const AbstractDataStore<T>& m_InputArray;
  const Float32AbstractDataStore& m_UnifiedBounds;
  std::vector<CompleteStatsCache<T>>& m_StatsVector;
  NeighborList<T>& m_ModesList;
};

/**
 * @brief This computes the basic and frequency map stats by bounding box that can be derived in a single pass
 * @tparam T the type of data for the stats to work from (bool invalid since we are working with NeighborList as a variable type)
 * @warning Class assumes that the size of statsVector and modesList is equivalent to numTuples in unifiedBounds to maintain parallel nature
 */
template <typename T>
class ComputeBasicAndFrequencyStatsImpl
{
public:
  // It is expected that the size of statsVector is equivalent to numTuples in unifiedBounds
  ComputeBasicAndFrequencyStatsImpl(const ImageGeom& geom, const AbstractDataStore<T>& inputArray, const Float32AbstractDataStore& unifiedBounds, std::vector<CompleteStatsCache<T>>& statsVector)
  : m_Geom(geom)
  , m_InputArray(inputArray)
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

      // We are working with primitives here for their trivially copyable nature, this lets us cut accesses to output vector
      usize count = 0;
      T minValue = std::numeric_limits<T>::max();
      T maxValue = std::numeric_limits<T>::lowest();
      T summationValue = static_cast<T>(0);

      // specialization also calculates value based statistics
      std::map<T, uint64> frequencyMap = {};

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
            T value = m_InputArray.getValue(tup);
            count++;
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
            summationValue += value;
            // modes
            frequencyMap[value]++;
          }
        }
      }

      if(count == 0)
      {
        minValue = std::numeric_limits<T>::quiet_NaN();
        maxValue = std::numeric_limits<T>::quiet_NaN();
      }

      // Copy primitives of base stats in the output vector
      m_StatsVector[targetBoundsIndex].count = count;
      m_StatsVector[targetBoundsIndex].minValue = minValue;
      m_StatsVector[targetBoundsIndex].maxValue = maxValue;
      m_StatsVector[targetBoundsIndex].summationValue = summationValue;

      if(frequencyMap.empty())
      {
        continue;
      }

      // Find Number of Unique Values from Frequency Map
      m_StatsVector[targetBoundsIndex].uniqueValCount = frequencyMap.size();

      // Calculate the median
      usize medianPosition = (count / 2) + 1;
      usize cumulativeFrequency = 0;
      for(auto it = frequencyMap.begin(); it != frequencyMap.end(); ++it)
      {
        cumulativeFrequency += it->second;

        // DO NOT TOUCH LESS THAN CHECK, basis of assumption for next frequency ifs
        if(cumulativeFrequency < medianPosition)
        {
          continue;
        }

        if(count % 2 == 0 && cumulativeFrequency == medianPosition)
        {
          // If we reached this point the frequency of this number is 1,
          // meaning the previous key contains the n-1 value
          auto upper = static_cast<float32>(it->first);
          --it;
          m_StatsVector[targetBoundsIndex].medianValue = (static_cast<float32>(it->first) + upper) / 2.0f;
        }
        else
        {
          // If we reached this point the number of values is either
          // - uneven, meaning this is the exact median
          // - even, but the frequency of the key value is greater than 1
          // meaning that both n and n-1 are the same number
          m_StatsVector[targetBoundsIndex].medianValue = static_cast<float32>(it->first);
        }

        break;
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
  const AbstractDataStore<T>& m_InputArray;
  const Float32AbstractDataStore& m_UnifiedBounds;
  std::vector<CompleteStatsCache<T>>& m_StatsVector;
};

template <class Cache>
concept CacheType = std::is_base_of_v<StatsCache<typename Cache::value_type>, Cache>;

/**
 * @brief This computes the standard deviation by bounding box that can be derived from precalculated base stats
 * @tparam T the type of data for the stats to work from
 * @warning Class assumes that the size of statsVector is equivalent to numTuples in unifiedBounds to maintain parallel nature
 */
template <typename T, CacheType CacheT>
class ComputeStdDevImpl
{
public:
  // It is expected that the size of statsVector is equivalent to numTuples in unifiedBounds
  ComputeStdDevImpl(const ImageGeom& geom, const AbstractDataStore<T>& inputArray, const Float32AbstractDataStore& unifiedBounds, const std::vector<CacheT>& statsVector,
                    Float32AbstractDataStore& stdDevArray)
  : m_Geom(geom)
  , m_InputArray(inputArray)
  , m_UnifiedBounds(unifiedBounds)
  , m_StatsVector(statsVector)
  , m_StdDevArray(stdDevArray)
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
      // Prevents dividing by zero
      if(m_StatsVector[targetBoundsIndex].count == 0)
      {
        continue;
      }

      std::array<usize, 6> voxelIndices = GetVoxelIndices(m_UnifiedBounds, targetBoundsIndex, m_Geom);

      // We are working with primitives here for their trivially copyable nature, this lets us cut accesses to output vector
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
            T value = m_InputArray.getValue(tup);
            sumOfDiffs += static_cast<float64>((value - meanValue) * (value - meanValue));
          }
        }
      }

      // Copy primitives of base stats in the output vector
      m_StdDevArray.setValue(targetBoundsIndex, static_cast<float32>(std::sqrt(sumOfDiffs / static_cast<float64>(m_StatsVector[targetBoundsIndex].count))));
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const ImageGeom& m_Geom;
  const AbstractDataStore<T>& m_InputArray;
  const Float32AbstractDataStore& m_UnifiedBounds;
  const std::vector<CacheT>& m_StatsVector;
  Float32AbstractDataStore& m_StdDevArray;
};

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

template <bool UseModeV = false>
struct ExecuteBoundsStatsCalculations
{
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const ComputeBoundingBoxStatsInputValues* inputValues, const ImageGeom& imageGeom, const Float32AbstractDataStore& unifiedBounds,
                      const IDataArray& inputIDataArray)
  {
    usize numTuples = unifiedBounds.getNumberOfTuples();

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, numTuples);

    const auto& inputArray = dynamic_cast<const DataArray<T>&>(inputIDataArray).getDataStoreRef();

    if constexpr(UseModeV)
    {
      std::vector<CompleteStatsCache<T>> statsVector(numTuples);
      auto& modeList = dataStructure.getDataRefAs<NeighborList<T>>(inputValues->ModePath);
      dataAlg.execute(ComputeAllStatsImpl<T>(imageGeom, inputArray, unifiedBounds, statsVector, modeList));

      if(inputValues->CalculateStdDev)
      {
        auto& stdDevArray = dataStructure.getDataRefAs<Float32Array>(inputValues->StdDevPath).getDataStoreRef();
        dataAlg.execute(ComputeStdDevImpl<T, CompleteStatsCache<T>>(imageGeom, inputArray, unifiedBounds, statsVector, stdDevArray));
      }

      return FillStatsArrays<T>(statsVector, dataStructure, inputValues);
    }
    else
    {
      if(inputValues->CalculateMedian || inputValues->CalculateNumUniqueValues)
      {
        std::vector<CompleteStatsCache<T>> statsVector(numTuples);
        dataAlg.execute(ComputeBasicAndFrequencyStatsImpl<T>(imageGeom, inputArray, unifiedBounds, statsVector));

        if(inputValues->CalculateStdDev)
        {
          auto& stdDevArray = dataStructure.getDataRefAs<Float32Array>(inputValues->StdDevPath).getDataStoreRef();
          dataAlg.execute(ComputeStdDevImpl<T, CompleteStatsCache<T>>(imageGeom, inputArray, unifiedBounds, statsVector, stdDevArray));
        }

        return FillStatsArrays<T>(statsVector, dataStructure, inputValues);
      }
      else
      {
        std::vector<StatsCache<T>> statsVector(numTuples);
        dataAlg.execute(ComputeBaseStatsImpl<T>(imageGeom, inputArray, unifiedBounds, statsVector));

        if(inputValues->CalculateStdDev)
        {
          auto& stdDevArray = dataStructure.getDataRefAs<Float32Array>(inputValues->StdDevPath).getDataStoreRef();
          dataAlg.execute(ComputeStdDevImpl<T, StatsCache<T>>(imageGeom, inputArray, unifiedBounds, statsVector, stdDevArray));
        }

        return FillStatsArrays<T>(statsVector, dataStructure, inputValues);
      }
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
ComputeBoundingBoxStats::ComputeBoundingBoxStats(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeBoundingBoxStatsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundingBoxStats::~ComputeBoundingBoxStats() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeBoundingBoxStats::operator()()
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
