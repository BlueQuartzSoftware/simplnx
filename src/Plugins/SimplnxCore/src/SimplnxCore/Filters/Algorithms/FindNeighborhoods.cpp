#include "FindNeighborhoods.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <cmath>

using namespace nx::core;

class FindNeighborhoodsImpl
{
public:
  FindNeighborhoodsImpl(FindNeighborhoods* filter, usize totalFeatures, const std::vector<int64_t>& bins, const std::vector<float>& criticalDistance, const std::atomic_bool& shouldCancel)
  : m_Filter(filter)
  , m_TotalFeatures(totalFeatures)
  , m_Bins(bins)
  , m_CriticalDistance(criticalDistance)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void convert(usize start, usize end) const
  {
    int64 bin1x, bin2x, bin1y, bin2y, bin1z, bin2z;
    float32 dBinX, dBinY, dBinZ;
    float32 criticalDistance1, criticalDistance2;

    auto increment = static_cast<float64>(end - start) / 100.0;
    float64 incCount = 0.0;
    // NEVER start at 0.
    if(start == 0)
    {
      start = 1;
    }

    auto startTime = std::chrono::steady_clock::now();
    for(usize featureIdx = start; featureIdx < end; featureIdx++)
    {
      incCount++;
      if(incCount >= increment)
      {
        auto now = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() > 1000)
        {
          incCount = 0;
          m_Filter->updateProgress(incCount, now);
          startTime = now;
        }
      }

      if(m_ShouldCancel)
      {
        return;
      }

      bin1x = m_Bins[3 * featureIdx];
      bin1y = m_Bins[3 * featureIdx + 1];
      bin1z = m_Bins[3 * featureIdx + 2];
      criticalDistance1 = m_CriticalDistance[featureIdx];

      for(usize j = featureIdx + 1; j < m_TotalFeatures; j++)
      {
        bin2x = m_Bins[3 * j];
        bin2y = m_Bins[3 * j + 1];
        bin2z = m_Bins[3 * j + 2];
        criticalDistance2 = m_CriticalDistance[j];

        dBinX = std::abs(static_cast<float32>(bin2x - bin1x));
        dBinY = std::abs(static_cast<float32>(bin2y - bin1y));
        dBinZ = std::abs(static_cast<float32>(bin2z - bin1z));

        if(dBinX < criticalDistance1 && dBinY < criticalDistance1 && dBinZ < criticalDistance1)
        {
          m_Filter->updateNeighborHood(featureIdx, j);
        }

        if(dBinX < criticalDistance2 && dBinY < criticalDistance2 && dBinZ < criticalDistance2)
        {
          m_Filter->updateNeighborHood(j, featureIdx);
        }
      }
    }
    m_Filter->updateProgress(incCount);
  }

  void operator()(const Range& range) const
  {
    convert(range[0], range[1]);
  }

private:
  FindNeighborhoods* m_Filter = nullptr;
  usize m_TotalFeatures = 0;
  const std::vector<int64>& m_Bins;
  const std::vector<float32>& m_CriticalDistance;
  const std::atomic_bool& m_ShouldCancel;
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
void FindNeighborhoods::updateNeighborHood(usize sourceIndex, usize destIndex)
{
  const std::lock_guard<std::mutex> lock(m_Mutex);
  (*m_Neighborhoods)[sourceIndex]++;
  m_LocalNeighborhoodList[sourceIndex].push_back(static_cast<int32_t>(destIndex));
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindNeighborhoods::updateProgress(float64 counter, const std::chrono::steady_clock::time_point& now)
{
  const std::lock_guard<std::mutex> lock(m_Mutex);

  m_ProgressCounter += counter;

  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > 1000) // every second update
  {
    auto progressInt = static_cast<int32>((m_ProgressCounter / m_TotalFeatures) * 100.0);
    std::string progressMessage = "Finding Feature Neighborhoods:";
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, progressInt});
    m_InitialTime = std::chrono::steady_clock::now();
  }
}

// -----------------------------------------------------------------------------
Result<> FindNeighborhoods::operator()()
{
  // m_ProgressCounter initialized to zero on filter creation

  std::vector<float32> criticalDistance;

  auto multiplesOfAverage = m_InputValues->MultiplesOfAverage;
  const auto& equivalentDiameters = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->EquivalentDiametersArrayPath);
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);

  m_Neighborhoods = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NeighborhoodsArrayName);

  usize totalFeatures = equivalentDiameters.getNumberOfTuples();
  m_TotalFeatures = static_cast<float64>(totalFeatures); // Pre-cast to save time in lock later

  m_LocalNeighborhoodList.resize(totalFeatures);
  criticalDistance.resize(totalFeatures);

  float32 aveDiam = 0.0f;
  for(usize i = 1; i < totalFeatures; i++)
  {
    (*m_Neighborhoods)[i] = 0;
    aveDiam += equivalentDiameters[i];
    criticalDistance[i] = equivalentDiameters[i] * multiplesOfAverage;
  }
  aveDiam /= static_cast<float32>(totalFeatures);
  for(usize i = 1; i < totalFeatures; i++)
  {
    criticalDistance[i] /= aveDiam;
  }

  std::vector<int64> bins(3 * totalFeatures, 0);
  FloatVec3 origin = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometry)->getOrigin();
  for(usize i = 1; i < totalFeatures; i++)
  {
    float32 x = centroids[3 * i];
    float32 y = centroids[3 * i + 1];
    float32 z = centroids[3 * i + 2];
    bins[3 * i] = static_cast<int64>((x - origin[0]) / aveDiam);     // x-Bin
    bins[3 * i + 1] = static_cast<int64>((y - origin[1]) / aveDiam); // y-Bin
    bins[3 * i + 2] = static_cast<int64>((z - origin[2]) / aveDiam); // z-Bin
  }

  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(Range(0, totalFeatures));
  parallelAlgorithm.setParallelizationEnabled(true);
  parallelAlgorithm.execute(FindNeighborhoodsImpl(this, totalFeatures, bins, criticalDistance, m_ShouldCancel));

  // Output Variables
  auto& outputNeighborList = m_DataStructure.getDataRefAs<NeighborList<int32_t>>(m_InputValues->NeighborhoodListArrayName);
  // Set the vector for each list into the NeighborList Object
  for(usize i = 1; i < totalFeatures; i++)
  {
    // Construct a shared vector<int32> through the std::vector<> copy constructor.
    NeighborList<int32>::SharedVectorType sharedMisOrientationList(new std::vector<int32>(m_LocalNeighborhoodList[i]));
    outputNeighborList.setList(static_cast<int32>(i), sharedMisOrientationList);
  }

  m_LocalNeighborhoodList.clear();

  return {};
}
