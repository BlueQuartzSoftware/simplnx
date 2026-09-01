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
 * @brief Specifies the fixed IPF scanline page size.
 *
 * The size bounds local buffers while reducing bulk-I/O calls. It need not match storage chunk dimensions.
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
Result<> ComputeIPFColorsScanline::operator()()
{
  std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();

  auto& eulers = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellEulerAnglesArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);
  auto& ipfColors = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->cellIpfColorsArrayPath);

  const usize totalPoints = eulers.getNumberOfTuples();
  const int32 numPhases = static_cast<int32>(crystalStructuresArray.getNumberOfTuples());

  // Cache the small ensemble array to avoid per-element OOC access.
  std::vector<uint32> crystalStructures(numPhases);
  crystalStructuresArray.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), static_cast<usize>(numPhases)));

  // EbsdLib expects a normalized double reference direction.
  FloatVec3 normRefDir = m_InputValues->referenceDirection;
  normRefDir = normRefDir.normalize();
  std::array<double, 3> refDir = {normRefDir[0], normRefDir[1], normRefDir[2]};

  const IDataArray* maskArray = nullptr;
  if(m_InputValues->useMask)
  {
    maskArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->maskArrayPath);
  }

  const auto& eulersStore = eulers.getDataStoreRef();
  const auto& phasesStore = phases.getDataStoreRef();
  auto& ipfColorsStore = ipfColors.getDataStoreRef();

  // Reused page buffers bound memory and avoid repeated allocation.
  auto eulerBuf = std::make_unique<float32[]>(k_ChunkTuples * 3);
  auto phasesBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto colorBuf = std::make_unique<uint8[]>(k_ChunkTuples * 3);

  std::unique_ptr<bool[]> boolMaskBuf;
  std::unique_ptr<uint8[]> uint8MaskBuf;
  const bool hasBoolMask = maskArray != nullptr && maskArray->getDataType() == DataType::boolean;
  const bool hasUint8Mask = maskArray != nullptr && maskArray->getDataType() == DataType::uint8;
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

  for(usize offset = 0; offset < totalPoints; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkTuples, totalPoints - offset);

    // Euler buffers use three values per tuple.
    eulersStore.copyIntoBuffer(offset * 3, nonstd::span<float32>(eulerBuf.get(), count * 3));
    phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phasesBuf.get(), count));

    if(hasBoolMask)
    {
      dynamic_cast<const BoolArray*>(maskArray)->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<bool>(boolMaskBuf.get(), count));
    }
    else if(hasUint8Mask)
    {
      dynamic_cast<const UInt8Array*>(maskArray)->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<uint8>(uint8MaskBuf.get(), count));
    }

    for(usize i = 0; i < count; i++)
    {
      const usize ci = i * 3;
      colorBuf[ci] = 0;
      colorBuf[ci + 1] = 0;
      colorBuf[ci + 2] = 0;

      const int32 phase = phasesBuf[i];
      dEuler[0] = eulerBuf[ci];
      dEuler[1] = eulerBuf[ci + 1];
      dEuler[2] = eulerBuf[ci + 2];

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

      if(phase < numPhases && calcIPF && crystalStructures[phase] < ebsdlib::CrystalStructure::LaueGroupEnd)
      {
        Rgba argb = ops[crystalStructures[phase]]->generateIPFColor(dEuler.data(), refDir.data(), false, m_InputValues->colorKey);
        colorBuf[ci] = static_cast<uint8>(RgbColor::dRed(argb));
        colorBuf[ci + 1] = static_cast<uint8>(RgbColor::dGreen(argb));
        colorBuf[ci + 2] = static_cast<uint8>(RgbColor::dBlue(argb));
      }
    }

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
