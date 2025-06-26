#include "ComputeNeighborhoods.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <cmath>

using namespace nx::core;
namespace
{
class ComputeNeighborhoodsImpl
{
public:
  ComputeNeighborhoodsImpl(ComputeNeighborhoods* filter, usize totalFeatures, const std::vector<int64_t>& bins, const std::vector<float>& criticalDistance, const std::atomic_bool& shouldCancel,
                           ProgressMessageHelper& progressMessageHelper)
  : m_Filter(filter)
  , m_TotalFeatures(totalFeatures)
  , m_Bins(bins)
  , m_CriticalDistance(criticalDistance)
  , m_ShouldCancel(shouldCancel)
  , m_ProgressMessageHelper(progressMessageHelper)
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

    ProgressMessenger progressMessenger = m_ProgressMessageHelper.createProgressMessenger();
    for(usize featureIdx = start; featureIdx < end; featureIdx++)
    {
      incCount++;
      if(incCount >= increment)
      {
        progressMessenger.sendProgressMessage(incCount, [&](usize currentProgress, usize maxProgress) {
          return fmt::format("Calculating feature histograms {}/{}", currentProgress, maxProgress);
        });
        incCount = 0;
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
    progressMessenger.sendProgressMessage(incCount);
  }

  void operator()(const Range& range) const
  {
    convert(range[0], range[1]);
  }

private:
  ComputeNeighborhoods* m_Filter = nullptr;
  usize m_TotalFeatures = 0;
  const std::vector<int64>& m_Bins;
  const std::vector<float32>& m_CriticalDistance;
  const std::atomic_bool& m_ShouldCancel;
  ProgressMessageHelper& m_ProgressMessageHelper;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeNeighborhoods::ComputeNeighborhoods(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeNeighborhoodsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_MessageHelper(m_MessageHandler)
{
}

// -----------------------------------------------------------------------------
ComputeNeighborhoods::~ComputeNeighborhoods() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeNeighborhoods::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void ComputeNeighborhoods::updateNeighborHood(usize sourceIndex, usize destIndex)
{
  const std::lock_guard<std::mutex> lock(m_Mutex);
  (*m_Neighborhoods)[sourceIndex]++;
  m_LocalNeighborhoodList[sourceIndex].push_back(static_cast<int32_t>(destIndex));
}

// -----------------------------------------------------------------------------
Result<> ComputeNeighborhoods::operator()()
{
  // m_ProgressCounter initialized to zero on filter creation
  std::vector<float32> criticalDistance;

  auto multiplesOfAverage = m_InputValues->MultiplesOfAverage;
  const auto& equivalentDiameters = m_DataStructure.getDataAs<Float32Array>(m_InputValues->EquivalentDiametersArrayPath)->getDataStoreRef();
  const auto& centroids = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

  m_Neighborhoods = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NeighborhoodsArrayName);

  usize totalFeatures = equivalentDiameters.getNumberOfTuples();

  ProgressMessageHelper progressMessageHelper = m_MessageHelper.createProgressMessageHelper();
  progressMessageHelper.setMaxProgresss(totalFeatures);
  progressMessageHelper.setProgressMessageTemplate("Finding Feature Neighborhoods: {:.2f}%");

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
  parallelAlgorithm.execute(ComputeNeighborhoodsImpl(this, totalFeatures, bins, criticalDistance, m_ShouldCancel, progressMessageHelper));

  // Output Variables
  auto& outputNeighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NeighborhoodListArrayName);
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
