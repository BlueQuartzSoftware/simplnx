#include "DBSCAN.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
template <typename T>
class FindEpsilonNeighborhoodsImpl
{
private:
  using AbstractDataStoreT = AbstractDataStore<T>;

public:
  FindEpsilonNeighborhoodsImpl(DBSCAN* filter, float64 epsilon, const AbstractDataStoreT& inputData, const std::unique_ptr<MaskCompare>& mask, usize numCompDims, usize numTuples,
                               ClusterUtilities::DistanceMetric distMetric, std::vector<std::list<usize>>& neighborhoods)
  : m_Filter(filter)
  , m_Epsilon(epsilon)
  , m_InputDataStore(inputData)
  , m_Mask(mask)
  , m_NumCompDims(numCompDims)
  , m_NumTuples(numTuples)
  , m_DistMetric(distMetric)
  , m_Neighborhoods(neighborhoods)
  {
  }

  void compute(usize start, usize end) const
  {
    // int64 counter = 0;
    // int64 totalElements = end - start;
    // int64 progIncrement = static_cast<int64>(totalElements / 100);

    for(usize i = start; i < end; i++)
    {
      if(m_Mask->isTrue(i))
      {
        std::list<usize> neighbors = epsilon_neighbors(i);
        m_Neighborhoods[i] = neighbors;
      }
    }
  }

  std::list<usize> epsilon_neighbors(usize index) const
  {
    std::list<usize> neighbors;

    for(usize i = 0; i < m_NumTuples; i++)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }
      if(m_Mask->isTrue(i))
      {
        float64 dist = ClusterUtilities::GetDistance(m_InputDataStore, (m_NumCompDims * index), m_InputDataStore, (m_NumCompDims * i), m_NumCompDims, m_DistMetric);
        if(dist < m_Epsilon)
        {
          neighbors.push_back(i);
        }
      }
    }

    return neighbors;
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  DBSCAN* m_Filter;
  float64 m_Epsilon;
  const AbstractDataStoreT& m_InputDataStore;
  const std::unique_ptr<MaskCompare>& m_Mask;
  usize m_NumCompDims;
  usize m_NumTuples;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::vector<std::list<usize>>& m_Neighborhoods;
};

template <typename T>
class DBSCANTemplate
{
private:
  using DataArrayT = DataArray<T>;
  using AbstractDataStoreT = AbstractDataStore<T>;

public:
  DBSCANTemplate(DBSCAN* filter, const IDataArray& inputIDataArray, const std::unique_ptr<MaskCompare>& maskDataArray, Int32Array& fIds, float32 epsilon, int32 minPoints,
                 ClusterUtilities::DistanceMetric distMetric)
  : m_Filter(filter)
  , m_InputDataStore(dynamic_cast<const DataArrayT&>(inputIDataArray).getDataStoreRef())
  , m_Mask(maskDataArray)
  , m_FeatureIds(fIds.getDataStoreRef())
  , m_Epsilon(epsilon)
  , m_MinPoints(minPoints)
  , m_DistMetric(distMetric)
  {
  }
  ~DBSCANTemplate() = default;

  DBSCANTemplate(const DBSCANTemplate&) = delete; // Copy Constructor Not Implemented
  void operator=(const DBSCANTemplate&) = delete; // Move assignment Not Implemented

  // -----------------------------------------------------------------------------
  void operator()()
  {
    usize numTuples = m_InputDataStore.getNumberOfTuples();
    usize numCompDims = m_InputDataStore.getNumberOfComponents();
    std::vector<bool> visited(numTuples, false);
    std::vector<bool> clustered(numTuples, false);

    auto minDist = static_cast<float64>(m_Epsilon);
    int32 cluster = 0;

    auto progIncrement = static_cast<int64>(numTuples / 100);
    int64 prog = 1;
    int64 progressInt = 0;
    int64 counter = 0;

    std::vector<std::list<usize>> epsilonNeighborhoods(numTuples);

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0ULL, numTuples);
    dataAlg.execute(FindEpsilonNeighborhoodsImpl<T>(m_Filter, minDist, m_InputDataStore, m_Mask, numCompDims, numTuples, m_DistMetric, epsilonNeighborhoods));

    prog = 1;
    progressInt = 0;
    counter = 0;

    for(usize i = 0; i < numTuples; i++)
    {
      if(m_Filter->getCancel())
      {
        return;
      }

      if(m_Mask->isTrue(i) && !visited[i])
      {
        visited[i] = true;

        if(counter > prog)
        {
          progressInt = static_cast<int64>((static_cast<float>(counter) / numTuples) * 100.0f);
          std::string ss = fmt::format("Scanning Data || Visited Point {} of {} || {}% Completed", counter, numTuples, progressInt);
          m_Filter->updateProgress(ss);
          prog = prog + progIncrement;
        }
        counter++;

        std::list<usize> neighbors = epsilonNeighborhoods[i];

        if(static_cast<int32>(neighbors.size()) < m_MinPoints)
        {
          m_FeatureIds[i] = 0;
          clustered[i] = true;
        }
        else
        {
          cluster++;
          m_FeatureIds[i] = cluster;
          clustered[i] = true;

          for(auto&& idx : neighbors)
          {
            if(m_Filter->getCancel())
            {
              return;
            }
            if(m_Mask->isTrue(idx))
            {
              if(!visited[idx])
              {
                visited[idx] = true;

                if(counter > prog)
                {
                  progressInt = static_cast<int64>((static_cast<float>(counter) / numTuples) * 100.0f);
                  std::string ss = fmt::format("Scanning Data || Visited Point {} of {} || {}% Completed", counter, numTuples, progressInt);
                  m_Filter->updateProgress(ss);
                  prog = prog + progIncrement;
                }
                counter++;

                std::list<usize> neighbors_prime = epsilonNeighborhoods[idx];
                if(static_cast<int32>(neighbors_prime.size()) >= m_MinPoints)
                {
                  neighbors.splice(std::end(neighbors), neighbors_prime);
                }
              }
              if(!clustered[idx])
              {
                m_FeatureIds[idx] = cluster;
                clustered[idx] = true;
              }
            }
          }
        }
      }
    }
  }

private:
  DBSCAN* m_Filter;
  const AbstractDataStoreT& m_InputDataStore;
  const std::unique_ptr<MaskCompare>& m_Mask;
  AbstractDataStore<int32>& m_FeatureIds;
  float32 m_Epsilon;
  int32 m_MinPoints;
  ClusterUtilities::DistanceMetric m_DistMetric;
};
} // namespace

// -----------------------------------------------------------------------------
DBSCAN::DBSCAN(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DBSCANInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
DBSCAN::~DBSCAN() noexcept = default;

// -----------------------------------------------------------------------------
void DBSCAN::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& DBSCAN::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> DBSCAN::operator()()
{
  auto& clusteringArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  std::unique_ptr<MaskCompare> maskCompare;
  try
  {
    maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-54060, message);
  }

  RunTemplateClass<DBSCANTemplate, types::NoBooleanType>(clusteringArray.getDataType(), this, clusteringArray, maskCompare, featureIds, m_InputValues->Epsilon, m_InputValues->MinPoints,
                                                         m_InputValues->DistanceMetric);

  auto& featureIdsDataStore = featureIds.getDataStoreRef();
  int32 maxCluster = *std::max_element(featureIdsDataStore.begin(), featureIdsDataStore.end());
  m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->FeatureAM)->resizeTuples(AttributeMatrix::ShapeType{static_cast<usize>(maxCluster + 1)});

  return {};
}
