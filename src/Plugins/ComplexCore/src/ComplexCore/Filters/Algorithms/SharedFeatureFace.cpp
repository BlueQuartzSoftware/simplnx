#include "SharedFeatureFace.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <map>
#include <random>
#include <vector>

using namespace complex;

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
  SharedFeatureFaceImpl(const std::vector<std::pair<int32, int32>>& faceLabelVector, Int32Array& surfaceMeshFeatureFaceLabels, Int32Array& surfaceMeshFeatureFaceNumTriangles,
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
  Int32Array& m_SurfaceMeshFeatureFaceLabels;
  Int32Array& m_SurfaceMeshFeatureFaceNumTriangles;
  std::map<uint64, int32>& m_FaceSizeMap;
  const std::atomic_bool& m_ShouldCancel;
};

using SeedGenerator = std::mt19937_64;
using Int64Distribution = std::uniform_int_distribution<int64>;
// -----------------------------------------------------------------------------
SeedGenerator initializeStaticVoxelSeedGenerator(Int64Distribution& distribution, const int64 rangeMin, const int64 rangeMax)
{
  SeedGenerator generator;
  generator.seed(SeedGenerator::default_seed);
  distribution = std::uniform_int_distribution<int64>(rangeMin, rangeMax);

  return generator;
}

// -----------------------------------------------------------------------------
void RandomizeFaceIds(complex::Int32Array& featureIds, uint64 totalFeatures, Int64Distribution& distribution)
{
  // Generate an even distribution of numbers between the min and max range
  const int64 rangeMin = 1;
  const int64 rangeMax = totalFeatures - 1;
  auto generator = initializeStaticVoxelSeedGenerator(distribution, rangeMin, rangeMax);

  DataStructure tmpStructure;
  auto rndNumbers = Int64Array::CreateWithStore<DataStore<int64>>(tmpStructure, std::string("_INTERNAL_USE_ONLY_NewFeatureIds"), std::vector<usize>{totalFeatures}, std::vector<usize>{1});
  auto rndStore = rndNumbers->getDataStore();

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

  // Now adjust all the Grain Id values for each Voxel
  auto featureIdsStore = featureIds.getDataStore();
  uint64 totalPoints = featureIds.getNumberOfTuples();
  for(int64 i = 0; i < totalPoints; ++i)
  {
    featureIdsStore->setValue(i, rndStore->getValue(featureIdsStore->getValue(i)));
  }
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

  std::map<uint64, int32> faceSizeMap;
  std::map<uint64, int32> faceIdMap; // This maps a unique 64-bit integer to an increasing 32-bit integer
  int32 index = 1;

  std::vector<std::pair<int32, int32>> faceLabelVector;
  faceLabelVector.emplace_back(std::pair<int32, int32>(0, 0));

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
      faceLabelVector.emplace_back(std::pair<int32, int32>(high, low));
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
  std::vector<usize> tDims = {static_cast<usize>(index)};
  faceFeatureAttrMat.resizeTuples(tDims);

  auto& surfaceMeshFeatureFaceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureFaceLabelsArrayPath);
  auto& surfaceMeshFeatureFaceNumTriangles = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureFaceNumTrianglesArrayPath);
  surfaceMeshFeatureFaceLabels.getDataStore()->resizeTuples(tDims);
  surfaceMeshFeatureFaceNumTriangles.getDataStore()->resizeTuples(tDims);

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
    const int64 rangeMax = static_cast<int64>(surfaceMeshFeatureFaceNumTriangles.getNumberOfTuples() - 1);
    Int64Distribution distribution;
    initializeStaticVoxelSeedGenerator(distribution, rangeMin, rangeMax);
    ::RandomizeFaceIds(surfaceMeshFeatureFaceIds, index, distribution);
  }
  return {};
}
