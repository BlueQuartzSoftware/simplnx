#include "KMedoids.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/IFilter.hpp"

#include <random>

using namespace complex;

namespace
{
template <typename T>
class KMedoidsTemplate
{
public:
  using Self = KMedoidsTemplate;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer()
  {
    return Pointer(static_cast<Self*>(nullptr));
  }

  /**
   * @brief Returns the name of the class for KMedoidsTemplate
   */
  /**
   * @brief Returns the name of the class for KMedoidsTemplate
   */
  std::string getNameOfClass() const
  {
    return "KMedoidsTemplate";
  }

  /**
   * @brief Returns the name of the class for KMedoidsTemplate
   */
  std::string ClassName()
  {
    return "KMedoidsTemplate";
  }

  KMedoidsTemplate() = default;
  virtual ~KMedoidsTemplate() = default;

  KMedoidsTemplate(const KMedoidsTemplate&); // Copy Constructor Not Implemented
  void operator=(const KMedoidsTemplate&);   // Move assignment Not Implemented

  // -----------------------------------------------------------------------------
  bool operator()(IDataArray& p)
  {
    return (std::dynamic_pointer_cast<DataArray<T>&>(p));
  }

  // -----------------------------------------------------------------------------
  void Execute(KMedoids* filter, IDataArray& inputIDataArray, IDataArray& outputIDataArray, BoolArray& maskDataArray, usize numClusters,
               Int32Array& fIds, int32 distMetric)
  {
    typename DataArray<T>::Pointer inputDataPtr = std::dynamic_pointer_cast<DataArray<T>>(inputIDataArray);
    typename DataArray<T>::Pointer outputDataPtr = std::dynamic_pointer_cast<DataArray<T>>(outputIDataArray);
    T* inputData = inputDataPtr->getPointer(0);
    T* outputData = outputDataPtr->getPointer(0);

    usize numTuples = inputDataPtr->getNumberOfTuples();
    int32 numCompDims = inputDataPtr->getNumberOfComponents();

    usize rangeMin = 0;
    usize rangeMax = numTuples - 1;
    std::mt19937_64::result_type seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<usize> dist(rangeMin, rangeMax);

    std::vector<usize> clusterIdxs(numClusters);
    bool* mask = maskDataArray->getPointer(0);
    usize clusterChoices = 0;

    while(clusterChoices < numClusters)
    {
      usize index = dist(gen);
      if(mask[index])
      {
        clusterIdxs[clusterChoices] = index;
        clusterChoices++;
      }
    }

    for(usize i = 0; i < numClusters; i++)
    {
      for(int32_t j = 0; j < numCompDims; j++)
      {
        outputData[numCompDims * (i + 1) + j] = inputData[numCompDims * clusterIdxs[i] + j];
      }
    }

    // usize updateCheck = 0;
    int32_t* fPtr = fIds->getPointer(0);

    findClusters(filter, mask, inputData, outputData, fPtr, numTuples, numClusters, numCompDims, distMetric);

    std::vector<usize> optClusterIdxs(clusterIdxs);
    std::vector<double> costs;

    costs = optimizeClusters(filter, mask, inputData, outputData, fPtr, numTuples, numClusters, numCompDims, clusterIdxs, distMetric);

    bool update = optClusterIdxs == clusterIdxs ? false : true;
    usize iteration = 1;

    while(update)
    {
      findClusters(filter, mask, inputData, outputData, fPtr, numTuples, numClusters, numCompDims, distMetric);

      optClusterIdxs = clusterIdxs;

      costs = optimizeClusters(filter, mask, inputData, outputData, fPtr, numTuples, numClusters, numCompDims, clusterIdxs, distMetric);

      update = optClusterIdxs == clusterIdxs ? false : true;

      double sum = std::accumulate(std::begin(costs), std::end(costs), 0.0);
      QString ss = QObject::tr("Clustering Data || Iteration %1 || Total Cost: %2").arg(iteration).arg(sum);
      filter->notifyStatusMessage(ss);
      iteration++;
    }
  }

private:
  // -----------------------------------------------------------------------------
  void findClusters(KMedoids* filter, bool* mask, T* input, T* medoids, int32_t* fIds, usize tuples, int32_t clusters, int32_t dims, int32_t distMetric)
  {
    double dist = 0.0;

    for(usize i = 0; i < tuples; i++)
    {
      if(filter->getCancel())
      {
        return;
      }
      if(mask[i])
      {
        double minDist = std::numeric_limits<double>::max();
        for(usize j = 0; j < clusters; j++)
        {
          dist = DistanceTemplate::GetDistance<T, T, double>(input + (dims * i), medoids + (dims * (j + 1)), dims, distMetric);
          if(dist < minDist)
          {
            minDist = dist;
            fIds[i] = j + 1;
          }
        }
      }
    }
  }

  // -----------------------------------------------------------------------------
  std::vector<double> optimizeClusters(KMedoids* filter, bool* mask, T* input, T* medoids, int32_t* fIds, usize tuples, int32_t clusters, int32_t dims, std::vector<usize>& clusterIdxs,
                                       int32_t distMetric)
  {
    double dist = 0.0;
    std::vector<double> minCosts(clusters, std::numeric_limits<double>::max());

    for(usize i = 0; i < clusters; i++)
    {
      if(filter->getCancel())
      {
        return std::vector<double>();
      }
      for(usize j = 0; j < tuples; j++)
      {
        if(filter->getCancel())
        {
          return std::vector<double>();
        }
        if(mask[j])
        {
          double cost = 0.0;
          if(fIds[j] == i + 1)
          {
            for(usize k = 0; k < tuples; k++)
            {
              if(filter->getCancel())
              {
                return std::vector<double>();
              }
              if(fIds[k] == i + 1 && mask[k])
              {
                dist = DistanceTemplate::GetDistance<T, T, double>(input + (dims * k), input + (dims * j), dims, distMetric);
                cost += dist;
              }
            }

            if(cost < minCosts[i])
            {
              minCosts[i] = cost;
              clusterIdxs[i] = j;
            }
          }
        }
      }
    }

    for(usize i = 0; i < clusters; i++)
    {
      for(int32_t j = 0; j < dims; j++)
      {
        medoids[dims * (i + 1) + j] = input[dims * clusterIdxs[i] + j];
      }
    }

    return minCosts;
  }
};
}

// -----------------------------------------------------------------------------
KMedoids::KMedoids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMedoidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
KMedoids::~KMedoids() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& KMedoids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> KMedoids::operator()()
{
  if(m_InputValues->UseMask)
  {
    EXECUTE_TEMPLATE(this, KMedoidsTemplate, m_InDataPtr.lock(), this, m_InDataPtr.lock(), m_MedoidsArrayPtr.lock(), m_MaskPtr.lock(), m_InitClusters, m_FeatureIdsPtr.lock(), m_DistanceMetric);
  }
  else
  {
    usize numTuples = m_DataStructure.getDataAs<Int32Array>(m_InputValues->MedoidsArrayName)->getNumberOfTuples();
    std::vector<bool> tmpMask(numTuples, true);
    EXECUTE_TEMPLATE(this, KMedoidsTemplate, m_InDataPtr.lock(), this, m_InDataPtr.lock(), m_MedoidsArrayPtr.lock(), tmpMask, m_InitClusters, m_FeatureIdsPtr.lock(), m_DistanceMetric);
  }

  return {};
}
