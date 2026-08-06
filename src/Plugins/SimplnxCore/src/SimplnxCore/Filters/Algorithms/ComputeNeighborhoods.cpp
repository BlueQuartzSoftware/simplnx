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
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>

#include <algorithm>
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
  ComputeNeighborhoodsImpl(ComputeNeighborhoods* filter, const nx::core::AbstractDataStore<float>& centroids, const std::vector<int64>& bins, const std::vector<float32>& radii, float32 binSize,
                           const std::atomic_bool& shouldCancel)
  : m_Filter(filter)
  , m_Centroids(centroids)
  , m_Bins(bins)
  , m_Radii(radii)
  , m_BinSize(binSize)
  , m_ShouldCancel(shouldCancel)
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

    for(usize i = start; i < end; i++)
    {
      incCount++;
      if(incCount >= increment)
      {
        m_Filter->sendThreadSafeProgressMessage(incCount);
        incCount = 0;
      }

      if(m_ShouldCancel)
      {
        return;
      }

      // (a) This feature's own search radius (per-feature in Multiples mode; constant in Search-Radius mode).
      //     The scan window k is derived from this feature's radius and the bin size.
      const float32 radiusSq = m_Radii[i] * m_Radii[i];
      const int64 k = static_cast<int64>(std::ceil(2.0f * m_Radii[i] / m_BinSize));

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
    m_Filter->sendThreadSafeProgressMessage(incCount);
  }

  void operator()(const Range& range) const
  {
    convert(range[0], range[1]);
  }

private:
  ComputeNeighborhoods* m_Filter = nullptr;
  const nx::core::AbstractDataStore<float>& m_Centroids;
  const std::vector<int64>& m_Bins;
  const std::vector<float32>& m_Radii;
  float32 m_BinSize;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeNeighborhoods::ComputeNeighborhoods(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeNeighborhoodsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Throttle(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeNeighborhoods::~ComputeNeighborhoods() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeNeighborhoods::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.incrementCount(counter);
}

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
  const auto& centroids = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

  m_Neighborhoods = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NeighborhoodsArrayName);

  const usize totalFeatures = centroids.getNumberOfTuples();
  if(totalFeatures == 0)
  {
    return {};
  }

  m_Throttle.reset(totalFeatures, "Finding Feature Neighborhoods");

  m_LocalNeighborhoodList.resize(totalFeatures);

  for(usize i = 0; i < totalFeatures; i++)
  {
    (*m_Neighborhoods)[i] = 0;
  }

  // Determine each feature's neighbor search radius and the spatial-bin grid size based on the user-selected
  // Search Radius Type.
  //   Type 0 (Multiples of Equivalent Diameter): each feature searches within its OWN Equivalent Sphere Diameter
  //     times the multiplier (radius_i = equivalentDiameters[i] * multiples). The neighbor relation is therefore
  //     per-feature (asymmetric): larger features have larger neighborhoods. The bin grid is sized by the
  //     average diameter of all features.
  //   Type 1 (Search Radius in microns): every feature uses the same absolute radius supplied by the user; the
  //     Equivalent Diameters array is not needed, so the bin grid is sized by the search radius itself.
  std::vector<float32> radii(totalFeatures, 0.0f);
  float32 binSize = 0.0f;
  if(m_InputValues->SearchRadiusType == 0)
  {
    // Find the average equivalent spherical (ESD) diameter of ALL features (excluding the background feature 0);
    // used only to size the bin grid.
    const auto& equivalentDiameters = m_DataStructure.getDataAs<Float32Array>(m_InputValues->EquivalentDiametersArrayPath)->getDataStoreRef();
    float32 avgDiameter = 0.0f;
    for(usize i = 1; i < totalFeatures; i++)
    {
      avgDiameter += equivalentDiameters[i];
    }
    if(totalFeatures > 1)
    {
      avgDiameter /= static_cast<float32>(totalFeatures - 1);
    }
    m_MessageHandler.sendInfoMessage(fmt::format("Feature Average Diameter: '{}'", avgDiameter));

    for(usize i = 1; i < totalFeatures; i++)
    {
      radii[i] = equivalentDiameters[i] * multiplesOfAverage;
    }
    binSize = avgDiameter;
  }
  else
  {
    // Feature 0 is the background/unassigned feature: it gets no search radius so it is never a search source.
    const float32 searchRadius = m_InputValues->SearchRadius;
    std::fill(radii.begin() + 1, radii.end(), searchRadius);
    binSize = searchRadius;
  }

  // Place each feature's centroid into a bin in the normalized 3D space (normalized by binSize)
  std::vector<int64> bins(3 * totalFeatures, 0);
  FloatVec3 origin = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometry)->getOrigin();
  for(usize i = 1; i < totalFeatures; i++)
  {
    const float32 x = centroids[3 * i];
    const float32 y = centroids[3 * i + 1];
    const float32 z = centroids[3 * i + 2];
    bins[3 * i] = static_cast<int64>((x - origin[0]) / binSize);     // x-Bin
    bins[3 * i + 1] = static_cast<int64>((y - origin[1]) / binSize); // y-Bin
    bins[3 * i + 2] = static_cast<int64>((z - origin[2]) / binSize); // z-Bin
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  // Feature 0 is the background/unassigned feature: it is excluded both as a search source (range starts at 1)
  // and as a candidate (binToFeatures is built from feature 1 onward), so Neighborhoods[0] stays 0 and
  // NeighborhoodList[0] stays empty.
  ParallelDataAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(Range(1, totalFeatures));
  IParallelAlgorithm::AlgorithmStores algStores;
  algStores.push_back(&centroids);
  parallelAlgorithm.requireStoresInMemory(algStores);
  parallelAlgorithm.execute(ComputeNeighborhoodsImpl(this, centroids, bins, radii, binSize, m_ShouldCancel));

  // Output Variables
  auto& outputNeighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NeighborhoodListArrayName);
  // Set the vector for each list into the NeighborList Object. Feature 0 gets an explicit empty list so the
  // Neighborhoods[i] == NeighborhoodList[i].size() invariant holds at every index.
  for(usize i = 0; i < totalFeatures; i++)
  {
    // Construct a shared vector<int32> through the std::vector<> copy constructor.
    const NeighborList<int32>::SharedVectorType sharedMisOrientationList(new std::vector<int32>(m_LocalNeighborhoodList[i]));
    outputNeighborList.setList(static_cast<int32>(i), sharedMisOrientationList);
  }

  m_LocalNeighborhoodList.clear();

  return {};
}
