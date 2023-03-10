#include "FindNeighborhoods.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/MessageUtilities.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <cmath>

using namespace complex;

class FindNeighborhoodsImpl
{
public:
  FindNeighborhoodsImpl(FindNeighborhoods* filter, size_t totalFeatures, const std::vector<int64_t>& bins, const std::vector<float>& criticalDistance, const std::atomic_bool& shouldCancel,
                        ThreadSafeMessenger& messenger)
  : m_Filter(filter)
  , m_TotalFeatures(totalFeatures)
  , m_Bins(bins)
  , m_CriticalDistance(criticalDistance)
  , m_ShouldCancel(shouldCancel)
  , m_Messenger(messenger)
  {
  }

  void convert(size_t start, size_t end) const
  {
    int64_t bin1x = 0, bin2x = 0, bin1y = 0, bin2y = 0, bin1z = 0, bin2z = 0;
    float dBinX = 0, dBinY = 0, dBinZ = 0;
    float criticalDistance1 = 0, criticalDistance2 = 0;

    size_t increment = m_Messenger.getProgressIncrement();
    size_t incCount = 0;
    // NEVER start at 0.
    if(start == 0)
    {
      start = 1;
      incCount++;
    }
    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        break;
      }

      bin1x = m_Bins[3 * i];
      bin1y = m_Bins[3 * i + 1];
      bin1z = m_Bins[3 * i + 2];
      criticalDistance1 = m_CriticalDistance[i];

      for(size_t j = i + 1; j < m_TotalFeatures; j++)
      {
        bin2x = m_Bins[3 * j];
        bin2y = m_Bins[3 * j + 1];
        bin2z = m_Bins[3 * j + 2];
        criticalDistance2 = m_CriticalDistance[j];

        dBinX = std::abs(bin2x - bin1x);
        dBinY = std::abs(bin2y - bin1y);
        dBinZ = std::abs(bin2z - bin1z);

        if(dBinX < criticalDistance1 && dBinY < criticalDistance1 && dBinZ < criticalDistance1)
        {
          m_Filter->updateNeighborHood(i, j);
        }

        if(dBinX < criticalDistance2 && dBinY < criticalDistance2 && dBinZ < criticalDistance2)
        {
          m_Filter->updateNeighborHood(j, i);
        }
      }

      incCount++;
      if(incCount > increment)
      {
        m_Messenger.updateProgress(incCount);
        incCount = 0;
      }
    }
    m_Messenger.updateProgress(incCount);
  }

  void operator()(const Range& range) const
  {
    convert(range[0], range[1]);
  }

private:
  FindNeighborhoods* m_Filter = nullptr;
  size_t m_TotalFeatures = 0;
  const std::vector<int64_t>& m_Bins;
  const std::vector<float>& m_CriticalDistance;
  const std::atomic_bool& m_ShouldCancel;
  ThreadSafeMessenger& m_Messenger;
};

// -----------------------------------------------------------------------------
FindNeighborhoods::FindNeighborhoods(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindNeighborhoodsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindNeighborhoods::~FindNeighborhoods() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindNeighborhoods::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void FindNeighborhoods::updateNeighborHood(size_t sourceIndex, size_t destIndex)
{
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);
  (*m_Neighborhoods)[sourceIndex]++;
  m_LocalNeighborhoodList[sourceIndex].push_back(static_cast<int32_t>(destIndex));
}

// -----------------------------------------------------------------------------
Result<> FindNeighborhoods::operator()()
{
  float x = 0.0f, y = 0.0f, z = 0.0f;
  std::vector<float> criticalDistance;

  auto multiplesOfAverage = m_InputValues->MultiplesOfAverage;
  const auto& equivalentDiameters = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->EquivalentDiametersArrayPath);
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);

  m_Neighborhoods = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NeighborhoodsArrayName);

  size_t totalFeatures = equivalentDiameters.getNumberOfTuples();

  m_LocalNeighborhoodList.resize(totalFeatures);
  criticalDistance.resize(totalFeatures);

  float aveDiam = 0.0f;
  for(size_t i = 1; i < totalFeatures; i++)
  {
    (*m_Neighborhoods)[i] = 0;
    aveDiam += equivalentDiameters[i];
    criticalDistance[i] = equivalentDiameters[i] * multiplesOfAverage;
  }
  aveDiam /= totalFeatures;
  for(size_t i = 1; i < totalFeatures; i++)
  {
    criticalDistance[i] /= aveDiam;
  }

  auto* gridGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometry);

  FloatVec3 origin = gridGeom->getOrigin();

  size_t xbin = 0, ybin = 0, zbin = 0;
  std::vector<int64_t> bins(3 * totalFeatures, 0);
  for(size_t i = 1; i < totalFeatures; i++)
  {
    x = centroids[3 * i];
    y = centroids[3 * i + 1];
    z = centroids[3 * i + 2];
    xbin = static_cast<size_t>((x - origin[0]) / aveDiam);
    ybin = static_cast<size_t>((y - origin[1]) / aveDiam);
    zbin = static_cast<size_t>((z - origin[2]) / aveDiam);
    bins[3 * i] = static_cast<int64_t>(xbin);
    bins[3 * i + 1] = static_cast<int64_t>(ybin);
    bins[3 * i + 2] = static_cast<int64_t>(zbin);
  }

  ThreadSafeMessenger messenger(m_MessageHandler, "Finding Neighborhoods...");
  messenger.setProgressIncrement(totalFeatures / 100);
  messenger.setTotalElements(totalFeatures);

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(Range(0, totalFeatures));
  parallelAlgorithm.execute(FindNeighborhoodsImpl(this, totalFeatures, bins, criticalDistance, getCancel(), messenger));

  // Output Variables
  auto& outputNeighborList = m_DataStructure.getDataRefAs<NeighborList<int32_t>>(m_InputValues->NeighborhoodListArrayName);
  // Set the vector for each list into the NeighborList Object
  for(size_t i = 1; i < totalFeatures; i++)
  {
    // Construct a shared vector<int32> through the std::vector<> copy constructor.
    NeighborList<int32>::SharedVectorType sharedMisorientationList(new std::vector<int32>(m_LocalNeighborhoodList[i]));
    outputNeighborList.setList(static_cast<int32_t>(i), sharedMisorientationList);
  }

  m_LocalNeighborhoodList.clear();

  return {};
}
