#include "SharedFeatureFace.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <map>
#include <random>
#include <vector>

using namespace nx::core;

namespace
{
//------------------------------------------------------------------------------
uint64 ConvertToUInt64(uint32 highWord, uint32 lowWord)
{
  return ((uint64)highWord << 32) | lowWord;
}

//------------------------------------------------------------------------------
class SharedFeatureFaceImpl
{
public:
  SharedFeatureFaceImpl(const std::vector<std::pair<int32, int32>>& faceLabelVector, Int32AbstractDataStore& surfaceMeshFeatureFaceLabels, Int32AbstractDataStore& surfaceMeshFeatureFaceNumTriangles,
                        std::map<uint64, int32>& faceSizeMap, const std::atomic_bool& shouldCancel)
  : m_FaceLabelVector(faceLabelVector)
  , m_SurfaceMeshFeatureFaceLabels(surfaceMeshFeatureFaceLabels)
  , m_SurfaceMeshFeatureFaceNumTriangles(surfaceMeshFeatureFaceNumTriangles)
  , m_FaceSizeMap(faceSizeMap)
  , m_ShouldCancel(shouldCancel)
  {
  }
  ~SharedFeatureFaceImpl() = default;

  void operator()(const Range& range) const
  {
    for(usize i = range.min(); i < range.max(); i++)
    {
      const auto& faceLabelMapEntry = m_FaceLabelVector[i];
      // get feature face labels
      m_SurfaceMeshFeatureFaceLabels[2 * i + 0] = faceLabelMapEntry.first;
      m_SurfaceMeshFeatureFaceLabels[2 * i + 1] = faceLabelMapEntry.second;

      // get feature triangle count
      uint64 faceId64 = ConvertToUInt64(faceLabelMapEntry.first, faceLabelMapEntry.second);
      m_SurfaceMeshFeatureFaceNumTriangles[i] = m_FaceSizeMap[faceId64];
      if(m_ShouldCancel)
      {
        break;
      }
    }
  }

private:
  const std::vector<std::pair<int32, int32>>& m_FaceLabelVector;
  Int32AbstractDataStore& m_SurfaceMeshFeatureFaceLabels;
  Int32AbstractDataStore& m_SurfaceMeshFeatureFaceNumTriangles;
  std::map<uint64, int32>& m_FaceSizeMap;
  const std::atomic_bool& m_ShouldCancel;
};

using SeedGenerator = std::mt19937_64;
using Int64Distribution = std::uniform_int_distribution<int64>;
// -----------------------------------------------------------------------------
SeedGenerator initializeStaticVoxelSeedGenerator(Int64Distribution& distribution, const int64 rangeMin, const int64 rangeMax)
{
  SeedGenerator generator(SeedGenerator::default_seed);
  distribution = std::uniform_int_distribution<int64>(rangeMin, rangeMax);

  return generator;
}

// -----------------------------------------------------------------------------
void RandomizeFaceIds(nx::core::Int32Array& featureIds, uint64 totalFeatures, Int64Distribution& distribution)
{
  // Generate an even distribution of numbers between the min and max range
  const int64 rangeMin = 1;
  const int64 rangeMax = totalFeatures - 1;
  auto generator = initializeStaticVoxelSeedGenerator(distribution, rangeMin, rangeMax);

  DataStructure tmpStructure;
  auto* rndNumbers = Int64Array::CreateWithStore<DataStore<int64>>(tmpStructure, std::string("_INTERNAL_USE_ONLY_NewFeatureIds"), std::vector<usize>{totalFeatures}, std::vector<usize>{1});
  auto* rndStore = rndNumbers->getDataStore();

  for(int64 i = 0; i < totalFeatures; ++i)
  {
    rndStore->setValue(i, i);
  }

  int64 r = 0;
  int64 temp = 0;

  //--- Shuffle elements by randomly exchanging each with one other.
  for(int64 i = 1; i < totalFeatures; i++)
  {
    r = distribution(generator); // Random remaining position.
    if(r >= totalFeatures)
    {
      continue;
    }
    temp = rndStore->getValue(i);
    rndStore->setValue(i, rndStore->getValue(r));
    rndStore->setValue(r, temp);
  }

  // Now adjust all the GrainId values for each Voxel
  auto featureIdsStore = featureIds.getDataStore();
  uint64 totalPoints = featureIds.getNumberOfTuples();
  for(int64 i = 0; i < totalPoints; ++i)
  {
    featureIdsStore->setValue(i, rndStore->getValue(featureIdsStore->getValue(i)));
  }
}

Result<> ComputeSharedFeatureFacesInBlocks(DataStructure& dataStructure, const SharedFeatureFaceInputValues& inputValues, usize totalPoints, const Int32Array& faceLabels, Int32Array& featureFaceIds,
                                           const std::atomic_bool& shouldCancel)
{
  Result<> result;
  std::map<uint64, int32> faceSizeMap;
  std::map<uint64, int32> faceIdMap;
  int32 index = 1;
  std::vector<std::pair<int32, int32>> faceLabelVector;
  faceLabelVector.emplace_back(0, 0);

  // Two labels and one ID per triangle use at most 3 MiB of transfer buffers.
  constexpr usize k_BlockTriangles = 262144;
  const usize bufferTriangles = std::min(k_BlockTriangles, totalPoints);
  std::vector<int32> faceLabelsBuffer(bufferTriangles * 2);
  std::vector<int32> faceIdsBuffer(bufferTriangles);
  auto& faceIdsStore = featureFaceIds.getDataStoreRef();
  for(usize start = 0; start < totalPoints;)
  {
    const usize count = std::min(k_BlockTriangles, totalPoints - start);
    result = MergeResults(std::move(result), faceLabels.getDataStoreRef().copyIntoBuffer(start * 2, nonstd::span<int32>(faceLabelsBuffer.data(), count * 2)));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-12910, fmt::format("Error reading face labels for shared feature faces at triangle {} ({} triangles).", start, count)));
    }

    // Keep signed label ordering, unsigned key conversion and first-encounter IDs.
    for(usize t = 0; t < count; ++t)
    {
      uint32 high = 0;
      uint32 low = 0;
      int32 fl0 = faceLabelsBuffer[t * 2];
      int32 fl1 = faceLabelsBuffer[t * 2 + 1];
      if(fl0 < fl1)
      {
        high = fl0;
        low = fl1;
      }
      else
      {
        high = fl1;
        low = fl0;
      }
      uint64 faceId64 = ConvertToUInt64(high, low);
      if(faceSizeMap.find(faceId64) == faceSizeMap.end())
      {
        faceSizeMap[faceId64] = 1;
        faceIdMap[faceId64] = index;
        faceIdsBuffer[t] = index;
        faceLabelVector.emplace_back(high, low);
        ++index;
      }
      else
      {
        faceSizeMap[faceId64]++;
        faceIdsBuffer[t] = faceIdMap[faceId64];
      }
    }
    // Every ID in this span is assigned; any suffix beyond the geometry is untouched.
    result = MergeResults(std::move(result), faceIdsStore.copyFromBuffer(start, nonstd::span<const int32>(faceIdsBuffer.data(), count)));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-12911, fmt::format("Error writing shared feature face IDs at triangle {} ({} triangles).", start, count)));
    }
    start += count;
  }
  // Match the in-core cancellation point after assigning every triangle ID.
  if(shouldCancel)
  {
    return result;
  }

  auto& faceFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(inputValues.GrainBoundaryAttributeMatrixPath);
  ShapeType tDims = {static_cast<usize>(index)};
  // Preserve the in-core resize sequence and its GCC false-positive suppression.
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 15 && (__GNUC__ > 12 || (__GNUC__ == 12 && __GNUC_MINOR__ >= 4))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
  result = MergeResults(std::move(result), faceFeatureAttrMat.resizeTuples(tDims));
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 15 && (__GNUC__ > 12 || (__GNUC__ == 12 && __GNUC_MINOR__ >= 4))
#pragma GCC diagnostic pop
#endif
  if(result.invalid())
  {
    return result;
  }

  auto& featureFaceLabels = dataStructure.getDataRefAs<Int32Array>(inputValues.FeatureFaceLabelsArrayPath).getDataStoreRef();
  auto& featureFaceNumTriangles = dataStructure.getDataRefAs<Int32Array>(inputValues.FeatureFaceNumTrianglesArrayPath).getDataStoreRef();
  result = MergeResults(std::move(result), featureFaceLabels.resizeTuples(tDims));
  if(result.invalid())
  {
    return result;
  }
  result = MergeResults(std::move(result), featureFaceNumTriangles.resizeTuples(tDims));
  if(result.invalid())
  {
    return result;
  }

  {
    // Cache only the per-feature outputs (12 bytes per feature face). Preserve any
    // values left unwritten by the existing parallel functor when cancellation occurs.
    DataStore<int32> featureLabelsBuffer(tDims, {2}, std::nullopt);
    DataStore<int32> featureCountsBuffer(tDims, {1}, std::nullopt);
    result = MergeResults(std::move(result), featureFaceLabels.copyIntoBuffer(0, featureLabelsBuffer.createSpan()));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-12912, "Error reading shared feature face labels before updating them."));
    }
    result = MergeResults(std::move(result), featureFaceNumTriangles.copyIntoBuffer(0, featureCountsBuffer.createSpan()));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-12913, "Error reading shared feature face triangle counts before updating them."));
    }

    ParallelDataAlgorithm dataAlg;
    dataAlg.setParallelizationEnabled(true);
    dataAlg.setRange(0, index);
    dataAlg.execute(SharedFeatureFaceImpl(faceLabelVector, featureLabelsBuffer, featureCountsBuffer, faceSizeMap, shouldCancel));

    result = MergeResults(std::move(result), featureFaceLabels.copyFromBuffer(0, nonstd::span<const int32>(featureLabelsBuffer.data(), featureLabelsBuffer.getSize())));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-12914, "Error writing shared feature face labels."));
    }
    result = MergeResults(std::move(result), featureFaceNumTriangles.copyFromBuffer(0, nonstd::span<const int32>(featureCountsBuffer.data(), featureCountsBuffer.getSize())));
    if(result.invalid())
    {
      return MergeResults(std::move(result), MakeErrorResult(-12915, "Error writing shared feature face triangle counts."));
    }
  }

  if(inputValues.ShouldRandomizeFeatureIds)
  {
    // Match RandomizeFaceIds' seed, int64 distribution and swap order exactly.
    const uint64 totalFeatures = index;
    Int64Distribution distribution;
    auto generator = initializeStaticVoxelSeedGenerator(distribution, 1, totalFeatures - 1);
    std::vector<int64> rndNumbers(totalFeatures);
    for(int64 i = 0; i < totalFeatures; ++i)
    {
      rndNumbers[i] = i;
    }
    for(int64 i = 1; i < totalFeatures; ++i)
    {
      const int64 r = distribution(generator);
      if(r >= totalFeatures)
      {
        continue;
      }
      const int64 temp = rndNumbers[i];
      rndNumbers[i] = rndNumbers[r];
      rndNumbers[r] = temp;
    }

    // The original randomization visits the entire ID array, even if it has more
    // tuples than the geometry. Read each block before applying the permutation.
    const usize totalIds = featureFaceIds.getNumberOfTuples();
    faceIdsBuffer.resize(std::min(k_BlockTriangles, totalIds));
    for(usize start = 0; start < totalIds;)
    {
      const usize count = std::min(k_BlockTriangles, totalIds - start);
      result = MergeResults(std::move(result), faceIdsStore.copyIntoBuffer(start, nonstd::span<int32>(faceIdsBuffer.data(), count)));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-12916, fmt::format("Error reading shared feature face IDs for randomization at tuple {} ({} tuples).", start, count)));
      }
      for(usize i = 0; i < count; ++i)
      {
        faceIdsBuffer[i] = rndNumbers[faceIdsBuffer[i]];
      }
      result = MergeResults(std::move(result), faceIdsStore.copyFromBuffer(start, nonstd::span<const int32>(faceIdsBuffer.data(), count)));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-12917, fmt::format("Error writing randomized shared feature face IDs at tuple {} ({} tuples).", start, count)));
      }
      start += count;
    }
  }
  return result;
}

} // namespace

// -----------------------------------------------------------------------------
SharedFeatureFace::SharedFeatureFace(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SharedFeatureFaceInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SharedFeatureFace::~SharedFeatureFace() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SharedFeatureFace::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SharedFeatureFace::operator()()
{

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  usize totalPoints = triangleGeom.getNumberOfFaces();

  const Int32Array& surfaceMeshFaceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsArrayPath);

  auto& surfaceMeshFeatureFaceIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureFaceIdsArrayPath);

  const bool usesOutOfCoreStore = AnyOutOfCore({&surfaceMeshFaceLabels, &surfaceMeshFeatureFaceIds, m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureFaceLabelsArrayPath),
                                                m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureFaceNumTrianglesArrayPath)});
  const bool useOutOfCoreAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCoreAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  if(useOutOfCoreAlgorithm)
  {
    return ComputeSharedFeatureFacesInBlocks(m_DataStructure, *m_InputValues, totalPoints, surfaceMeshFaceLabels, surfaceMeshFeatureFaceIds, m_ShouldCancel);
  }

  std::map<uint64, int32> faceSizeMap;
  std::map<uint64, int32> faceIdMap; // This maps a unique 64-bit integer to an increasing 32-bit integer
  int32 index = 1;

  std::vector<std::pair<int32, int32>> faceLabelVector;
  faceLabelVector.emplace_back(0, 0);

  // Loop through all the Triangles and figure out how many triangles we have in each one.
  for(usize t = 0; t < totalPoints; ++t)
  {
    uint32 high = 0;
    uint32 low = 0;
    int32 fl0 = surfaceMeshFaceLabels[t * 2];
    int32 fl1 = surfaceMeshFaceLabels[t * 2 + 1];
    if(fl0 < fl1)
    {
      high = fl0;
      low = fl1;
    }
    else
    {
      high = fl1;
      low = fl0;
    }
    uint64 faceId64 = ConvertToUInt64(high, low);
    if(faceSizeMap.find(faceId64) == faceSizeMap.end())
    {
      faceSizeMap[faceId64] = 1;
      faceIdMap[faceId64] = index;
      surfaceMeshFeatureFaceIds[t] = index;
      faceLabelVector.emplace_back(high, low);
      ++index;
    }
    else
    {
      faceSizeMap[faceId64]++;
      surfaceMeshFeatureFaceIds[t] = faceIdMap[faceId64];
    }
  }
  if(m_ShouldCancel)
  {
    return {};
  }
  // resize + update pointers
  // Grain Boundary Attribute Matrix
  auto& faceFeatureAttrMat = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->GrainBoundaryAttributeMatrixPath);
  ShapeType tDims = {static_cast<usize>(index)};
  // This line has a Wstringop-overflow warning starting in gcc 12.4 in release configuration
  // This is a false positive that eventually disappears in gcc 15
  // Likely related to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94335
  // Minimum reproducible example: https://godbolt.org/z/qTr8hPo1e
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 15 && (__GNUC__ > 12 || (__GNUC__ == 12 && __GNUC_MINOR__ >= 4))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
  Result<> resizeResult = faceFeatureAttrMat.resizeTuples(tDims);
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 15 && (__GNUC__ > 12 || (__GNUC__ == 12 && __GNUC_MINOR__ >= 4))
#pragma GCC diagnostic pop
#endif
  if(resizeResult.invalid())
  {
    return resizeResult;
  }

  auto& surfaceMeshFeatureFaceLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureFaceLabelsArrayPath)->getDataStoreRef();
  auto& surfaceMeshFeatureFaceNumTriangles = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureFaceNumTrianglesArrayPath)->getDataStoreRef();
  resizeResult = surfaceMeshFeatureFaceLabels.resizeTuples(tDims);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  resizeResult = surfaceMeshFeatureFaceNumTriangles.resizeTuples(tDims);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }

  // For smaller data sets having data parallelization ON actually runs slower due to
  // all the overhead of the threads. We are just going to turn this off for
  // now until we hit a large data set that is better suited for parallelization
  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setParallelizationEnabled(true);
  dataAlg.setRange(0, index);
  dataAlg.execute(SharedFeatureFaceImpl(faceLabelVector, surfaceMeshFeatureFaceLabels, surfaceMeshFeatureFaceNumTriangles, faceSizeMap, m_ShouldCancel));

  if(m_InputValues->ShouldRandomizeFeatureIds)
  {
    const int64 rangeMin = 0;
    const auto rangeMax = static_cast<int64>(surfaceMeshFeatureFaceNumTriangles.getNumberOfTuples() - 1);
    Int64Distribution distribution;
    initializeStaticVoxelSeedGenerator(distribution, rangeMin, rangeMax);
    ::RandomizeFaceIds(surfaceMeshFeatureFaceIds, index, distribution);
  }
  return {};
}
