#include "ComputeIPFColorsScanline.hpp"

#include "ComputeIPFColors.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <nonstd/span.hpp>

#include <memory>

using namespace nx::core;

namespace
{
/**
 * @brief Number of tuples processed per chunk in the scanline loop.
 *
 * 65,536 tuples is chosen as a balance between minimizing the number of
 * copyIntoBuffer()/copyFromBuffer() round-trips and keeping the per-chunk
 * heap allocation small (~768 KB for 3-component float32 Euler angles).
 * This value does NOT need to align with the OOC chunk size; the DataStore's
 * bulk I/O methods handle partial and cross-chunk reads internally.
 */
constexpr usize k_ChunkTuples = 65536;
} // namespace

// -----------------------------------------------------------------------------
ComputeIPFColorsScanline::ComputeIPFColorsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   const ComputeIPFColorsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(mesgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
ComputeIPFColorsScanline::~ComputeIPFColorsScanline() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief OOC-safe IPF color computation using sequential chunk-based bulk I/O.
 *
 * The algorithm processes the entire tuple range in fixed-size chunks of
 * k_ChunkTuples. For each chunk:
 *   1. Bulk-read Euler angles, phase IDs, and (optionally) mask values from
 *      the OOC-backed DataStores into local heap buffers.
 *   2. Compute IPF colors for every tuple in the chunk (same LaueOps logic
 *      as the Direct path).
 *   3. Bulk-write the computed RGB colors back to the output DataStore.
 *
 * This linear access pattern ensures that each OOC disk chunk is read at most
 * once per pass, avoiding the random-access chunk thrashing that would occur
 * if ParallelDataAlgorithm workers accessed the OOC stores concurrently.
 */
Result<> ComputeIPFColorsScanline::operator()()
{
  // Create all LaueOps instances upfront. The vector is indexed by crystal
  // structure enum (e.g., Cubic_High = 1, Hexagonal_High = 2, etc.).
  std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();

  auto& eulers = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellEulerAnglesArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);
  auto& ipfColors = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->cellIpfColorsArrayPath);

  const usize totalPoints = eulers.getNumberOfTuples();
  const int32 numPhases = static_cast<int32>(crystalStructuresArray.getNumberOfTuples());

  // Cache crystal structures locally. This ensemble-level array is tiny (one entry
  // per phase) so it is always worth copying into a contiguous local vector to
  // avoid per-element OOC access during the inner loop.
  std::vector<uint32> crystalStructures(numPhases);
  crystalStructuresArray.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), static_cast<usize>(numPhases)));

  // Normalize the reference direction to a unit vector and promote to double
  // for compatibility with EbsdLib's generateIPFColor() API.
  FloatVec3 normRefDir = m_InputValues->referenceDirection;
  normRefDir = normRefDir.normalize();
  std::array<double, 3> refDir = {normRefDir[0], normRefDir[1], normRefDir[2]};

  // Optional mask array -- only retrieved if the user opted into masking.
  const IDataArray* goodVoxelsArray = nullptr;
  if(m_InputValues->useGoodVoxels)
  {
    goodVoxelsArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->goodVoxelsArrayPath);
  }

  // Obtain DataStore references for bulk I/O. We never use operator[] on these
  // stores; all access goes through copyIntoBuffer()/copyFromBuffer().
  const auto& eulersStore = eulers.getDataStoreRef();
  const auto& phasesStore = phases.getDataStoreRef();
  auto& ipfColorsStore = ipfColors.getDataStoreRef();

  // Allocate fixed-size chunk buffers on the heap. These persist across chunks
  // to avoid repeated allocation/deallocation.
  //   eulerBuf:  k_ChunkTuples * 3 float32  (~768 KB)
  //   phasesBuf: k_ChunkTuples * 1 int32     (~256 KB)
  //   colorBuf:  k_ChunkTuples * 3 uint8     (~192 KB)
  auto eulerBuf = std::make_unique<float32[]>(k_ChunkTuples * 3);
  auto phasesBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto colorBuf = std::make_unique<uint8[]>(k_ChunkTuples * 3);

  // Mask buffer -- only allocated for the mask type that is actually in use,
  // to avoid wasting memory when no mask is needed.
  std::unique_ptr<bool[]> boolMaskBuf;
  std::unique_ptr<uint8[]> uint8MaskBuf;
  const bool hasBoolMask = goodVoxelsArray != nullptr && goodVoxelsArray->getDataType() == DataType::boolean;
  const bool hasUint8Mask = goodVoxelsArray != nullptr && goodVoxelsArray->getDataType() == DataType::uint8;
  if(hasBoolMask)
  {
    boolMaskBuf = std::make_unique<bool[]>(k_ChunkTuples);
  }
  else if(hasUint8Mask)
  {
    uint8MaskBuf = std::make_unique<uint8[]>(k_ChunkTuples);
  }

  int32 phaseWarningCount = 0;
  std::array<double, 3> dEuler = {0.0, 0.0, 0.0};

  // ---- Main scanline loop: process k_ChunkTuples tuples per iteration ----
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    // The last chunk may be smaller than k_ChunkTuples.
    const usize count = std::min(k_ChunkTuples, totalPoints - offset);

    // Bulk-read cell-level data for this chunk. The copyIntoBuffer() calls
    // issue sequential reads against the underlying OOC store, which reads
    // disk chunks in order and avoids thrashing.
    // Note: Euler angles have 3 components per tuple, so the element offset
    // and span size are multiplied by 3.
    eulersStore.copyIntoBuffer(offset * 3, nonstd::span<float32>(eulerBuf.get(), count * 3));
    phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phasesBuf.get(), count));

    // Read the mask chunk if applicable. The mask type is determined once
    // before the loop and the appropriate buffer is used.
    if(hasBoolMask)
    {
      dynamic_cast<const BoolArray*>(goodVoxelsArray)->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<bool>(boolMaskBuf.get(), count));
    }
    else if(hasUint8Mask)
    {
      dynamic_cast<const UInt8Array*>(goodVoxelsArray)->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<uint8>(uint8MaskBuf.get(), count));
    }

    // Process every tuple in the chunk. This inner loop operates entirely on
    // local heap buffers with no OOC store access -- all the I/O happened above.
    for(usize i = 0; i < count; i++)
    {
      const usize ci = i * 3;
      // Default color is black (R=0, G=0, B=0) for bad/unindexed voxels.
      colorBuf[ci] = 0;
      colorBuf[ci + 1] = 0;
      colorBuf[ci + 2] = 0;

      const int32 phase = phasesBuf[i];
      dEuler[0] = eulerBuf[ci];
      dEuler[1] = eulerBuf[ci + 1];
      dEuler[2] = eulerBuf[ci + 2];

      // Apply mask: skip voxels where the mask indicates bad data.
      bool calcIPF = true;
      if(hasBoolMask)
      {
        calcIPF = boolMaskBuf[i];
      }
      else if(hasUint8Mask)
      {
        calcIPF = uint8MaskBuf[i] != 0;
      }

      if(phase >= numPhases)
      {
        phaseWarningCount++;
      }

      // Compute the IPF color only for valid, unmasked voxels with a recognized
      // crystal structure. The generateIPFColor() call is the same as in the
      // Direct path -- the only difference is that the data was pre-loaded from
      // an OOC store via bulk I/O instead of accessed through random AbstractDataStore calls.
      if(phase < numPhases && calcIPF && crystalStructures[phase] < ebsdlib::CrystalStructure::LaueGroupEnd)
      {
        Rgba argb = ops[crystalStructures[phase]]->generateIPFColor(dEuler.data(), refDir.data(), false);
        colorBuf[ci] = static_cast<uint8>(RgbColor::dRed(argb));
        colorBuf[ci + 1] = static_cast<uint8>(RgbColor::dGreen(argb));
        colorBuf[ci + 2] = static_cast<uint8>(RgbColor::dBlue(argb));
      }
    }

    // Bulk-write the computed colors back to the output store. Like the reads,
    // this is a sequential write that aligns with the OOC chunk layout.
    ipfColorsStore.copyFromBuffer(offset * 3, nonstd::span<const uint8>(colorBuf.get(), count * 3));
  }

  if(phaseWarningCount > 0)
  {
    std::string message = fmt::format("The Ensemble Phase information only references {} phase(s) but {} cell(s) had a phase value greater than {}. "
                                      "This indicates a problem with the input cell phase data. DREAM3D-NX will give INCORRECT RESULTS.",
                                      (numPhases - 1), phaseWarningCount, (numPhases - 1));

    return nx::core::MakeErrorResult(-48000, message);
  }

  return {};
}
