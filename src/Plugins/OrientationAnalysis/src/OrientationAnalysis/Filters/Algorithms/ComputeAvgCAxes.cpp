#include "ComputeAvgCAxes.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"

#include <EbsdLib/Core/Orientation.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

namespace
{
constexpr usize k_ChunkSize = 4096;
} // namespace

// -----------------------------------------------------------------------------
ComputeAvgCAxes::ComputeAvgCAxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeAvgCAxesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeAvgCAxes::~ComputeAvgCAxes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeAvgCAxes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
/**
 * @brief Computes the average crystallographic c-axis direction per feature for
 * hexagonal phases. Each cell's quaternion is converted to a passive rotation
 * matrix, transposed to an active rotation, and multiplied by <0,0,1> to get
 * the c-axis in the sample reference frame. A running average c-axis is
 * accumulated per feature, then normalized.
 *
 * OOC strategy: Cell-level arrays (featureIds, phases, quats) are read in
 * fixed-size chunks (k_ChunkSize tuples) via copyIntoBuffer. Feature-level
 * avgCAxes is cached entirely in a local buffer (random access by featureId
 * would cause severe OOC chunk thrashing). The final result is bulk-written
 * back to the DataStore via copyFromBuffer.
 */
Result<> ComputeAvgCAxes::operator()()
{
  // Bulk-read ensemble-level crystal structures into local memory to avoid
  // per-element OOC virtual dispatch during the cell loop
  const auto& crystalStructuresStoreRef = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath)->getDataStoreRef();
  const usize numCrystalStructures = crystalStructuresStoreRef.getSize();
  auto crystalStructuresCache = std::make_unique<uint32[]>(numCrystalStructures);
  crystalStructuresStoreRef.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.get(), numCrystalStructures));

  // Figure out if all phases are either Hexagonal-Low 6/m or Hexagonal-High 6/mmm Laue Phases
  bool allPhasesHexagonal = true;
  bool noPhasesHexagonal = true;
  for(usize i = 1; i < numCrystalStructures; ++i)
  {
    const auto crystalStructureType = crystalStructuresCache[i];
    const bool isHex = crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low;
    allPhasesHexagonal = allPhasesHexagonal && isHex;
    noPhasesHexagonal = noPhasesHexagonal && !isHex;
  }

  // If NONE of the phases are hexagonal then bail out now with an error
  if(noPhasesHexagonal)
  {
    return MakeErrorResult(-76402, "No phases that have a crystal symmetry of Hexagonal (6/mmm or 6/m) were found.");
  }

  Result<> result;

  // Throw a warning for any NON-Hex Laue Phases
  if(!allPhasesHexagonal)
  {
    result.warnings().push_back({-76403, "Non Hexagonal phases were found. All calculations for non Hexagonal phases will be skipped and a NaN value inserted."});
  }

  // DataStore references for cell-level arrays — all access goes through
  // copyIntoBuffer/copyFromBuffer to avoid per-element OOC overhead.
  const auto& featureIdsStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  const auto& quatsStoreRef = m_DataStructure.getDataAs<Float32Array>(m_InputValues->QuatsArrayPath)->getDataStoreRef();
  const auto& cellPhasesStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath)->getDataStoreRef();
  auto& avgCAxesStoreRef = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgCAxesArrayPath)->getDataStoreRef();

  const usize totalPoints = featureIdsStoreRef.getNumberOfTuples();
  const usize totalFeatures = avgCAxesStoreRef.getNumberOfTuples();

  // Cache feature-level avgCAxes entirely in a local buffer. The cell loop
  // accesses this by featureId (random order), which would cause severe OOC
  // chunk thrashing if left in the DataStore.
  const usize avgCAxesElements = totalFeatures * 3;
  auto avgCAxesCache = std::make_unique<float32[]>(avgCAxesElements);
  avgCAxesStoreRef.copyIntoBuffer(0, nonstd::span<float32>(avgCAxesCache.get(), avgCAxesElements));

  const Eigen::Vector3d cAxis{0.0f, 0.0f, 1.0f};
  Eigen::Vector3d c1{0.0f, 0.0f, 0.0f};

  auto counter = std::make_unique<int32[]>(totalFeatures);

  // Pre-allocate chunk buffers for sequential cell-level reads. These are
  // reused every iteration to avoid repeated heap allocations.
  auto featureIdsChunk = std::make_unique<int32[]>(k_ChunkSize);
  auto cellPhasesChunk = std::make_unique<int32[]>(k_ChunkSize);
  auto quatsChunk = std::make_unique<float32[]>(k_ChunkSize * 4);

  // Process cells in fixed-size chunks. Each chunk triggers one bulk read per
  // array, amortizing OOC overhead over k_ChunkSize tuples.
  usize tupleIdx = 0;
  while(tupleIdx < totalPoints)
  {
    if(m_ShouldCancel)
    {
      return result;
    }

    const usize chunkTuples = std::min(k_ChunkSize, totalPoints - tupleIdx);

    // Bulk-read this chunk of cell data (sequential access pattern, OOC-friendly)
    featureIdsStoreRef.copyIntoBuffer(tupleIdx, nonstd::span<int32>(featureIdsChunk.get(), chunkTuples));
    cellPhasesStoreRef.copyIntoBuffer(tupleIdx, nonstd::span<int32>(cellPhasesChunk.get(), chunkTuples));
    quatsStoreRef.copyIntoBuffer(tupleIdx * 4, nonstd::span<float32>(quatsChunk.get(), chunkTuples * 4));

    for(usize t = 0; t < chunkTuples; t++)
    {
      const int32 currentFeatureId = featureIdsChunk[t];
      // If the featureId for a given cell is valid ( > 0) then analyze that value
      if(currentFeatureId > 0)
      {
        const int32 currentCellPhase = cellPhasesChunk[t];                          // Get the current cell phase
        const auto crystalStructureType = crystalStructuresCache[currentCellPhase]; // Get the CrystalStructure, i.e., Laue class of the cell
        const usize cAxesIndex = 3 * static_cast<usize>(currentFeatureId);

        // Ensure the Laue class is correct, otherwise mark the values with a NaN and continue
        if(crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_High && crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_Low)
        {
          avgCAxesCache[cAxesIndex] = NAN;
          avgCAxesCache[cAxesIndex + 1] = NAN;
          avgCAxesCache[cAxesIndex + 2] = NAN;
          continue;
        }

        counter[currentFeatureId]++; // Increment the count
        const usize quatOffset = t * 4;

        // Create the 3x3 Orientation Matrix from the Quaternion. This represents a passive rotation matrix
        ebsdlib::OrientationMatrixDType oMatrix =
            ebsdlib::QuaternionDType(quatsChunk[quatOffset], quatsChunk[quatOffset + 1], quatsChunk[quatOffset + 2], quatsChunk[quatOffset + 3]).toOrientationMatrix();

        // Convert the passive rotation matrix to an active rotation matrix by taking the transpose
        // Multiply the active transformation matrix by the C-Axis (as Miller Index). This actively rotates
        // the crystallographic C-Axis (which is along the <0,0,1> direction) into the physical sample
        // reference frame
        c1 = oMatrix.transpose() * cAxis;

        // normalize so that the magnitude is 1
        c1.normalize();

        // Compute the running average c-axis and normalize the result
        Eigen::Vector3d curCAxis{0.0f, 0.0f, 0.0f};
        curCAxis[0] = avgCAxesCache[cAxesIndex] / static_cast<float32>(counter[currentFeatureId]);
        curCAxis[1] = avgCAxesCache[cAxesIndex + 1] / static_cast<float32>(counter[currentFeatureId]);
        curCAxis[2] = avgCAxesCache[cAxesIndex + 2] / static_cast<float32>(counter[currentFeatureId]);
        curCAxis.normalize();

        // Ensure that angle between the current point's sample reference frame C-Axis
        // and the running average sample C-Axis is positive
        float64 w = ImageRotationUtilities::CosBetweenVectors(c1, curCAxis);
        if(w < 0.0)
        {
          c1 *= -1.0f;
        }

        // Continue summing up the rotations
        avgCAxesCache[cAxesIndex] += static_cast<float32>(c1[0]);
        avgCAxesCache[cAxesIndex + 1] += static_cast<float32>(c1[1]);
        avgCAxesCache[cAxesIndex + 2] += static_cast<float32>(c1[2]);
      }
    }

    tupleIdx += chunkTuples;
  }

  // Normalize the accumulated c-axis values
  for(usize i = 1; i < totalFeatures; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize tupleIndex = i * 3;
    float32 avgCAxesValue = avgCAxesCache[tupleIndex];
    if(std::isnan(avgCAxesValue))
    {
      continue;
    }
    // If we got passed the last check this could happen if the cell points were
    // masked out? Maybe?
    if(counter[i] == 0)
    {
      avgCAxesCache[tupleIndex] = 0;
      avgCAxesCache[tupleIndex + 1] = 0;
      avgCAxesCache[tupleIndex + 2] = 1;
    }
    else
    {
      avgCAxesCache[tupleIndex] /= static_cast<float32>(counter[i]);
      avgCAxesCache[tupleIndex + 1] /= static_cast<float32>(counter[i]);
      avgCAxesCache[tupleIndex + 2] /= static_cast<float32>(counter[i]);
    }
  }

  // Single bulk-write of the completed feature-level avgCAxes back to the DataStore.
  // All accumulation and normalization was done in the local buffer.
  avgCAxesStoreRef.copyFromBuffer(0, nonstd::span<const float32>(avgCAxesCache.get(), avgCAxesElements));

  return result;
}
