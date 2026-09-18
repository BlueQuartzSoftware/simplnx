#include "AddBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <random>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 65536;

/**
 * @brief Selects tuple indexes for one page with deterministic random draws.
 * @param distances Supplies boundary-distance values.
 * @param tupleCount Number of tuples in the page.
 * @param inputValues Defines enabled noise modes and fractions.
 * @param generator Supplies seeded random values.
 * @param distribution Produces values in [0, 1).
 * @param mutationIndices Receives page-local tuple indexes.
 * @return Number of selected tuple indexes.
 *
 * Poisson noise consumes its independent draw even when boundary noise already
 * selected the tuple. This preserves direct and scanline random sequences.
 */
usize GenerateMutationIndices(const int32* distances, usize tupleCount, const AddBadDataInputValues& inputValues, std::mt19937& generator, std::uniform_real_distribution<float32>& distribution,
                              usize* mutationIndices)
{
  usize mutationCount = 0;
  for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
  {
    bool shouldMutate = false;
    if(inputValues.BoundaryNoise && distances[tupleIndex] < 1)
    {
      shouldMutate = distribution(generator) < inputValues.BoundaryVolFraction;
    }

    // Keep the independent Poisson draw even when boundary noise already selected the tuple.
    if(inputValues.PoissonNoise && distribution(generator) < inputValues.PoissonVolFraction)
    {
      shouldMutate = true;
    }

    if(shouldMutate)
    {
      mutationIndices[mutationCount++] = tupleIndex;
    }
  }
  return mutationCount;
}

/**
 * @struct ZeroTuplesDirectFunctor
 * @brief Zeros selected tuples through a concrete contiguous DataStore.
 */
struct ZeroTuplesDirectFunctor
{
  template <typename T>
  Result<> operator()(IDataArray& array, usize tupleOffset, nonstd::span<const usize> mutationIndices) const
  {
    auto& dataStore = array.template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& contiguousStore = dynamic_cast<DataStore<T>&>(dataStore);
    T* values = contiguousStore.data();
    const usize numComponents = dataStore.getNumberOfComponents();
    for(const usize localTupleIndex : mutationIndices)
    {
      std::fill_n(values + (tupleOffset + localTupleIndex) * numComponents, numComponents, T{});
    }
    return {};
  }
};

/**
 * @struct ZeroTuplesScanlineFunctor
 * @brief Reads, mutates, and writes one selected tuple page.
 *
 * Page bytes scale with tuple count, component count, and element size.
 */
struct ZeroTuplesScanlineFunctor
{
  template <typename T>
  Result<> operator()(IDataArray& array, usize tupleOffset, usize tupleCount, nonstd::span<const usize> mutationIndices) const
  {
    auto& dataStore = array.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize numComponents = dataStore.getNumberOfComponents();
    const usize valueOffset = tupleOffset * numComponents;
    const usize valueCount = tupleCount * numComponents;
    auto values = std::make_unique<T[]>(valueCount);

    Result<> readResult = dataStore.copyIntoBuffer(valueOffset, nonstd::span<T>(values.get(), valueCount));
    if(readResult.invalid())
    {
      return readResult;
    }

    for(const usize localTupleIndex : mutationIndices)
    {
      std::fill_n(values.get() + localTupleIndex * numComponents, numComponents, T{});
    }

    return dataStore.copyFromBuffer(valueOffset, nonstd::span<const T>(values.get(), valueCount));
  }
};

/**
 * @class AddBadDataScanline
 * @brief Streams distance and child-array chunks so disk-backed stores never see per-tuple I/O.
 */
class AddBadDataScanline
{
public:
  AddBadDataScanline(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const AddBadDataInputValues* inputValues, const std::vector<DataPath>& voxelArrayPaths)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  , m_VoxelArrayPaths(voxelArrayPaths)
  {
  }

  Result<> operator()()
  {
    auto& distancesArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->GBEuclideanDistancesArrayPath);
    auto& distancesStore = distancesArray.getDataStoreRef();
    const usize totalPoints = distancesStore.getSize();
    if(totalPoints == 0)
    {
      return {};
    }

    auto distances = std::make_unique<int32[]>(k_ChunkTuples);
    auto mutationIndices = std::make_unique<usize[]>(k_ChunkTuples);
    std::mt19937 generator(m_InputValues->SeedValue);
    std::uniform_real_distribution<float32> distribution(0.0F, 1.0F);

    for(usize tupleOffset = 0; tupleOffset < totalPoints; tupleOffset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize tupleCount = std::min(k_ChunkTuples, totalPoints - tupleOffset);
      Result<> distanceReadResult = distancesStore.copyIntoBuffer(tupleOffset, nonstd::span<int32>(distances.get(), tupleCount));
      if(distanceReadResult.invalid())
      {
        return distanceReadResult;
      }

      const usize mutationCount = GenerateMutationIndices(distances.get(), tupleCount, *m_InputValues, generator, distribution, mutationIndices.get());
      if(mutationCount != 0)
      {
        const nonstd::span<const usize> mutations(mutationIndices.get(), mutationCount);
        for(const DataPath& voxelArrayPath : m_VoxelArrayPaths)
        {
          Result<> mutationResult;
          if(voxelArrayPath == m_InputValues->GBEuclideanDistancesArrayPath && distancesStore.getNumberOfComponents() == 1)
          {
            for(const usize localTupleIndex : mutations)
            {
              distances[localTupleIndex] = 0;
            }
            mutationResult = distancesStore.copyFromBuffer(tupleOffset, nonstd::span<const int32>(distances.get(), tupleCount));
          }
          else
          {
            auto& voxelArray = m_DataStructure.getDataRefAs<IDataArray>(voxelArrayPath);
            mutationResult = ExecuteDataFunction(ZeroTuplesScanlineFunctor{}, voxelArray.getDataType(), voxelArray, tupleOffset, tupleCount, mutations);
          }

          if(mutationResult.invalid())
          {
            return mutationResult;
          }
        }
      }
    }

    return {};
  }

private:
  DataStructure& m_DataStructure;
  const AddBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const std::vector<DataPath>& m_VoxelArrayPaths;
};

/**
 * @class AddBadDataDirect
 * @brief Uses contiguous in-memory stores and only visits tuples selected by the seeded draws.
 */
class AddBadDataDirect
{
public:
  AddBadDataDirect(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const AddBadDataInputValues* inputValues, const std::vector<DataPath>& voxelArrayPaths)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  , m_VoxelArrayPaths(voxelArrayPaths)
  {
  }

  Result<> operator()()
  {
    auto& distancesArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->GBEuclideanDistancesArrayPath);
    auto& distancesStore = distancesArray.getDataStoreRef();
    auto* contiguousDistancesStore = dynamic_cast<Int32DataStore*>(&distancesStore);
    if(contiguousDistancesStore == nullptr)
    {
      return AddBadDataScanline(m_DataStructure, m_ShouldCancel, m_InputValues, m_VoxelArrayPaths)();
    }

    for(const DataPath& voxelArrayPath : m_VoxelArrayPaths)
    {
      if(m_DataStructure.getDataRefAs<IDataArray>(voxelArrayPath).getIDataStoreRef().getStoreType() != IDataStore::StoreType::InMemory)
      {
        return AddBadDataScanline(m_DataStructure, m_ShouldCancel, m_InputValues, m_VoxelArrayPaths)();
      }
    }

    const usize totalPoints = distancesStore.getSize();
    if(totalPoints == 0)
    {
      return {};
    }

    int32* distances = contiguousDistancesStore->data();
    auto mutationIndices = std::make_unique<usize[]>(k_ChunkTuples);
    std::mt19937 generator(m_InputValues->SeedValue);
    std::uniform_real_distribution<float32> distribution(0.0F, 1.0F);

    for(usize tupleOffset = 0; tupleOffset < totalPoints; tupleOffset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize tupleCount = std::min(k_ChunkTuples, totalPoints - tupleOffset);
      const usize mutationCount = GenerateMutationIndices(distances + tupleOffset, tupleCount, *m_InputValues, generator, distribution, mutationIndices.get());
      if(mutationCount != 0)
      {
        const nonstd::span<const usize> mutations(mutationIndices.get(), mutationCount);
        for(const DataPath& voxelArrayPath : m_VoxelArrayPaths)
        {
          auto& voxelArray = m_DataStructure.getDataRefAs<IDataArray>(voxelArrayPath);
          Result<> mutationResult = ExecuteDataFunction(ZeroTuplesDirectFunctor{}, voxelArray.getDataType(), voxelArray, tupleOffset, mutations);
          if(mutationResult.invalid())
          {
            return mutationResult;
          }
        }
      }
    }

    return {};
  }

private:
  DataStructure& m_DataStructure;
  const AddBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const std::vector<DataPath>& m_VoxelArrayPaths;
};
} // namespace

AddBadData::AddBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AddBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

AddBadData::~AddBadData() noexcept = default;

const std::atomic_bool& AddBadData::getCancel()
{
  return m_ShouldCancel;
}

Result<> AddBadData::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  auto childArrayPaths = GetAllChildArrayDataPaths(m_DataStructure, imageGeom.getCellDataPath());
  const std::vector<DataPath> voxelArrayPaths = childArrayPaths.has_value() ? std::move(childArrayPaths.value()) : std::vector<DataPath>{};

  const auto& distancesArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->GBEuclideanDistancesArrayPath);
  const IDataArray* oocChildArray = nullptr;
  for(const DataPath& voxelArrayPath : voxelArrayPaths)
  {
    const auto& voxelArray = m_DataStructure.getDataRefAs<IDataArray>(voxelArrayPath);
    if(IsOutOfCore(voxelArray))
    {
      oocChildArray = &voxelArray;
      break;
    }
  }

  return DispatchAlgorithm<AddBadDataDirect, AddBadDataScanline>({&distancesArray, oocChildArray}, m_DataStructure, m_ShouldCancel, m_InputValues, voxelArrayPaths);
}
