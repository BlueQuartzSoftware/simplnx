#include "InitializeImageGeomCellData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <chrono>
#include <limits>
#include <random>

using namespace nx::core;

namespace
{
using RangeType = std::pair<float64, float64>;

enum class InitType : uint64
{
  Manual = 0,
  Random = 1,
  RandomWithRange = 2
};

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

template <class T>
auto CreateRandomGenerator(T rangeMin, T rangeMax, uint64 seed)
{
  std::random_device randomDevice;           // Will be used to obtain a seed for the random number engine
  std::mt19937_64 generator(randomDevice()); // Standard mersenne_twister_engine seeded with rd()
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

struct InitializeArrayFunctor
{
  template <class T>
  void operator()(IDataArray& dataArray, const std::array<usize, 3>& dims, uint64 xMin, uint64 xMax, uint64 yMin, uint64 yMax, uint64 zMin, uint64 zMax, InitType initType, float64 initValue,
                  const RangeType& initRange, uint64 seed)
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

    for(uint64 k = zMin; k < zMax + 1; k++)
    {
      for(uint64 j = yMin; j < yMax + 1; j++)
      {
        for(uint64 i = xMin; i < xMax + 1; i++)
        {
          usize index = (k * dims[0] * dims[1]) + (j * dims[0]) + i;

          if(initType == InitType::Manual)
          {
            T num = static_cast<T>(initValue);
            dataStore.fillTuple(index, num);
          }
          else
          {
            T randNum = distribution(generator);
            dataStore.fillTuple(index, randNum);
          }
        }
      }
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
InitializeImageGeomCellData::InitializeImageGeomCellData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         InitializeImageGeomCellDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
InitializeImageGeomCellData::~InitializeImageGeomCellData() noexcept = default;

// -----------------------------------------------------------------------------
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

  // Store Seed Value in Top Level Array
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

  for(const DataPath& path : cellArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto& iDataArray = m_DataStructure.getDataRefAs<IDataArray>(path);

    ExecuteNeighborFunction(InitializeArrayFunctor{}, iDataArray.getDataType(), iDataArray, dims, xMin, xMax, yMin, yMax, zMin, zMax, initType, initValue, initRange, seed); // NO BOOL

    // Avoid the exact same seeding for each array
    seed++;
  }

  return {};
}
