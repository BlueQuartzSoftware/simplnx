#include "InitializeImageGeomCellData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <random>
#include <vector>

using namespace nx::core;

namespace
{
using RangeType = std::pair<float64, float64>;
// Row writes target 65,536 values and retain at least one complete tuple.
constexpr usize k_InitializationChunkValues = 65536;

/**
 * @enum InitType
 * @brief Specifies internal image-cell initialization modes.
 */
enum class InitType : uint64
{
  Manual = 0,         ///< Uses the configured constant value.
  Random = 1,         ///< Uses default scalar bounds.
  RandomWithRange = 2 ///< Uses configured scalar bounds.
};

/**
 * @brief Converts a persisted choice index to an initialization mode.
 * @param index Specifies the persisted choice index.
 * @return Corresponding initialization mode.
 * @throws std::runtime_error when index does not identify a supported mode.
 */
InitType ConvertIndexToInitType(uint64 index)
{
  switch(index)
  {
  case static_cast<uint64>(InitType::Manual): {
    return InitType::Manual;
  }
  case static_cast<uint64>(InitType::Random): {
    return InitType::Random;
  }
  case static_cast<uint64>(InitType::RandomWithRange): {
    return InitType::RandomWithRange;
  }
  default: {
    throw std::runtime_error("InitializeImageGeomCellData: Invalid value for InitType");
  }
  }
}

/**
 * @brief Creates a seeded distribution and generator for one target array.
 * @tparam T Specifies the target scalar type.
 * @param rangeMin Distribution lower bound.
 * @param rangeMax Inclusive integral or excluded floating upper bound.
 * @param seed Specifies the generator seed.
 * @return Distribution and generator pair for T.
 *
 * Integral types use uniform_int_distribution<int>. Bounds outside int cannot
 * be represented by that distribution.
 */
template <class T>
auto CreateRandomGenerator(T rangeMin, T rangeMax, uint64 seed)
{
  std::random_device randomDevice;
  std::mt19937_64 generator(randomDevice());
  generator.seed(seed);

  if constexpr(std::is_integral_v<T>)
  {
    std::uniform_int_distribution<> distribution(rangeMin, rangeMax);
    return std::make_pair(distribution, generator);
  }
  else if constexpr(std::is_floating_point_v<T>)
  {
    std::uniform_real_distribution<T> distribution(rangeMin, rangeMax);
    return std::make_pair(distribution, generator);
  }
}

/**
 * @struct InitializeArrayFunctor
 * @brief Initializes one typed image-cell subvolume through bounded contiguous row segments.
 *
 * Values follow X, Y, and Z order so seeded output remains reproducible. One
 * generated value fills all components of a tuple. Each row segment writes once.
 */
struct InitializeArrayFunctor
{
  /**
   * @brief Generates and writes one inclusive image-cell subvolume.
   * @tparam T Specifies the target scalar type.
   * @param dataArray Receives initialized values.
   * @param dims Specifies X, Y, and Z cell dimensions.
   * @param xMin Specifies the inclusive X lower bound.
   * @param xMax Specifies the inclusive X upper bound.
   * @param yMin Specifies the inclusive Y lower bound.
   * @param yMax Specifies the inclusive Y upper bound.
   * @param zMin Specifies the inclusive Z lower bound.
   * @param zMax Specifies the inclusive Z upper bound.
   * @param initType Specifies constant or random initialization.
   * @param initValue Specifies the constant initialization value.
   * @param initRange Specifies random lower and upper bounds.
   * @param seed Specifies the random generator seed.
   * @param shouldCancel Stops before later row segments when true.
   * @return Dimension, offset, or bulk-write error, or success after cancellation.
   *
   * The buffer target is not a hard cap when one tuple has more components.
   */
  template <class T>
  Result<> operator()(IDataArray& dataArray, const std::array<usize, 3>& dims, uint64 xMin, uint64 xMax, uint64 yMin, uint64 yMax, uint64 zMin, uint64 zMax, InitType initType, float64 initValue,
                      const RangeType& initRange, uint64 seed, const std::atomic_bool& shouldCancel)
  {
    T rangeMin;
    T rangeMax;
    if(initType == InitType::RandomWithRange)
    {
      rangeMin = static_cast<T>(initRange.first);
      rangeMax = static_cast<T>(initRange.second);
    }
    else
    {
      rangeMin = std::numeric_limits<T>().min();
      rangeMax = std::numeric_limits<T>().max();
    }

    auto& dataStore = dataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();

    auto&& [distribution, generator] = CreateRandomGenerator(rangeMin, rangeMax, seed);
    const usize numComponents = dataStore.getNumberOfComponents();
    if(numComponents == 0)
    {
      return MakeErrorResult(-27490, "InitializeImageGeomCellData cannot initialize an array with zero components.");
    }
    if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0 || dims[1] > std::numeric_limits<usize>::max() / dims[0])
    {
      return MakeErrorResult(-27491, "InitializeImageGeomCellData encountered image dimensions that overflow the data store index type.");
    }
    const usize sliceTupleCount = dims[0] * dims[1];
    const usize tuplesPerChunk = std::max<usize>(1, k_InitializationChunkValues / numComponents);
    auto values = std::make_unique<T[]>(tuplesPerChunk * numComponents);
    const T manualValue = static_cast<T>(initValue);

    for(uint64 k = zMin;; k++)
    {
      for(uint64 j = yMin;; j++)
      {
        for(uint64 i = xMin;;)
        {
          if(shouldCancel)
          {
            return {};
          }

          const usize tupleCount = std::min<usize>(tuplesPerChunk, xMax - i + 1);
          for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
          {
            const T value = initType == InitType::Manual ? manualValue : distribution(generator);
            std::fill_n(values.get() + (tupleIndex * numComponents), numComponents, value);
          }

          if(k > std::numeric_limits<usize>::max() / sliceTupleCount || j > (std::numeric_limits<usize>::max() - (k * sliceTupleCount)) / dims[0] ||
             i > std::numeric_limits<usize>::max() - ((k * sliceTupleCount) + (j * dims[0])))
          {
            return MakeErrorResult(-27491, "InitializeImageGeomCellData encountered image dimensions that overflow the data store index type.");
          }
          const usize tupleIndex = (k * sliceTupleCount) + (j * dims[0]) + i;
          if(tupleIndex > std::numeric_limits<usize>::max() / numComponents)
          {
            return MakeErrorResult(-27492, "InitializeImageGeomCellData encountered an array offset that overflows the data store index type.");
          }
          auto writeResult = dataStore.copyFromBuffer(tupleIndex * numComponents, nonstd::span<const T>(values.get(), tupleCount * numComponents));
          if(writeResult.invalid())
          {
            return writeResult;
          }

          if(tupleCount == xMax - i + 1)
          {
            break;
          }
          i += tupleCount;
        }
        if(j == yMax)
        {
          break;
        }
      }
      if(k == zMax)
      {
        break;
      }
    }

    return {};
  }
};
} // namespace

InitializeImageGeomCellData::InitializeImageGeomCellData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         InitializeImageGeomCellDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

InitializeImageGeomCellData::~InitializeImageGeomCellData() noexcept = default;

Result<> InitializeImageGeomCellData::operator()()
{
  auto cellArrayPaths = m_InputValues->CellArrays;
  auto imageGeomPath = m_InputValues->InputImageGeometryPath;
  auto minPoint = m_InputValues->MinPoint;
  auto maxPoint = m_InputValues->MaxPoint;
  auto initTypeIndex = m_InputValues->InitTypeIndex;
  auto initValue = m_InputValues->InitValue;
  auto initRangeVec = m_InputValues->InitRange;

  auto seed = m_InputValues->SeedValue;
  if(!m_InputValues->UseSeed)
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store the effective seed so random output can be reproduced.
  m_DataStructure.getDataRefAs<UInt64Array>(DataPath({m_InputValues->SeedArrayName}))[0] = seed;

  uint64 xMin = minPoint.at(0);
  uint64 yMin = minPoint.at(1);
  uint64 zMin = minPoint.at(2);

  uint64 xMax = maxPoint.at(0);
  uint64 yMax = maxPoint.at(1);
  uint64 zMax = maxPoint.at(2);

  InitType initType = ConvertIndexToInitType(initTypeIndex);
  RangeType initRange = {initRangeVec.at(0), initRangeVec.at(1)};

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  std::array<usize, 3> dims = imageGeom.getDimensions().toArray();
  // Every selected cell array participates because mixed storage is valid.
  std::vector<const IArray*> arrayTargets;
  arrayTargets.reserve(cellArrayPaths.size());
  for(const DataPath& path : cellArrayPaths)
  {
    arrayTargets.push_back(&m_DataStructure.getDataRefAs<IDataArray>(path));
  }

  const AlgorithmArrayTargets dispatchTargets(std::move(arrayTargets));
  const bool usesOutOfCoreStore = AnyOutOfCore(dispatchTargets);
  const bool useOutOfCorePath = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCorePath ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

  for(const DataPath& path : cellArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto& iDataArray = m_DataStructure.getDataRefAs<IDataArray>(path);

    auto initializeResult = ExecuteNeighborFunction(InitializeArrayFunctor{}, iDataArray.getDataType(), iDataArray, dims, xMin, xMax, yMin, yMax, zMin, zMax, initType, initValue, initRange, seed,
                                                    m_ShouldCancel); // ExecuteNeighborFunction excludes Boolean arrays.
    if(initializeResult.invalid())
    {
      return initializeResult;
    }

    // Advance the seed so separate selected arrays do not repeat a sequence.
    seed++;
  }

  return {};
}
