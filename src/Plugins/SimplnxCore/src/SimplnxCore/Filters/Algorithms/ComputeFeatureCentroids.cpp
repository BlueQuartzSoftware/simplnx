#include "ComputeFeatureCentroids.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <limits>
#include <memory>

using namespace nx::core;

namespace
{
// Each bulk read contains 65,536 Feature IDs. This keeps the staging buffer cache-sized.
constexpr usize k_ChunkTuples = 65536;
} // namespace

ComputeFeatureCentroids::ComputeFeatureCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeFeatureCentroidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureCentroids::~ComputeFeatureCentroids() noexcept = default;

const std::atomic_bool& ComputeFeatureCentroids::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeFeatureCentroids::operator()()
{
  const auto* featureIdsPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStoreRef = featureIdsPtr->getDataStoreRef();

  auto& centroids = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->CentroidsArrayPath, *featureIdsPtr, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize totalFeatures = centroids.getNumberOfTuples();
  const usize xPoints = imageGeom.getNumXCells();
  const usize yPoints = imageGeom.getNumYCells();
  const usize zPoints = imageGeom.getNumZCells();

  // Feature-sized vectors avoid DataStore calls in the voxel accumulation loop.
  const usize featureElems3 = totalFeatures * 3;
  const usize featureElems2 = totalFeatures * 2;
  std::vector<float64> kahanSum(featureElems3, 0.0);
  std::vector<float64> kahanComp(featureElems3, 0.0);
  std::vector<uint64> voxelCount(featureElems3, 0);
  std::vector<uint64> rangeX(featureElems2, 0);
  std::vector<uint64> rangeY(featureElems2, 0);
  std::vector<uint64> rangeZ(featureElems2, 0);

  for(usize f = 0; f < totalFeatures; f++)
  {
    rangeX[f * 2] = std::numeric_limits<uint64>::max();
    rangeY[f * 2] = std::numeric_limits<uint64>::max();
    rangeZ[f * 2] = std::numeric_limits<uint64>::max();
  }

  const FloatVec3 origin = imageGeom.getOrigin();
  const FloatVec3 spacing = imageGeom.getSpacing();
  const usize totalVoxels = xPoints * yPoints * zPoints;
  const usize xySize = xPoints * yPoints;

  // Each chunk resolves coordinates and feature accumulation from local values.
  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < totalVoxels; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize chunkCount = std::min(k_ChunkTuples, totalVoxels - offset);
    featureIdsStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), chunkCount));

    for(usize idx = 0; idx < chunkCount; idx++)
    {
      const int32 featureId = featureIdBuf[idx];
      // Feature zero is valid. Only negative or unassigned IDs are skipped.
      if(featureId < 0)
      {
        continue;
      }

      const usize flatIdx = offset + idx;
      const uint64 k = flatIdx % xPoints;
      const uint64 j = (flatIdx / xPoints) % yPoints;
      const uint64 i = flatIdx / xySize;
      const usize fid = static_cast<usize>(featureId);

      rangeX[fid * 2] = std::min(k, rangeX[fid * 2]);
      rangeX[fid * 2 + 1] = std::max(k, rangeX[fid * 2 + 1]);
      rangeY[fid * 2] = std::min(j, rangeY[fid * 2]);
      rangeY[fid * 2 + 1] = std::max(j, rangeY[fid * 2 + 1]);
      rangeZ[fid * 2] = std::min(i, rangeZ[fid * 2]);
      rangeZ[fid * 2 + 1] = std::max(i, rangeZ[fid * 2 + 1]);

      const double vx = static_cast<double>(origin[0]) + (static_cast<double>(k) + 0.5) * static_cast<double>(spacing[0]);
      const double vy = static_cast<double>(origin[1]) + (static_cast<double>(j) + 0.5) * static_cast<double>(spacing[1]);
      const double vz = static_cast<double>(origin[2]) + (static_cast<double>(i) + 0.5) * static_cast<double>(spacing[2]);

      const std::array<double, 3> voxelCoords = {vx, vy, vz};
      for(usize c = 0; c < 3; c++)
      {
        const usize fi = fid * 3 + c;
        const double componentValue = voxelCoords[c] - kahanComp[fi];
        const double temp = kahanSum[fi] + componentValue;
        kahanComp[fi] = (temp - kahanSum[fi]) - componentValue;
        kahanSum[fi] = temp;
        voxelCount[fi]++;
      }
    }
  }

  // One bulk write publishes all finalized Kahan means.
  std::vector<float32> centroidsBuf(featureElems3, 0.0f);
  for(usize featureId = 0; featureId < totalFeatures; featureId++)
  {
    for(usize c = 0; c < 3; c++)
    {
      const usize fi = featureId * 3 + c;
      if(voxelCount[fi] > 0)
      {
        centroidsBuf[fi] = static_cast<float32>(kahanSum[fi] / static_cast<float64>(voxelCount[fi]));
      }
    }
  }
  centroids.copyFromBuffer(0, nonstd::span<const float32>(centroidsBuf.data(), featureElems3));

  if(m_InputValues->IsPeriodic)
  {
    m_MessageHandler({IFilter::Message::Type::Info, "Checking for periodic data."});

    ShapeType tupleShape{totalFeatures};
    ShapeType componentShape{2};
    // Periodic adjustment receives plain stores because these feature ranges never enter the DataStructure.
    auto rangeXStorePtr = std::make_shared<DataStore<uint64>>(tupleShape, componentShape, uint64{0});
    auto rangeYStorePtr = std::make_shared<DataStore<uint64>>(tupleShape, componentShape, uint64{0});
    auto rangeZStorePtr = std::make_shared<DataStore<uint64>>(tupleShape, componentShape, uint64{0});
    auto& rangeXStoreRef = *rangeXStorePtr;
    auto& rangeYStoreRef = *rangeYStorePtr;
    auto& rangeZStoreRef = *rangeZStorePtr;
    for(usize i = 0; i < featureElems2; i++)
    {
      rangeXStoreRef[i] = rangeX[i];
      rangeYStoreRef[i] = rangeY[i];
      rangeZStoreRef[i] = rangeZ[i];
    }

    if(GeometryHelpers::Topology::AdjustCentroidsForPeriodicFaces(imageGeom, rangeXStoreRef, rangeYStoreRef, rangeZStoreRef, centroids))
    {
      m_MessageHandler({IFilter::Message::Type::Info, "ComputeFeatureCentroids found Non-Contiguous Features. Centroids may require additional checks."});
    }
  }

  return {};
}
