#include "ComputeFeatureCentroids.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>

using namespace nx::core;

namespace
{
// Each bulk read contains 65,536 Feature IDs. This keeps the staging buffer cache-sized.
constexpr usize k_ChunkTuples = 65536;
// A feature with a per-axis unit-vector resultant below this threshold has its mass spread almost uniformly around the domain.
// This distribution makes the circular mean indeterminate. The algorithm keeps the arithmetic mean.
constexpr double k_DegenerateResultant = 1.0e-6;
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
  std::vector<float64> sumCos;
  std::vector<float64> sumSin;
  if(m_InputValues->IsPeriodic)
  {
    sumCos.resize(featureElems3, 0.0);
    sumSin.resize(featureElems3, 0.0);
  }
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
  const std::array<double, 3> domainLength = {static_cast<double>(xPoints) * spacing[0], static_cast<double>(yPoints) * spacing[1], static_cast<double>(zPoints) * spacing[2]};
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
    Result<> ioResult = featureIdsStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), chunkCount));
    if(ioResult.invalid())
    {
      return ioResult;
    }

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
        if(m_InputValues->IsPeriodic)
        {
          const double phase = Constants::k_2Pi<double> * (voxelCoords[c] - static_cast<double>(origin[c])) / domainLength[c];
          sumCos[fi] += std::cos(phase);
          sumSin[fi] += std::sin(phase);
        }
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
  if(m_InputValues->IsPeriodic)
  {
    m_MessageHandler({IFilter::Message::Type::Info, "Checking for periodic data."});
    // When a feature spans the full extent on an axis, its arithmetic centroid is in the empty middle of the wrapped feature.
    // Replace that component with the circular mean from the per-feature unit-vector sums.
    // If the resultant is near zero, the feature mass is almost uniform and the algorithm keeps the arithmetic mean.
    const std::array<const std::vector<uint64>*, 3> rangeStores = {&rangeX, &rangeY, &rangeZ};
    const std::array<usize, 3> dims = {xPoints, yPoints, zPoints};
    bool anyAdjusted = false;
    for(usize featureId = 0; featureId < totalFeatures; featureId++)
    {
      for(usize axis = 0; axis < 3; axis++)
      {
        const usize axisIdx = featureId * 3 + axis;
        if(voxelCount[axisIdx] == 0)
        {
          continue;
        }
        const auto& rangeStore = *rangeStores[axis];
        const bool spansExtent = rangeStore[featureId * 2] == 0 && rangeStore[featureId * 2 + 1] == dims[axis] - 1;
        if(!spansExtent)
        {
          continue;
        }
        const double resultant = std::sqrt(sumCos[axisIdx] * sumCos[axisIdx] + sumSin[axisIdx] * sumSin[axisIdx]) / static_cast<double>(voxelCount[axisIdx]);
        if(resultant < k_DegenerateResultant)
        {
          continue;
        }
        double phase = std::atan2(sumSin[axisIdx], sumCos[axisIdx]);
        if(phase < 0.0)
        {
          phase += Constants::k_2Pi<double>;
        }
        centroidsBuf[axisIdx] = static_cast<float32>(static_cast<double>(origin[axis]) + (phase / Constants::k_2Pi<double>)*domainLength[axis]);
        anyAdjusted = true;
      }
    }
    if(anyAdjusted)
    {
      m_MessageHandler({IFilter::Message::Type::Info, "ComputeFeatureCentroids adjusted centroids of features that wrap the periodic boundary."});
    }
  }

  Result<> writeResult = centroids.copyFromBuffer(0, nonstd::span<const float32>(centroidsBuf.data(), featureElems3));
  if(writeResult.invalid())
  {
    return writeResult;
  }

  return {};
}
