#include "ComputeIPFColorsDirect.hpp"

#include "ComputeIPFColors.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/OrientationFwd.hpp>

using namespace nx::core;

namespace
{

/**
 * @class ComputeIPFColorsImpl
 * @brief Threaded worker for computing IPF colors on a per-voxel range.
 *
 * Each instance is invoked by ParallelDataAlgorithm on a disjoint [start, end) tuple
 * range. The worker reads Euler angles, looks up the crystal symmetry for the voxel's
 * phase, and calls EbsdLib's LaueOps::generateIPFColor() to map the reference direction
 * into the crystal frame and obtain an RGB color on the inverse pole figure triangle.
 *
 * This worker holds AbstractDataStore references, which is safe ONLY when all stores
 * are in-core (contiguous memory). For OOC-backed stores, see ComputeIPFColorsScanline.
 */
class ComputeIPFColorsImpl
{
public:
  ComputeIPFColorsImpl(ComputeIPFColorsDirect* filter, nx::core::FloatVec3 referenceDir, nx::core::Float32Array& eulers, nx::core::Int32Array& phases, nx::core::UInt32Array& crystalStructures,
                       int32_t numPhases, const nx::core::IDataArray* goodVoxels, nx::core::UInt8Array& colors)
  : m_Filter(filter)
  , m_ReferenceDir(referenceDir)
  , m_CellEulerAngles(eulers.getDataStoreRef())
  , m_CellPhases(phases.getDataStoreRef())
  , m_CrystalStructures(crystalStructures.getDataStoreRef())
  , m_NumPhases(numPhases)
  , m_GoodVoxels(goodVoxels)
  , m_CellIPFColors(colors.getDataStoreRef())
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
    const MaskArrayType* maskArray = nullptr;
    if(nullptr != m_GoodVoxels)
    {
      maskArray = dynamic_cast<const MaskArrayType*>(m_GoodVoxels);
    }

    // Create thread-local copies of LaueOps to avoid sharing mutable state.
    // Each LaueOps instance encapsulates the symmetry operators for one Laue class.
    std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();
    std::array<double, 3> refDir = {m_ReferenceDir[0], m_ReferenceDir[1], m_ReferenceDir[2]};
    std::array<double, 3> dEuler = {0.0, 0.0, 0.0};
    Rgba argb = 0x00000000;
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
      if(nullptr != maskArray)
      {
        calcIPF = (*maskArray)[i];
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
        // generateIPFColor() transforms refDir into the crystal frame using the
        // orientation defined by the Euler angles, then maps the resulting crystal
        // direction to an RGB color on the stereographic triangle for the crystal's
        // Laue class. The 'false' parameter disables the conversion from radians
        // (our Euler angles are already in radians).
        argb = ops[m_CrystalStructures[phase]]->generateIPFColor(dEuler.data(), refDir.data(), false);
        m_CellIPFColors.setValue(index, static_cast<uint8_t>(nx::core::RgbColor::dRed(argb)));
        m_CellIPFColors.setValue(index + 1, static_cast<uint8_t>(nx::core::RgbColor::dGreen(argb)));
        m_CellIPFColors.setValue(index + 2, static_cast<uint8_t>(nx::core::RgbColor::dBlue(argb)));
      }
    }
  }

  /**
   * @brief Dispatches to the correct convert<T>() instantiation based on the
   *        runtime DataType of the mask array.
   */
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

  /**
   * @brief ParallelDataAlgorithm entry point. Called once per thread with a
   *        disjoint Range of tuple indices.
   */
  void operator()(const Range& range) const
  {
    run(range.min(), range.max());
  }

private:
  ComputeIPFColorsDirect* m_Filter = nullptr;             ///< Back-pointer for cancellation and phase-warning accumulation.
  nx::core::FloatVec3 m_ReferenceDir;                     ///< Normalized sample-frame reference direction.
  nx::core::Float32AbstractDataStore& m_CellEulerAngles;  ///< DataStore of Euler angles (3 components per tuple, radians).
  nx::core::Int32AbstractDataStore& m_CellPhases;         ///< DataStore of per-voxel phase IDs.
  nx::core::UInt32AbstractDataStore& m_CrystalStructures; ///< DataStore of ensemble crystal structure enums.
  int32_t m_NumPhases = 0;                                ///< Number of phases in the ensemble (bounds check for phase IDs).
  const nx::core::IDataArray* m_GoodVoxels = nullptr;     ///< Optional mask array (bool or uint8). nullptr means compute all.
  nx::core::UInt8AbstractDataStore& m_CellIPFColors;      ///< Output DataStore of RGB colors (3 components per tuple).
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
ComputeIPFColorsDirect::~ComputeIPFColorsDirect() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief In-core IPF color computation using multi-threaded ParallelDataAlgorithm.
 *
 * All data arrays are accessed through AbstractDataStore references, which provide
 * O(1) random access when the backing store is in-memory. The ParallelDataAlgorithm
 * splits the total voxel count into sub-ranges and dispatches ComputeIPFColorsImpl
 * workers to separate threads.
 *
 * requireArraysInMemory() is called to pin all arrays in RAM for the duration of
 * parallel execution, preventing potential issues if a store implements lazy loading.
 */
Result<> ComputeIPFColorsDirect::operator()()
{
  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  // Retrieve typed array references. These are guaranteed to exist because
  // the filter's preflightImpl() validated them via selection parameters.
  nx::core::Float32Array& eulers = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellEulerAnglesArrayPath);
  nx::core::Int32Array& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  nx::core::UInt32Array& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);
  nx::core::UInt8Array& ipfColors = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->cellIpfColorsArrayPath);

  m_PhaseWarningCount = 0;
  size_t totalPoints = eulers.getNumberOfTuples();

  // The crystal structures array has one entry per phase (including phase 0 = unknown).
  // numPhases is used as an upper bound for phase ID validation.
  int32_t numPhases = static_cast<int32_t>(crystalStructures.getNumberOfTuples());

  // Normalize the reference direction to a unit vector. The user may supply
  // any non-zero vector (e.g., [0,0,1] for the sample Z axis).
  nx::core::FloatVec3 normRefDir = m_InputValues->referenceDirection;
  normRefDir = normRefDir.normalize();

  // Collect all arrays that will be accessed by the parallel workers so that
  // ParallelDataAlgorithm can pin them in memory for the duration of execution.
  typename IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(&eulers);
  algArrays.push_back(&phases);
  algArrays.push_back(&crystalStructures);
  algArrays.push_back(&ipfColors);

  nx::core::IDataArray* goodVoxelsArray = nullptr;
  if(m_InputValues->useGoodVoxels)
  {
    goodVoxelsArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->goodVoxelsArrayPath);
    algArrays.push_back(goodVoxelsArray);
  }

  // Launch multi-threaded execution. Each thread processes a contiguous sub-range
  // of tuple indices via ComputeIPFColorsImpl::operator()(Range).
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, totalPoints);
  dataAlg.requireArraysInMemory(algArrays);

  dataAlg.execute(ComputeIPFColorsImpl(this, normRefDir, eulers, phases, crystalStructures, numPhases, goodVoxelsArray, ipfColors));

  // After all threads have joined, check whether any voxels had phase IDs that
  // exceeded the ensemble array bounds. This is a data-quality issue in the input.
  if(m_PhaseWarningCount > 0)
  {
    std::string message = fmt::format("The Ensemble Phase information only references {} phase(s) but {} cell(s) had a phase value greater than {}. \
This indicates a problem with the input cell phase data. DREAM3D-NX will give INCORRECT RESULTS.",
                                      (numPhases - 1), m_PhaseWarningCount, (numPhases - 1));

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
