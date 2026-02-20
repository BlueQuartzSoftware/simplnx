#include "ComputeNeighborhoods.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

using namespace nx::core;
namespace
{
struct BinKey
{
  int64 bx = 0;
  int64 by = 0;
  int64 bz = 0;

  BinKey(int64 x, int64 y, int64 z)
  : bx(x)
  , by(y)
  , bz(z)
  {
  }

  BinKey(const std::vector<int64>& bins, usize tupleIndex)
  {
    bx = bins[3 * tupleIndex + 0];
    by = bins[3 * tupleIndex + 1];
    bz = bins[3 * tupleIndex + 2];
  }

  bool operator==(const BinKey& other) const noexcept
  {
    return bx == other.bx && by == other.by && bz == other.bz;
  }
};

struct BinKeyHasher
{
  usize operator()(const BinKey& key) const noexcept
  {
    // simple hash combine
    const usize h1 = std::hash<int64>{}(key.bx);
    const usize h2 = std::hash<int64>{}(key.by);
    const usize h3 = std::hash<int64>{}(key.bz);
    usize seed = h1;
    seed ^= h2 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    seed ^= h3 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    return seed;
  }
};

class ComputeNeighborhoodsImpl
{
public:
  ComputeNeighborhoodsImpl(ComputeNeighborhoods* filter, const nx::core::AbstractDataStore<float>& centroids, const std::vector<int64>& bins, float32 avgDiam, float32 multiplesOfAverage,
                           const std::atomic_bool& shouldCancel, ProgressHelper& progressHelper)
  : m_Filter(filter)
  , m_Centroids(centroids)
  , m_Bins(bins)
  , m_AvgDiam(avgDiam)
  , m_MultiplesOfAverage(multiplesOfAverage)
  , m_ShouldCancel(shouldCancel)
  , m_ProgressHelper(progressHelper)
  {
  }

  void convert(usize start, usize end) const
  {
    const auto increment = static_cast<int64>((end - start) / 100.0);
    int64 incCount = 0.0;

    const usize totalFeatures = m_Centroids.getNumberOfTuples();

    // 1. Build spatial grid: BinKey -> list of features
    std::unordered_map<BinKey, std::vector<usize>, BinKeyHasher> binToFeatures;
    binToFeatures.reserve(totalFeatures);

    for(usize i = 1; i < totalFeatures; ++i) // assuming feature 0 is background
    {
      const BinKey key(m_Bins, i);
      binToFeatures[key].push_back(i);
    }

    // 2. Precompute radius info
    const float32 radius = m_AvgDiam * m_MultiplesOfAverage / 2.0f;
    const float32 radiusSq = radius * radius;
    const int64 k = static_cast<int64>(std::ceil(m_MultiplesOfAverage));

    ProgressWorker progressWorker = m_ProgressHelper.createWorkerHandle();
    for(usize i = start; i < end; i++)
    {
      incCount++;
      if(incCount >= increment)
      {
        progressWorker.incrementProgress(incCount);
        incCount = 0;
      }

      if(m_ShouldCancel)
      {
        return;
      }
      // (a) Get feature's i position
      const float32 xi = m_Centroids[3 * i + 0];
      const float32 yi = m_Centroids[3 * i + 1];
      const float32 zi = m_Centroids[3 * i + 2];

      // (b) Get its bin
      const int64 bx0 = m_Bins[3 * i + 0];
      const int64 by0 = m_Bins[3 * i + 1];
      const int64 bz0 = m_Bins[3 * i + 2];

      // (c) Scan all bins within +/- k in each dimension
      for(int64 dbx = -k; dbx <= k; ++dbx)
      {
        for(int64 dby = -k; dby <= k; ++dby)
        {
          for(int64 dbz = -k; dbz <= k; ++dbz)
          {
            const BinKey nbKey{bx0 + dbx, by0 + dby, bz0 + dbz};

            auto it = binToFeatures.find(nbKey);
            if(it == binToFeatures.end())
            {
              continue; // no features in this bin
            }

            const std::vector<usize>& candidates = it->second;

            // (d) Check actual distances to candidates in this bin
            for(const usize j : candidates)
            {
              if(j == i)
              {
                continue; // skip self
              }

              const float32 xj = m_Centroids[3 * j + 0];
              const float32 yj = m_Centroids[3 * j + 1];
              const float32 zj = m_Centroids[3 * j + 2];

              const float32 dx = xi - xj;
              const float32 dy = yi - yj;
              const float32 dz = zi - zj;

              const float32 distSq = dx * dx + dy * dy + dz * dz;
              if(distSq <= radiusSq)
              {
                m_Filter->updateNeighborHood(i, j);
              }
            }
          }
        }
      }
    }
    progressWorker.incrementProgress(incCount);
  }

  void operator()(const Range& range) const
  {
    convert(range[0], range[1]);
  }

private:
  ComputeNeighborhoods* m_Filter = nullptr;
  const nx::core::AbstractDataStore<float>& m_Centroids;
  const std::vector<int64>& m_Bins;
  float32 m_AvgDiam;
  float32 m_MultiplesOfAverage;
  const std::atomic_bool& m_ShouldCancel;
  ProgressHelper& m_ProgressHelper;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeNeighborhoods::ComputeNeighborhoods(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeNeighborhoodsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeNeighborhoods::~ComputeNeighborhoods() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeNeighborhoods::updateNeighborHood(usize sourceIndex, usize destIndex)
{
  const std::scoped_lock lock(m_Mutex);
  (*m_Neighborhoods)[sourceIndex].inc();
  m_LocalNeighborhoodList[sourceIndex].push_back(static_cast<int32_t>(destIndex));
}

// -----------------------------------------------------------------------------
Result<> ComputeNeighborhoods::operator()()
{
  // m_ProgressCounter initialized to zero on filter creation
  auto multiplesOfAverage = m_InputValues->MultiplesOfAverage;
  const auto& equivalentDiameters = m_DataStructure.getDataAs<Float32Array>(m_InputValues->EquivalentDiametersArrayPath)->getDataStoreRef();
  const auto& centroids = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

  m_Neighborhoods = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NeighborhoodsArrayName);

  const usize totalFeatures = equivalentDiameters.getNumberOfTuples();

  MessageHelper messageHelper(m_MessageHandler);
  ProgressHelper progressHelper = messageHelper.createProgressHelper(totalFeatures, [](usize currentProgress, usize maxProgress) {
    return fmt::format("Finding Feature Neighborhoods: {:.2f}%", CalculatePercentComplete(currentProgress, maxProgress));
  });

  m_LocalNeighborhoodList.resize(totalFeatures);

  // (a) This section finds the average equivalent spherical (ESD) diameter of ALL features
  float32 avgDiameter = 0.0f;
  for(usize i = 1; i < totalFeatures; i++)
  {
    (*m_Neighborhoods)[i] = 0;
    avgDiameter += equivalentDiameters[i];
  }
  avgDiameter /= static_cast<float32>(totalFeatures);
  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Feature Average Diameter: '{}'", avgDiameter));

  // (c) We are going to place each feature's centroid into a bin in the normalized 3D space.
  // The centroid is normalized by the Average Diameter
  std::vector<int64> bins(3 * totalFeatures, 0);
  FloatVec3 origin = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometry)->getOrigin();
  for(usize i = 1; i < totalFeatures; i++)
  {
    const float32 x = centroids[3 * i];
    const float32 y = centroids[3 * i + 1];
    const float32 z = centroids[3 * i + 2];
    bins[3 * i] = static_cast<int64>((x - origin[0]) / avgDiameter);     // x-Bin
    bins[3 * i + 1] = static_cast<int64>((y - origin[1]) / avgDiameter); // y-Bin
    bins[3 * i + 2] = static_cast<int64>((z - origin[2]) / avgDiameter); // z-Bin
  }
  if(m_ShouldCancel)
  {
    return {};
  }
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(Range(0, totalFeatures));
  parallelAlgorithm.setParallelizationEnabled(true);
  parallelAlgorithm.execute(ComputeNeighborhoodsImpl(this, centroids, bins, avgDiameter, multiplesOfAverage, m_ShouldCancel, progressHelper));

  // Output Variables
  auto& outputNeighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NeighborhoodListArrayName);
  // Set the vector for each list into the NeighborList Object
  for(usize i = 1; i < totalFeatures; i++)
  {
    // Construct a shared vector<int32> through the std::vector<> copy constructor.
    const NeighborList<int32>::SharedVectorType sharedMisOrientationList(new std::vector<int32>(m_LocalNeighborhoodList[i]));
    outputNeighborList.setList(static_cast<int32>(i), sharedMisOrientationList);
  }

  m_LocalNeighborhoodList.clear();

  return {};
}
