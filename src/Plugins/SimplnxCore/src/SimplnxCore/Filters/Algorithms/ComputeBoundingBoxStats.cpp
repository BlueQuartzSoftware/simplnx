#include "ComputeBoundingBoxStats.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
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
  T minValue = std::numeric_limits<T>::quiet_NaN();
  T maxValue = std::numeric_limits<T>::quiet_NaN();
  usize count = 0;
  T summationValue = static_cast<T>(0);
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
    FloatVec3 spacing = m_Geom.getSpacing();
    FloatVec3 origin = m_Geom.getOrigin();

    usize xPoints = m_Geom.getNumXCells();
    usize yPoints = m_Geom.getNumYCells();

    for(usize targetBoundsIndex = start; targetBoundsIndex < end; targetBoundsIndex++)
    {
      // Preflight handles checking that we don't divide by 0 by validating spacing, cutting extra checks here

      // We are inlining the calculations here to leverage the speed of primitives (no Point object or vector from the API)
      auto minXVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 0) - ((spacing[0] * 0.5f) + origin[0])) / spacing[0]));
      auto minYVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 1) - ((spacing[1] * 0.5f) + origin[1])) / spacing[1]));
      auto minZVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 2) - ((spacing[2] * 0.5f) + origin[2])) / spacing[2]));

      auto maxXVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 3) - ((spacing[0] * 0.5f) + origin[0])) / spacing[0])) + 1;
      auto maxYVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 4) - ((spacing[1] * 0.5f) + origin[1])) / spacing[1])) + 1;
      auto maxZVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 5) - ((spacing[2] * 0.5f) + origin[2])) / spacing[2])) + 1;

      // We are working with primitives here for their trivially copyable nature, this lets us cut accesses to output vector
      usize count = 0;
      T minValue = std::numeric_limits<T>::quiet_NaN();
      T maxValue = std::numeric_limits<T>::quiet_NaN();
      T summationValue = static_cast<T>(0);

      usize zStride = 0, yStride = 0;
      for(usize zIndex = minZVoxel; zIndex < maxZVoxel; zIndex++)
      {
        zStride = zIndex * xPoints * yPoints;
        for(usize yIndex = minYVoxel; yIndex < maxYVoxel; yIndex++)
        {
          yStride = yIndex * xPoints;
          for(usize xIndex = minXVoxel; xIndex < maxXVoxel; xIndex++)
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
 * @brief This computes the basic stats and mode by bounding box that can be derived in a single pass
 * @tparam T the type of data for the stats to work from (bool invalid since we are working with NeighborList as a variable type)
 * @warning Class assumes that the size of statsVector and modesList is equivalent to numTuples in unifiedBounds to maintain parallel nature
 */
template <typename T>
class ComputeBaseStatsAndModeImpl
{
public:
  // It is expected that the size of statsVector is equivalent to numTuples in unifiedBounds
  ComputeBaseStatsAndModeImpl(const ImageGeom& geom, const AbstractDataStore<T>& inputArray, const Float32AbstractDataStore& unifiedBounds, std::vector<StatsCache<T>>& statsVector,
                              NeighborList<T>& modesList)
  : m_Geom(geom)
  , m_InputArray(inputArray)
  , m_UnifiedBounds(unifiedBounds)
  , m_StatsVector(statsVector)
  , m_ModesList(modesList)
  {
  }
  ~ComputeBaseStatsAndModeImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    FloatVec3 spacing = m_Geom.getSpacing();
    FloatVec3 origin = m_Geom.getOrigin();

    usize xPoints = m_Geom.getNumXCells();
    usize yPoints = m_Geom.getNumYCells();

    for(usize targetBoundsIndex = start; targetBoundsIndex < end; targetBoundsIndex++)
    {
      // Preflight handles checking that we don't divide by 0 by validating spacing, cutting extra checks here

      // We are inlining the calculations here to leverage the speed of primitives (no Point object or vector from the API)
      auto minXVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 0) - ((spacing[0] * 0.5f) + origin[0])) / spacing[0]));
      auto minYVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 1) - ((spacing[1] * 0.5f) + origin[1])) / spacing[1]));
      auto minZVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 2) - ((spacing[2] * 0.5f) + origin[2])) / spacing[2]));

      auto maxXVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 3) - ((spacing[0] * 0.5f) + origin[0])) / spacing[0])) + 1;
      auto maxYVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 4) - ((spacing[1] * 0.5f) + origin[1])) / spacing[1])) + 1;
      auto maxZVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 5) - ((spacing[2] * 0.5f) + origin[2])) / spacing[2])) + 1;

      // We are working with primitives here for their trivially copyable nature, this lets us cut accesses to output vector
      usize count = 0;
      T minValue = std::numeric_limits<T>::quiet_NaN();
      T maxValue = std::numeric_limits<T>::quiet_NaN();
      T summationValue = static_cast<T>(0);

      // specialization also calculates mode
      std::map<T, uint64> modalMap = {};

      usize zStride = 0, yStride = 0;
      for(usize zIndex = minZVoxel; zIndex < maxZVoxel; zIndex++)
      {
        zStride = zIndex * xPoints * yPoints;
        for(usize yIndex = minYVoxel; yIndex < maxYVoxel; yIndex++)
        {
          yStride = yIndex * xPoints;
          for(usize xIndex = minXVoxel; xIndex < maxXVoxel; xIndex++)
          {
            usize tup = zStride + yStride + xIndex;
            T value = m_InputArray.getValue(tup);
            count++;
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
            summationValue += value;

            // modes
            modalMap[value]++;
          }
        }
      }

      // Copy primitives of base stats in the output vector
      m_StatsVector[targetBoundsIndex].count = count;
      m_StatsVector[targetBoundsIndex].minValue = minValue;
      m_StatsVector[targetBoundsIndex].maxValue = maxValue;
      m_StatsVector[targetBoundsIndex].summationValue = summationValue;

      // Output the mode
      if(!modalMap.empty())
      {
        continue;
      }

      // Find the maximum occurrence
      auto pr = std::max_element(modalMap.begin(), modalMap.end(), [](const auto& x, const auto& y) { return x.second < y.second; });
      int maxCount = pr->second;

      // Store all values that have this maximum occurrence under the proper feature id
      for(const auto& modalPair : modalMap)
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
  std::vector<StatsCache<T>>& m_StatsVector;
  NeighborList<T>& m_ModesList;
};

/**
 * @brief This computes the standard deviation by bounding box that can be derived from precalculated base stats
 * @tparam T the type of data for the stats to work from
 * @warning Class assumes that the size of statsVector is equivalent to numTuples in unifiedBounds to maintain parallel nature
 */
template <typename T>
class ComputeStdDevImpl
{
public:
  // It is expected that the size of statsVector is equivalent to numTuples in unifiedBounds
  ComputeStdDevImpl(const ImageGeom& geom, const AbstractDataStore<T>& inputArray, const Float32AbstractDataStore& unifiedBounds, const std::vector<StatsCache<T>>& statsVector,
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
    FloatVec3 spacing = m_Geom.getSpacing();
    FloatVec3 origin = m_Geom.getOrigin();

    usize xPoints = m_Geom.getNumXCells();
    usize yPoints = m_Geom.getNumYCells();

    for(usize targetBoundsIndex = start; targetBoundsIndex < end; targetBoundsIndex++)
    {
      // Prevents dividing by zero
      if(m_StatsVector[targetBoundsIndex].count == 0)
      {
        continue;
      }

      // Preflight handles checking that we don't divide by 0 by validating spacing, cutting extra checks here

      // We are inlining the calculations here to leverage the speed of primitives (no Point object or vector from the API)
      auto minXVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 0) - ((spacing[0] * 0.5f) + origin[0])) / spacing[0]));
      auto minYVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 1) - ((spacing[1] * 0.5f) + origin[1])) / spacing[1]));
      auto minZVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 2) - ((spacing[2] * 0.5f) + origin[2])) / spacing[2]));

      auto maxXVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 3) - ((spacing[0] * 0.5f) + origin[0])) / spacing[0])) + 1;
      auto maxYVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 4) - ((spacing[1] * 0.5f) + origin[1])) / spacing[1])) + 1;
      auto maxZVoxel = static_cast<usize>(std::floor((m_UnifiedBounds.getValue((targetBoundsIndex * 6) + 5) - ((spacing[2] * 0.5f) + origin[2])) / spacing[2])) + 1;

      // We are working with primitives here for their trivially copyable nature, this lets us cut accesses to output vector
      float64 sumOfDiffs = 0.0f;
      float32 meanValue = m_StatsVector[targetBoundsIndex].summationValue / static_cast<float32>(m_StatsVector[targetBoundsIndex].count);

      usize zStride = 0, yStride = 0;
      for(usize zIndex = minZVoxel; zIndex < maxZVoxel; zIndex++)
      {
        zStride = zIndex * xPoints * yPoints;
        for(usize yIndex = minYVoxel; yIndex < maxYVoxel; yIndex++)
        {
          yStride = yIndex * xPoints;
          for(usize xIndex = minXVoxel; xIndex < maxXVoxel; xIndex++)
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
  const std::vector<StatsCache<T>>& m_StatsVector;
  Float32AbstractDataStore& m_StdDevArray;
};

template <typename T>
Result<> FillStatsArrays(const std::vector<StatsCache<T>>& statsVector, DataStructure& dataStructure, const ComputeBoundingBoxStatsInputValues* inputValues)
{
  AbstractDataStore<bool>* boundsHasDataArray = dataStructure.getDataRefAs<BoolArray>(inputValues->BoundsHasDataPath).getDataStore();
  if(boundsHasDataArray == nullptr)
  {
    return MakeErrorResult(-69309, fmt::format("Bounds Has Data array from path {} invalid", inputValues->BoundsHasDataPath.toString()));
  }

  AbstractDataStore<uint64>* lengthArray = nullptr;
  AbstractDataStore<T>* minArray = nullptr;
  AbstractDataStore<T>* maxArray = nullptr;
  AbstractDataStore<T>* summationArray = nullptr;
  AbstractDataStore<float32>* meanArray = nullptr;

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
    summationArray = dataStructure.getDataRefAs<DataArray<T>>(inputValues->SummationPath).getDataStore();
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
        if constexpr(std::is_same_v<T, bool>)
        {
          meanValue = static_cast<float32>(statsVector[i].summationValue >= (statsVector.size() - statsVector[i].summationValue));
        }
        else
        {
          meanValue = statsVector[i].summationValue / static_cast<float32>(statsVector[i].count);
        }
        meanArray->setValue(i, meanValue);
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

    // Initialize StatsCache vector
    std::vector<StatsCache<T>> statsVector(numTuples);

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, numTuples);

    const auto& inputArray = dynamic_cast<const DataArray<T>&>(inputIDataArray).getDataStoreRef();

    if constexpr(UseModeV)
    {
      auto& modeList = dataStructure.getDataRefAs<NeighborList<T>>(inputValues->ModePath);
      dataAlg.execute(ComputeBaseStatsAndModeImpl<T>(imageGeom, inputArray, unifiedBounds, statsVector, modeList));
    }
    else
    {
      dataAlg.execute(ComputeBaseStatsImpl<T>(imageGeom, inputArray, unifiedBounds, statsVector));
    }

    if(inputValues->CalculateStdDev)
    {
      auto& stdDevArray = dataStructure.getDataRefAs<Float32Array>(inputValues->StdDevPath).getDataStoreRef(); // get abstract data store
      dataAlg.execute(ComputeStdDevImpl<T>(imageGeom, inputArray, unifiedBounds, statsVector, stdDevArray));
    }

    return FillStatsArrays<T>(statsVector, dataStructure, inputValues);
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
  if(m_InputValues->CalculateMode)
  {
    return ExecuteNeighborFunction(ExecuteBoundsStatsCalculations<true>{}, inputArray.getDataType(), m_DataStructure, m_InputValues, geom, unifiedArray, inputArray);
  }
  else
  {
    return ExecuteDataFunction(ExecuteBoundsStatsCalculations<false>{}, inputArray.getDataType(), m_DataStructure, m_InputValues, geom, unifiedArray, inputArray);
  }
}
