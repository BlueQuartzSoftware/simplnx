#include "ComputeIPFColorsDirect.hpp"

#include "ComputeIPFColors.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <utility>

using namespace nx::core;

namespace
{

/**
 * @class ComputeIPFColorsImpl
 * @brief Computes IPF colors for one disjoint tuple range.
 *
 * The direct executor uses this worker. requireArraysInMemory() can select
 * serial execution when a listed array is not in-memory. This worker gives no
 * generic DataArray or DataStore thread-safety guarantee.
 */
class ComputeIPFColorsImpl
{
public:
  ComputeIPFColorsImpl(ComputeIPFColorsDirect* filter, nx::core::FloatVec3Type referenceDir, nx::core::Float32ArrayType& eulers, nx::core::Int32ArrayType& phases,
                       nx::core::UInt32ArrayType& crystalStructures, int32_t numPhases, const nx::core::IDataArray* goodVoxels, nx::core::UInt8ArrayType& colors, ebsdlib::ColorKeyKind colorKey)
  : m_Filter(filter)
  , m_ReferenceDir(std::move(referenceDir))
  , m_CellEulerAngles(eulers.getDataStoreRef())
  , m_CellPhases(phases.getDataStoreRef())
  , m_CrystalStructures(crystalStructures.getDataStoreRef())
  , m_NumPhases(numPhases)
  , m_GoodVoxels(goodVoxels)
  , m_CellIPFColors(colors.getDataStoreRef())
  , m_ColorKey(colorKey)
  {
  }

  virtual ~ComputeIPFColorsImpl() = default;

  /**
   * @brief Computes IPF colors for voxels in the range [start, end).
   *
   * Templated on the mask element type (bool or uint8) because the mask array
   * can be either DataType::boolean or DataType::uint8. When no mask is provided,
   * the bool specialization is used with a nullptr maskArray, which causes all
   * voxels to be computed.
   *
   * @tparam T Element type of the mask array (bool or uint8).
   * @param start First tuple index (inclusive).
   * @param end Last tuple index (exclusive).
   */
  template <typename T>
  void convert(size_t start, size_t end) const
  {
    using MaskArrayType = DataArray<T>;
    const MaskArrayType* maskArrayPtr = nullptr;
    if(nullptr != m_GoodVoxels)
    {
      maskArrayPtr = dynamic_cast<const MaskArrayType*>(m_GoodVoxels);
    }

    // Each worker owns its LaueOps instances.
    std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();
    std::array<double, 3> refDir = {m_ReferenceDir[0], m_ReferenceDir[1], m_ReferenceDir[2]};
    std::array<double, 3> dEuler = {0.0, 0.0, 0.0};
    RgbaType argb = 0x00000000;
    int32_t phase = 0;
    bool calcIPF = false;
    size_t index = 0;
    for(size_t i = start; i < end; i++)
    {
      if(m_Filter->shouldCancel())
      {
        return;
      }
      phase = m_CellPhases[i];
      // Each voxel's color is stored as 3 consecutive uint8 values (R, G, B).
      // Default to black (0, 0, 0); overwritten only if orientation is valid.
      index = i * 3;
      m_CellIPFColors.setValue(index, 0);
      m_CellIPFColors.setValue(index + 1, 0);
      m_CellIPFColors.setValue(index + 2, 0);

      // Read the three Euler angles (phi1, Phi, phi2) in radians for this voxel.
      dEuler[0] = m_CellEulerAngles.getValue(index);
      dEuler[1] = m_CellEulerAngles.getValue(index + 1);
      dEuler[2] = m_CellEulerAngles.getValue(index + 2);

      // If a mask is active, skip voxels marked as bad (mask == false/0).
      calcIPF = true;
      if(nullptr != maskArrayPtr)
      {
        calcIPF = (*maskArrayPtr)[i];
      }
      // Guard against phase IDs that exceed the crystal structures array length.
      // This indicates corrupt input data; we count occurrences for a post-run warning.
      if(phase >= m_NumPhases)
      {
        m_Filter->incrementPhaseWarningCount();
      }

      // Compute the IPF color only if:
      //  1. Phase ID is within the ensemble array bounds.
      //  2. The mask allows computation (or no mask is used).
      //  3. The crystal structure is a recognized Laue group (not Unknown).
      if(phase < m_NumPhases && calcIPF && m_CrystalStructures[phase] < ebsdlib::CrystalStructure::LaueGroupEnd)
      {
        // Euler angles are already radians, so generateIPFColor() does not convert them.
        argb = ops[m_CrystalStructures[phase]]->generateIPFColor(dEuler.data(), refDir.data(), false, m_ColorKey);
        m_CellIPFColors.setValue(index, static_cast<uint8_t>(nx::core::rgb_color::DRed(argb)));
        m_CellIPFColors.setValue(index + 1, static_cast<uint8_t>(nx::core::rgb_color::DGreen(argb)));
        m_CellIPFColors.setValue(index + 2, static_cast<uint8_t>(nx::core::rgb_color::DBlue(argb)));
      }
    }
  }

  void run(size_t start, size_t end) const
  {
    if(m_GoodVoxels != nullptr)
    {
      if(m_GoodVoxels->getDataType() == DataType::boolean)
      {
        convert<bool>(start, end);
      }
      else if(m_GoodVoxels->getDataType() == DataType::uint8)
      {
        convert<uint8>(start, end);
      }
    }
    else
    {
      // No mask provided -- compute IPF color for every voxel.
      convert<bool>(start, end);
    }
  }

  void operator()(const Range& range) const
  {
    run(range.min(), range.max());
  }

private:
  ComputeIPFColorsDirect* m_Filter = nullptr;
  nx::core::FloatVec3Type m_ReferenceDir;
  nx::core::Float32AbstractDataStoreType& m_CellEulerAngles;
  nx::core::Int32AbstractDataStoreType& m_CellPhases;
  nx::core::UInt32AbstractDataStoreType& m_CrystalStructures;
  int32_t m_NumPhases = 0;
  const nx::core::IDataArray* m_GoodVoxels = nullptr;
  nx::core::UInt8AbstractDataStoreType& m_CellIPFColors;
  ebsdlib::ColorKeyKind m_ColorKey = ebsdlib::ColorKeyKind::TSL;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeIPFColorsDirect::ComputeIPFColorsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               const ComputeIPFColorsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(mesgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
Result<> ComputeIPFColorsDirect::operator()()
{
  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  // Retrieve typed array references. These are guaranteed to exist because
  // the filter's preflightImpl() validated them via selection parameters.
  nx::core::Float32ArrayType& eulers = m_DataStructure.getDataRefAs<Float32ArrayType>(m_InputValues->cell_euler_angles_array_path);
  nx::core::Int32ArrayType& phases = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->cell_phases_array_path);
  nx::core::UInt32ArrayType& crystalStructures = m_DataStructure.getDataRefAs<UInt32ArrayType>(m_InputValues->crystal_structures_array_path);
  nx::core::UInt8ArrayType& ipfColors = m_DataStructure.getDataRefAs<UInt8ArrayType>(m_InputValues->cell_ipf_colors_array_path);

  m_PhaseWarningCount = 0;
  size_t totalPoints = eulers.getNumberOfTuples();

  // The crystal structures array has one entry per phase (including phase 0 = unknown).
  // numPhases is used as an upper bound for phase ID validation.
  int32_t numPhases = static_cast<int32_t>(crystalStructures.getNumberOfTuples());

  // Normalize the reference direction to a unit vector. The user may supply
  // any non-zero vector (e.g., [0,0,1] for the sample Z axis).
  nx::core::FloatVec3Type normRefDir = m_InputValues->reference_direction;
  normRefDir = normRefDir.normalize();

  // List every worker array so requireArraysInMemory() disables parallel
  // scheduling when any listed array is not in-memory.
  typename IParallelAlgorithm::AlgorithmArraysType algArrays;
  algArrays.push_back(&eulers);
  algArrays.push_back(&phases);
  algArrays.push_back(&crystalStructures);
  algArrays.push_back(&ipfColors);

  nx::core::IDataArray* maskArrayPtr = nullptr;
  if(m_InputValues->use_mask)
  {
    maskArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->mask_array_path);
    algArrays.push_back(maskArrayPtr);
  }

  const auto validateNegativePhases = [&](const auto* typedMaskArrayPtr) -> Result<> {
    const auto& cellPhasesStoreRef = phases.getDataStoreRef();
    for(usize voxelIdx = 0; voxelIdx < totalPoints; voxelIdx++)
    {
      const bool calculateIpf = typedMaskArrayPtr == nullptr || static_cast<bool>((*typedMaskArrayPtr)[voxelIdx]);
      const int32 currentPhaseIdx = cellPhasesStoreRef[voxelIdx];
      if(calculateIpf && currentPhaseIdx < 0)
      {
        return MakeErrorResult(-48001, fmt::format("Cell Phases array '{}' has value {} at voxel index {}. Valid enabled Phase indices are in [0, {}).",
                                                   m_InputValues->cell_phases_array_path.toString(), currentPhaseIdx, voxelIdx, numPhases));
      }
    }
    return {};
  };

  Result<> validationResult;
  if(maskArrayPtr == nullptr)
  {
    validationResult = validateNegativePhases(static_cast<const BoolArrayType*>(nullptr));
  }
  else if(maskArrayPtr->getDataType() == DataType::boolean)
  {
    validationResult = validateNegativePhases(m_DataStructure.getDataAs<BoolArrayType>(m_InputValues->mask_array_path));
  }
  else
  {
    validationResult = validateNegativePhases(m_DataStructure.getDataAs<UInt8ArrayType>(m_InputValues->mask_array_path));
  }
  if(validationResult.invalid())
  {
    return validationResult;
  }

  // The executor processes contiguous tuple ranges. A nonresident listed array
  // makes requireArraysInMemory() select serial execution.
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, totalPoints);
  dataAlg.requireArraysInMemory(algArrays);

  dataAlg.execute(ComputeIPFColorsImpl(this, normRefDir, eulers, phases, crystalStructures, numPhases, maskArrayPtr, ipfColors, m_InputValues->color_key));

  // After execution, check whether any phase IDs exceeded the ensemble bounds.
  if(m_PhaseWarningCount > 0)
  {
    std::string message = fmt::format("The Ensemble Phase information only references {} phase(s) but {} cell(s) had a phase value greater than {}. \
This indicates a problem with the input cell phase data. DREAM3D-NX will give INCORRECT RESULTS.",
                                      (numPhases - 1), m_PhaseWarningCount.load(), (numPhases - 1));

    return nx::core::MakeErrorResult(-48000, message);
  }

  return {};
}

// -----------------------------------------------------------------------------
void ComputeIPFColorsDirect::incrementPhaseWarningCount()
{
  ++m_PhaseWarningCount;
}

bool ComputeIPFColorsDirect::shouldCancel() const
{
  return m_ShouldCancel;
}
