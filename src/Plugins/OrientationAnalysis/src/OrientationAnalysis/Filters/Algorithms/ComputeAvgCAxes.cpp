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
  const auto& crystalStructuresStoreRef = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath)->getDataStoreRef();
  const usize numCrystalStructures = crystalStructuresStoreRef.getSize();
  auto crystalStructuresCache = std::make_unique<uint32[]>(numCrystalStructures);
  crystalStructuresStoreRef.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.get(), numCrystalStructures));

  bool allPhasesHexagonal = true;
  bool noPhasesHexagonal = true;
  for(usize i = 1; i < numCrystalStructures; ++i)
  {
    const auto crystalStructureType = crystalStructuresCache[i];
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

  // Cell data uses chunked bulk I/O to avoid per-element OOC access.
  const auto& featureIdsStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  const auto& quatsStoreRef = m_DataStructure.getDataAs<Float32Array>(m_InputValues->QuatsArrayPath)->getDataStoreRef();
  const auto& cellPhasesStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath)->getDataStoreRef();
  auto& avgCAxesStoreRef = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgCAxesArrayPath)->getDataStoreRef();

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

    featureIdsStoreRef.copyIntoBuffer(tupleIdx, nonstd::span<int32>(featureIdsChunk.get(), chunkTuples));
    cellPhasesStoreRef.copyIntoBuffer(tupleIdx, nonstd::span<int32>(cellPhasesChunk.get(), chunkTuples));
    quatsStoreRef.copyIntoBuffer(tupleIdx * 4, nonstd::span<float32>(quatsChunk.get(), chunkTuples * 4));

    for(usize t = 0; t < chunkTuples; t++)
    {
      const int32 currentFeatureId = featureIdsChunk[t];
      if(currentFeatureId > 0)
      {
        const int32 currentCellPhase = cellPhasesChunk[t];                          // Get the current cell phase
        const auto crystalStructureType = crystalStructuresCache[currentCellPhase]; // Get the CrystalStructure, i.e., Laue class of the cell
        const usize cAxesIndex = 3 * static_cast<usize>(currentFeatureId);

        // Skip non-hexagonal cells so mixed features retain valid contributions.
        if(crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_High && crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_Low)
        {
          continue;
        }

        counter[currentFeatureId]++;
        const usize quatOffset = t * 4;

        ebsdlib::OrientationMatrixDType oMatrix =
            ebsdlib::QuaternionDType(quatsChunk[quatOffset], quatsChunk[quatOffset + 1], quatsChunk[quatOffset + 2], quatsChunk[quatOffset + 3]).toOrientationMatrix();

        // The transposed matrix maps crystal [001] into the sample frame.
        c1 = oMatrix.transpose() * cAxis;

        c1.normalize();

        Eigen::Vector3d curCAxis{0.0f, 0.0f, 0.0f};
        curCAxis[0] = avgCAxesCache[cAxesIndex] / static_cast<float32>(counter[currentFeatureId]);
        curCAxis[1] = avgCAxesCache[cAxesIndex + 1] / static_cast<float32>(counter[currentFeatureId]);
        curCAxis[2] = avgCAxesCache[cAxesIndex + 2] / static_cast<float32>(counter[currentFeatureId]);
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

  for(usize i = 0; i < totalFeatures; i++)
  {
    if(m_ShouldCancel)
    {
      return result;
    }

    const usize tupleIndex = i * 3;
    if(counter[i] == 0)
    {
      // Features without a hexagonal contribution have no c-axis average.
      avgCAxesCache[tupleIndex] = NAN;
      avgCAxesCache[tupleIndex + 1] = NAN;
      avgCAxesCache[tupleIndex + 2] = NAN;
    }
    else
    {
      // Antipodal flips keep the accumulated direction away from zero.
      Eigen::Vector3d finalAvg{avgCAxesCache[tupleIndex] / static_cast<float64>(counter[i]), avgCAxesCache[tupleIndex + 1] / static_cast<float64>(counter[i]),
                               avgCAxesCache[tupleIndex + 2] / static_cast<float64>(counter[i])};
      finalAvg.normalize();
      avgCAxesCache[tupleIndex] = static_cast<float32>(finalAvg[0]);
      avgCAxesCache[tupleIndex + 1] = static_cast<float32>(finalAvg[1]);
      avgCAxesCache[tupleIndex + 2] = static_cast<float32>(finalAvg[2]);
    }
  }

  avgCAxesStoreRef.copyFromBuffer(0, nonstd::span<const float32>(avgCAxesCache.get(), avgCAxesElements));

  return result;
}
