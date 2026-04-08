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

// -----------------------------------------------------------------------------
ComputeFeatureReferenceMisorientations::ComputeFeatureReferenceMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                               ComputeFeatureReferenceMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureReferenceMisorientations::~ComputeFeatureReferenceMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeFeatureReferenceMisorientations::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeFeatureReferenceMisorientations::operator()()
{
  DataPath imageGeomPath = m_InputValues->CellPhasesArrayPath.getParent().getParent();
  const ImageGeom& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  // Input Arrays
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);

  const auto* avgQuatsPtr = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  const auto* featureAttrMatPtr = m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->FeatureAttributeMatrixPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  auto& featureReferenceMisorientations = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureReferenceMisorientationsArrayName);
  auto& avgReferenceMisorientation = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureAvgMisorientationsArrayName);

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->FeatureAvgMisorientationsArrayName, featureIds, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  const usize totalVoxels = featureIds.getNumberOfTuples();

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

  // Cache crystal structures locally (ensemble-level, tiny)
  const usize numXtalEntries = crystalStructures.getNumberOfTuples();
  std::vector<uint32> localCrystalStructures(numXtalEntries);
  crystalStructures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(localCrystalStructures.data(), numXtalEntries));

  // Cache avgQuats locally (feature-level) — used in mode 0
  std::vector<float32> localAvgQuats;
  if(m_InputValues->ReferenceOrientation == 0 && avgQuatsPtr != nullptr)
  {
    localAvgQuats.resize(totalFeatures * 4);
    avgQuatsPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(localAvgQuats.data(), totalFeatures * 4));
  }

  std::vector<usize> centerVoxels(totalFeatures, 0);
  std::vector<float32> centerDistances(totalFeatures, 0.0f);
  std::vector<float32> centerQuats;

  const auto& featureIdsStore = featureIds.getDataStoreRef();
  const auto& phasesStore = cellPhases.getDataStoreRef();
  const auto& quatsStore = quats.getDataStoreRef();
  auto& misoStore = featureReferenceMisorientations.getDataStoreRef();

  // Mode 1: find center voxels using chunked I/O
  if(m_InputValues->ReferenceOrientation == 1)
  {
    const auto& gbDistStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->GBEuclideanDistancesArrayPath).getDataStoreRef();
    auto fidBuf = std::make_unique<int32[]>(k_ChunkTuples);
    auto distBuf = std::make_unique<float32[]>(k_ChunkTuples);

    for(usize offset = 0; offset < totalVoxels; offset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkTuples, totalVoxels - offset);
      featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(fidBuf.get(), count));
      gbDistStore.copyIntoBuffer(offset, nonstd::span<float32>(distBuf.get(), count));
      for(usize i = 0; i < count; i++)
      {
        const int32 featureId = fidBuf[i];
        if(featureId > 0 && distBuf[i] >= centerDistances[featureId])
        {
          centerDistances[featureId] = distBuf[i];
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

    centerQuats.resize(totalFeatures * 4, 0.0f);
    for(usize i = 1; i < totalFeatures; i++)
    {
      float32 qBuf[4] = {};
      quatsStore.copyIntoBuffer(centerVoxels[i] * 4, nonstd::span<float32>(qBuf, 4));
      centerQuats[i * 4 + 0] = qBuf[0];
      centerQuats[i * 4 + 1] = qBuf[1];
      centerQuats[i * 4 + 2] = qBuf[2];
      centerQuats[i * 4 + 3] = qBuf[3];
    }
  }

  std::vector<float32> avgMisorientationSums(totalFeatures, 0.0f);
  std::vector<float32> avgMisorientationCounts(totalFeatures, 0.0f);
  featureReferenceMisorientations.fill(0.0f);

  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto phasesBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto quatsBuf = std::make_unique<float32[]>(k_ChunkTuples * 4);
  auto misoBuf = std::make_unique<float32[]>(k_ChunkTuples);

  for(usize offset = 0; offset < totalVoxels; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, totalVoxels - offset);
    featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));
    phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phasesBuf.get(), count));
    quatsStore.copyIntoBuffer(offset * 4, nonstd::span<float32>(quatsBuf.get(), count * 4));
    std::fill_n(misoBuf.get(), count, 0.0f);

    for(usize i = 0; i < count; i++)
    {
      const int32 featureId = featureIdBuf[i];
      const int32 phase = phasesBuf[i];
      if(featureId > 0 && phase > 0)
      {
        const usize qi = i * 4;
        ebsdlib::QuatD q1(quatsBuf[qi], quatsBuf[qi + 1], quatsBuf[qi + 2], quatsBuf[qi + 3]);
        ebsdlib::QuatD q2;
        if(m_InputValues->ReferenceOrientation == 0)
        {
          const usize fi = static_cast<usize>(featureId) * 4;
          q2 = ebsdlib::QuatD(localAvgQuats[fi], localAvgQuats[fi + 1], localAvgQuats[fi + 2], localAvgQuats[fi + 3]);
        }
        else if(m_InputValues->ReferenceOrientation == 1)
        {
          const usize fi = static_cast<usize>(featureId) * 4;
          q2 = ebsdlib::QuatD(centerQuats[fi], centerQuats[fi + 1], centerQuats[fi + 2], centerQuats[fi + 3]);
        }

        const uint32 laueClass = localCrystalStructures[phase];
        ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass]->calculateMisorientation(q1, q2);
        const float32 misoValue = static_cast<float32>(Constants::k_RadToDegD * axisAngle[3]);
        misoBuf[i] = misoValue;
        avgMisorientationCounts[featureId]++;
        avgMisorientationSums[featureId] += misoValue;
      }
    }
    misoStore.copyFromBuffer(offset, nonstd::span<const float32>(misoBuf.get(), count));
  }

  avgReferenceMisorientation[0] = 0.0f;
  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    avgReferenceMisorientation[featureIdx] = (avgMisorientationCounts[featureIdx] == 0.0f) ? 0.0f : avgMisorientationSums[featureIdx] / avgMisorientationCounts[featureIdx];
  }
  return {};
}
