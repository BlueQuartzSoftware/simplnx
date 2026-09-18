#include <array>

#include "ComputeFeatureReferenceMisorientations.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <nonstd/span.hpp>

#include <memory>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 65536;
} // namespace

ComputeFeatureReferenceMisorientations::ComputeFeatureReferenceMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                               ComputeFeatureReferenceMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureReferenceMisorientations::~ComputeFeatureReferenceMisorientations() noexcept = default;

const std::atomic_bool& ComputeFeatureReferenceMisorientations::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeFeatureReferenceMisorientations::operator()()
{
  // The validated Cell Phases path follows the standard ImageGeom hierarchy.
  DataPath imageGeomPath = m_InputValues->CellPhasesArrayPath.getParent().getParent();
  const ImageGeom& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  const auto& cellPhasesArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& featureIdsArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& quatsArrayRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);

  const auto* avgQuatsPtr = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  const auto* featureAttrMatPtr = m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->FeatureAttributeMatrixPath);
  const auto& crystalStructuresArrayRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  auto& featureReferenceMisorientations = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureReferenceMisorientationsArrayName);
  auto& avgReferenceMisorientation = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureAvgMisorientationsArrayName);

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->FeatureAvgMisorientationsArrayName, featureIdsArrayRef, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  const usize totalVoxels = featureIdsArrayRef.getNumberOfTuples();

  // Either configured feature source supplies the same tuple count.
  usize totalFeatures = 0;
  if(featureAttrMatPtr != nullptr)
  {
    totalFeatures = featureAttrMatPtr->getNumberOfTuples();
  }
  if(avgQuatsPtr != nullptr)
  {
    totalFeatures = avgQuatsPtr->getNumberOfTuples();
  }
  if(totalFeatures == 0)
  {
    return MakeErrorResult(-34900, "Total features was zero. The filter cannot proceed. Check either the feature attribute matrix or the average quaternions for proper size");
  }

  // The local ensemble cache avoids cell-loop store access.
  const usize numCrystalStructures = crystalStructuresArrayRef.getNumberOfTuples();
  std::vector<uint32> crystalStructuresCache(numCrystalStructures);
  if(Result<> ioResult = crystalStructuresArrayRef.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.data(), numCrystalStructures)); ioResult.invalid())
  {
    return ConvertResult(std::move(ioResult));
  }

  // Average quaternions stay local for random feature access.
  std::vector<float32> localAvgQuats;
  if(m_InputValues->ReferenceOrientation == 0 && avgQuatsPtr != nullptr)
  {
    localAvgQuats.resize(totalFeatures * 4);
    if(Result<> ioResult = avgQuatsPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(localAvgQuats.data(), totalFeatures * 4)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
  }

  std::vector<usize> centerVoxels(totalFeatures, 0);
  std::vector<float32> centerDistances(totalFeatures, 0.0f);
  std::vector<float32> centerQuats;

  const auto& featureIdsStoreRef = featureIdsArrayRef.getDataStoreRef();
  const auto& cellPhasesStoreRef = cellPhasesArrayRef.getDataStoreRef();
  const auto& quatsStoreRef = quatsArrayRef.getDataStoreRef();
  auto& misoStore = featureReferenceMisorientations.getDataStoreRef();

  // Mode 1 selects the farthest grain-boundary cell for each feature.
  if(m_InputValues->ReferenceOrientation == 1)
  {
    const auto& gbDistStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->GBEuclideanDistancesArrayPath).getDataStoreRef();
    auto fidBuf = std::make_unique<std::array<int32, k_ChunkTuples>>();
    auto distBuf = std::make_unique<std::array<float32, k_ChunkTuples>>();

    for(usize offset = 0; offset < totalVoxels; offset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkTuples, totalVoxels - offset);
      if(Result<> ioResult = featureIdsStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(fidBuf->data(), count)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
      if(Result<> ioResult = gbDistStore.copyIntoBuffer(offset, nonstd::span<float32>(distBuf->data(), count)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
      for(usize i = 0; i < count; i++)
      {
        const int32 featureId = (*fidBuf)[i];
        // A later equal-distance cell wins to preserve legacy raster order.
        if(featureId > 0 && (*distBuf)[i] >= centerDistances[featureId])
        {
          centerDistances[featureId] = (*distBuf)[i];
          centerVoxels[featureId] = offset + i;
        }
      }
    }

    const auto& euclideanCellCenters = m_DataStructure.getDataAs<Float32Array>(m_InputValues->FeatureEuclideanCentersPath)->getIDataStoreAs<AbstractDataStore<float32>>();
    for(usize i = 1; i < totalFeatures; i++)
    {
      auto cellCenter = imageGeom.getCoordsf(centerVoxels[i]);
      euclideanCellCenters->setTuple(i, cellCenter.data());
    }

    // One point read per feature avoids a full quaternion cache.
    centerQuats.resize(totalFeatures * 4, 0.0f);
    for(usize i = 1; i < totalFeatures; i++)
    {
      std::array<float32, 4> qBuf = {};
      if(Result<> ioResult = quatsStoreRef.copyIntoBuffer(centerVoxels[i] * 4, nonstd::span<float32>(qBuf.data(), qBuf.size())); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
      centerQuats[i * 4 + 0] = qBuf[0];
      centerQuats[i * 4 + 1] = qBuf[1];
      centerQuats[i * 4 + 2] = qBuf[2];
      centerQuats[i * 4 + 3] = qBuf[3];
    }
  }

  std::vector<float32> avgMisorientationSums(totalFeatures, 0.0f);
  std::vector<float32> avgMisorientationCounts(totalFeatures, 0.0f);
  featureReferenceMisorientations.fill(0.0f);

  auto featureIdsBuffer = std::make_unique<std::array<int32, k_ChunkTuples>>();
  auto cellPhasesBuffer = std::make_unique<std::array<int32, k_ChunkTuples>>();
  auto quatsBuffer = std::make_unique<std::array<float32, k_ChunkTuples * 4>>();
  auto misoBuf = std::make_unique<std::array<float32, k_ChunkTuples>>();

  for(usize offset = 0; offset < totalVoxels; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, totalVoxels - offset);
    if(Result<> ioResult = featureIdsStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(featureIdsBuffer->data(), count)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
    if(Result<> ioResult = cellPhasesStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(cellPhasesBuffer->data(), count)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
    if(Result<> ioResult = quatsStoreRef.copyIntoBuffer(offset * 4, nonstd::span<float32>(quatsBuffer->data(), count * 4)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
    std::fill_n(misoBuf->data(), count, 0.0f);

    for(usize chunkTupleIdx = 0; chunkTupleIdx < count; chunkTupleIdx++)
    {
      const int32 currentFeatureIdx = (*featureIdsBuffer)[chunkTupleIdx];
      const int32 currentPhaseIdx = (*cellPhasesBuffer)[chunkTupleIdx];
      if(currentFeatureIdx > 0 && currentPhaseIdx > 0)
      {
        if(static_cast<usize>(currentPhaseIdx) >= numCrystalStructures)
        {
          return MakeErrorResult(-34901,
                                 fmt::format("Cell Phases array '{}' has value {} at voxel index {}, but Crystal Structures array '{}' has {} tuples. Valid positive Phase indices are in [1, {}).",
                                             m_InputValues->CellPhasesArrayPath.toString(), currentPhaseIdx, offset + chunkTupleIdx, m_InputValues->CrystalStructuresArrayPath.toString(),
                                             numCrystalStructures, numCrystalStructures));
        }
        const uint32 currentLaueIndex = crystalStructuresCache[currentPhaseIdx];
        if(currentLaueIndex >= orientationOps.size())
        {
          return MakeErrorResult(-34902, fmt::format("Crystal Structures array '{}' has value {} at Phase index {}, but only {} Laue operations are available. Valid Laue indices are in [0, {}).",
                                                     m_InputValues->CrystalStructuresArrayPath.toString(), currentLaueIndex, currentPhaseIdx, orientationOps.size(), orientationOps.size()));
        }
        const usize qi = chunkTupleIdx * 4;
        ebsdlib::QuatD q1((*quatsBuffer)[qi], (*quatsBuffer)[qi + 1], (*quatsBuffer)[qi + 2], (*quatsBuffer)[qi + 3]);
        ebsdlib::QuatD q2;
        if(m_InputValues->ReferenceOrientation == 0)
        {
          const usize fi = static_cast<usize>(currentFeatureIdx) * 4;
          q2 = ebsdlib::QuatD(localAvgQuats[fi], localAvgQuats[fi + 1], localAvgQuats[fi + 2], localAvgQuats[fi + 3]);
        }
        else if(m_InputValues->ReferenceOrientation == 1)
        {
          const usize fi = static_cast<usize>(currentFeatureIdx) * 4;
          q2 = ebsdlib::QuatD(centerQuats[fi], centerQuats[fi + 1], centerQuats[fi + 2], centerQuats[fi + 3]);
        }

        ebsdlib::AxisAngleDType axisAngle = orientationOps[currentLaueIndex]->calculateMisorientation(q1, q2);
        const float32 misoValue = static_cast<float32>(Constants::k_RadToDegD * axisAngle[3]);
        (*misoBuf)[chunkTupleIdx] = misoValue;
        avgMisorientationCounts[currentFeatureIdx]++;
        avgMisorientationSums[currentFeatureIdx] += misoValue;
      }
    }
    if(Result<> ioResult = misoStore.copyFromBuffer(offset, nonstd::span<const float32>(misoBuf->data(), count)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
  }

  avgReferenceMisorientation[0] = 0.0f;
  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    avgReferenceMisorientation[featureIdx] = (avgMisorientationCounts[featureIdx] == 0.0f) ? 0.0f : avgMisorientationSums[featureIdx] / avgMisorientationCounts[featureIdx];
  }
  return {};
}
