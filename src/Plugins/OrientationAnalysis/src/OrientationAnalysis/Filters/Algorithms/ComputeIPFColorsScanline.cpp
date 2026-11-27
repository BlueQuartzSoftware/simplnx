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

// -----------------------------------------------------------------------------
Result<> ComputeIPFColorsScanline::operator()()
{
  std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();

  auto& eulers = m_DataStructure.getDataRefAs<Float32ArrayType>(m_InputValues->cell_euler_angles_array_path);
  auto& phases = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->cell_phases_array_path);
  auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32ArrayType>(m_InputValues->crystal_structures_array_path);
  auto& ipfColors = m_DataStructure.getDataRefAs<UInt8ArrayType>(m_InputValues->cell_ipf_colors_array_path);

  const usize totalPoints = eulers.getNumberOfTuples();
  const int32 numPhases = static_cast<int32>(crystalStructuresArray.getNumberOfTuples());

  // Cache the small ensemble array to avoid per-element OOC access.
  std::vector<uint32> crystalStructures(numPhases);
  if(Result<> ioResult = crystalStructuresArray.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), static_cast<usize>(numPhases))); ioResult.invalid())
  {
    return ConvertResult(std::move(ioResult));
  }

  // EbsdLib expects a normalized double reference direction.
  FloatVec3Type normRefDir = m_InputValues->reference_direction;
  normRefDir = normRefDir.normalize();
  std::array<double, 3> refDir = {normRefDir[0], normRefDir[1], normRefDir[2]};

  const IDataArray* maskArrayPtr = nullptr;
  if(m_InputValues->use_mask)
  {
    maskArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->mask_array_path);
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
  const bool hasBoolMask = maskArrayPtr != nullptr && maskArrayPtr->getDataType() == DataType::boolean;
  const bool hasUint8Mask = maskArrayPtr != nullptr && maskArrayPtr->getDataType() == DataType::uint8;
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
    if(Result<> ioResult = eulersStore.copyIntoBuffer(offset * 3, nonstd::span<float32>(eulerBuf.get(), count * 3)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
    if(Result<> ioResult = phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phasesBuf.get(), count)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }

    if(hasBoolMask)
    {
      if(Result<> ioResult = dynamic_cast<const BoolArrayType*>(maskArrayPtr)->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<bool>(boolMaskBuf.get(), count)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
    }
    else if(hasUint8Mask)
    {
      if(Result<> ioResult = dynamic_cast<const UInt8ArrayType*>(maskArrayPtr)->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<uint8>(uint8MaskBuf.get(), count)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
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

      if(calcIPF && phase < 0)
      {
        return MakeErrorResult(-48001, fmt::format("Cell Phases array '{}' has value {} at voxel index {}. Valid enabled Phase indices are in [0, {}).", m_InputValues->cell_phases_array_path.toString(),
                                                   phase, offset + i, numPhases));
      }

      if(phase >= numPhases)
      {
        phaseWarningCount++;
      }

      if(phase < numPhases && calcIPF && crystalStructures[phase] < ebsdlib::CrystalStructure::LaueGroupEnd)
      {
        RgbaType argb = ops[crystalStructures[phase]]->generateIPFColor(dEuler.data(), refDir.data(), false, m_InputValues->color_key);
        colorBuf[ci] = static_cast<uint8>(rgb_color::DRed(argb));
        colorBuf[ci + 1] = static_cast<uint8>(rgb_color::DGreen(argb));
        colorBuf[ci + 2] = static_cast<uint8>(rgb_color::DBlue(argb));
      }
    }

    if(Result<> ioResult = ipfColorsStore.copyFromBuffer(offset * 3, nonstd::span<const uint8>(colorBuf.get(), count * 3)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
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
