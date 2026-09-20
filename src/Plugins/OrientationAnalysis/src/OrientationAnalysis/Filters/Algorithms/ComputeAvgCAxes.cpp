#include "ComputeAvgCAxes.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"

#include <EbsdLib/Core/Orientation.hpp>

#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>
#include <algorithm>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

namespace
{
constexpr usize k_ChunkSize = 4096;
} // namespace

ComputeAvgCAxes::ComputeAvgCAxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeAvgCAxesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeAvgCAxes::~ComputeAvgCAxes() noexcept = default;

Result<> ComputeAvgCAxes::operator()()
{
  // The local ensemble cache avoids cell-loop store access.
  const auto& crystalStructuresStoreRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath).getDataStoreRef();
  const usize numCrystalStructures = crystalStructuresStoreRef.getSize();
  auto crystalStructuresCache = std::make_unique<uint32[]>(numCrystalStructures);
  if(Result<> ioResult = crystalStructuresStoreRef.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.get(), numCrystalStructures)); ioResult.invalid())
  {
    return ConvertResult(std::move(ioResult));
  }

  bool allPhasesHexagonal = true;
  bool noPhasesHexagonal = true;
  for(usize phaseIdx = 1; phaseIdx < numCrystalStructures; ++phaseIdx)
  {
    const auto crystalStructureType = crystalStructuresCache[phaseIdx];
    const bool isHex = crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low;
    allPhasesHexagonal = allPhasesHexagonal && isHex;
    noPhasesHexagonal = noPhasesHexagonal && !isHex;
  }

  if(noPhasesHexagonal)
  {
    return MakeErrorResult(-76402, "No phases that have a crystal symmetry of Hexagonal (6/mmm or 6/m) were found.");
  }

  Result<> result;

  if(!allPhasesHexagonal)
  {
    result.warnings().push_back({-76403, "Non Hexagonal phases were found. All calculations for non Hexagonal phases will be skipped and a NaN value inserted."});
  }
  auto mergeWarnings = [&result](Result<> ioResult) { return MergeResults(std::move(result), std::move(ioResult)); };

  // Cell data uses chunked bulk I/O to avoid per-element OOC access.
  const auto& featureIdsStoreRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& quatsStoreRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath).getDataStoreRef();
  const auto& cellPhasesStoreRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();
  auto& avgCAxesStoreRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgCAxesArrayPath).getDataStoreRef();

  const usize totalPoints = featureIdsStoreRef.getNumberOfTuples();
  const usize totalFeatures = avgCAxesStoreRef.getNumberOfTuples();

  // Feature IDs select output tuples in random order, so accumulation stays
  // local until the final write.
  const usize avgCAxesElements = totalFeatures * 3;
  auto avgCAxesCache = std::make_unique<float32[]>(avgCAxesElements);
  std::fill_n(avgCAxesCache.get(), avgCAxesElements, 0.0f);

  const Eigen::Vector3d cAxis{0.0f, 0.0f, 1.0f};
  Eigen::Vector3d c1{0.0f, 0.0f, 0.0f};

  auto counter = std::make_unique<int32[]>(totalFeatures);

  auto featureIdsChunk = std::make_unique<int32[]>(k_ChunkSize);
  auto cellPhasesChunk = std::make_unique<int32[]>(k_ChunkSize);
  auto quatsChunk = std::make_unique<float32[]>(k_ChunkSize * 4);

  m_MessageHandler({IFilter::Message::Type::Info, "Computing cell contributions"});

  usize tupleIdx = 0;
  while(tupleIdx < totalPoints)
  {
    if(m_ShouldCancel)
    {
      return result;
    }

    const usize chunkTuples = std::min(k_ChunkSize, totalPoints - tupleIdx);

    if(Result<> ioResult = featureIdsStoreRef.copyIntoBuffer(tupleIdx, nonstd::span<int32>(featureIdsChunk.get(), chunkTuples)); ioResult.invalid())
    {
      return mergeWarnings(std::move(ioResult));
    }
    if(Result<> ioResult = cellPhasesStoreRef.copyIntoBuffer(tupleIdx, nonstd::span<int32>(cellPhasesChunk.get(), chunkTuples)); ioResult.invalid())
    {
      return mergeWarnings(std::move(ioResult));
    }
    if(Result<> ioResult = quatsStoreRef.copyIntoBuffer(tupleIdx * 4, nonstd::span<float32>(quatsChunk.get(), chunkTuples * 4)); ioResult.invalid())
    {
      return mergeWarnings(std::move(ioResult));
    }

    for(usize chunkTupleIdx = 0; chunkTupleIdx < chunkTuples; chunkTupleIdx++)
    {
      const int32 currentFeatureIdx = featureIdsChunk[chunkTupleIdx];
      if(currentFeatureIdx > 0)
      {
        const int32 currentPhaseIdx = cellPhasesChunk[chunkTupleIdx];
        if(currentPhaseIdx < 0 || static_cast<usize>(currentPhaseIdx) >= numCrystalStructures)
        {
          return MakeErrorResult(-76404, fmt::format("Cell Phases array '{}' has value {} at voxel index {}, but Crystal Structures array '{}' has {} tuples. Valid Phase indices are in [0, {}).",
                                                     m_InputValues->CellPhasesArrayPath.toString(), currentPhaseIdx, tupleIdx + chunkTupleIdx, m_InputValues->CrystalStructuresArrayPath.toString(),
                                                     numCrystalStructures, numCrystalStructures));
        }
        const auto crystalStructureType = crystalStructuresCache[currentPhaseIdx];
        const usize cAxesIndex = 3 * static_cast<usize>(currentFeatureIdx);

        // Skip non-hexagonal cells so mixed features retain valid contributions.
        if(crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_High && crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_Low)
        {
          continue;
        }

        counter[currentFeatureIdx]++;
        const usize quatOffset = chunkTupleIdx * 4;

        ebsdlib::OrientationMatrixDType oMatrix =
            ebsdlib::QuaternionDType(quatsChunk[quatOffset], quatsChunk[quatOffset + 1], quatsChunk[quatOffset + 2], quatsChunk[quatOffset + 3]).toOrientationMatrix();

        // The transposed matrix maps crystal [001] into the sample frame.
        c1 = oMatrix.transpose() * cAxis;

        c1.normalize();

        Eigen::Vector3d curCAxis{0.0f, 0.0f, 0.0f};
        curCAxis[0] = avgCAxesCache[cAxesIndex] / static_cast<float32>(counter[currentFeatureIdx]);
        curCAxis[1] = avgCAxesCache[cAxesIndex + 1] / static_cast<float32>(counter[currentFeatureIdx]);
        curCAxis[2] = avgCAxesCache[cAxesIndex + 2] / static_cast<float32>(counter[currentFeatureIdx]);
        curCAxis.normalize();

        // Antiparallel c axes represent the same hexagonal direction.
        float64 w = ImageRotationUtilities::CosBetweenVectors(c1, curCAxis);
        if(w < 0.0)
        {
          c1 *= -1.0f;
        }

        avgCAxesCache[cAxesIndex] += static_cast<float32>(c1[0]);
        avgCAxesCache[cAxesIndex + 1] += static_cast<float32>(c1[1]);
        avgCAxesCache[cAxesIndex + 2] += static_cast<float32>(c1[2]);
      }
    }

    tupleIdx += chunkTuples;
  }

  m_MessageHandler({IFilter::Message::Type::Info, "Computing final feature average C-Axis values"});

  for(usize featureIdx = 0; featureIdx < totalFeatures; featureIdx++)
  {
    if(m_ShouldCancel)
    {
      return result;
    }

    const usize tupleIndex = featureIdx * 3;
    if(counter[featureIdx] == 0)
    {
      // Features without a hexagonal contribution have no c-axis average.
      avgCAxesCache[tupleIndex] = NAN;
      avgCAxesCache[tupleIndex + 1] = NAN;
      avgCAxesCache[tupleIndex + 2] = NAN;
    }
    else
    {
      // Antipodal flips keep the accumulated direction away from zero.
      Eigen::Vector3d finalAvg{avgCAxesCache[tupleIndex] / static_cast<float64>(counter[featureIdx]), avgCAxesCache[tupleIndex + 1] / static_cast<float64>(counter[featureIdx]),
                               avgCAxesCache[tupleIndex + 2] / static_cast<float64>(counter[featureIdx])};
      finalAvg.normalize();
      avgCAxesCache[tupleIndex] = static_cast<float32>(finalAvg[0]);
      avgCAxesCache[tupleIndex + 1] = static_cast<float32>(finalAvg[1]);
      avgCAxesCache[tupleIndex + 2] = static_cast<float32>(finalAvg[2]);
    }
  }

  if(Result<> ioResult = avgCAxesStoreRef.copyFromBuffer(0, nonstd::span<const float32>(avgCAxesCache.get(), avgCAxesElements)); ioResult.invalid())
  {
    return mergeWarnings(std::move(ioResult));
  }

  return result;
}
