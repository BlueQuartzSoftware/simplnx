#include "GenerateIPFColors.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"
#include "complex/Common/RgbColor.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

using namespace complex;

using FloatVec3Type = std::vector<float>;

namespace
{

/**
 * @brief The GenerateIPFColorsImpl class implements a threaded algorithm that computes the IPF
 * colors for each element in a geometry
 */
class GenerateIPFColorsImpl
{
public:
  GenerateIPFColorsImpl(GenerateIPFColors* filter, FloatVec3Type referenceDir, complex::Float32Array& eulers, complex::Int32Array& phases, complex::UInt32Array& crystalStructures, int32_t numPhases,
                        complex::BoolArray* goodVoxels, complex::UInt8Array& colors)
  : m_Filter(filter)
  , m_ReferenceDir(referenceDir)
  , m_CellEulerAngles(eulers)
  , m_CellPhases(phases)
  , m_CrystalStructures(crystalStructures)
  , m_NumPhases(numPhases)
  , m_GoodVoxels(goodVoxels)
  , m_CellIPFColors(colors)
  {
  }

  virtual ~GenerateIPFColorsImpl() = default;

  void convert(size_t start, size_t end) const
  {
    std::vector<LaueOps::Pointer> ops = LaueOps::GetAllOrientationOps();
    std::array<double, 3> refDir = {m_ReferenceDir[0], m_ReferenceDir[1], m_ReferenceDir[2]};
    std::array<double, 3> dEuler = {0.0, 0.0, 0.0};
    Rgba argb = 0x00000000;
    int32_t phase = 0;
    bool calcIPF = false;
    size_t index = 0;
    for(size_t i = start; i < end; i++)
    {
      phase = m_CellPhases[i];
      index = i * 3;
      m_CellIPFColors[index] = 0;
      m_CellIPFColors[index + 1] = 0;
      m_CellIPFColors[index + 2] = 0;
      dEuler[0] = m_CellEulerAngles[index];
      dEuler[1] = m_CellEulerAngles[index + 1];
      dEuler[2] = m_CellEulerAngles[index + 2];

      // Make sure we are using a valid Euler Angles with valid crystal symmetry
      calcIPF = true;
      if(nullptr != m_GoodVoxels)
      {
        calcIPF = (*m_GoodVoxels)[i];
      }
      // Sanity check the phase data to make sure we do not walk off the end of the array
      if(phase >= m_NumPhases)
      {
        m_Filter->incrementPhaseWarningCount();
      }

      if(phase < m_NumPhases && calcIPF && m_CrystalStructures[phase] < EbsdLib::CrystalStructure::LaueGroupEnd)
      {
        argb = ops[m_CrystalStructures[phase]]->generateIPFColor(dEuler.data(), refDir.data(), false);
        m_CellIPFColors[index] = static_cast<uint8_t>(complex::RgbColor::dRed(argb));
        m_CellIPFColors[index + 1] = static_cast<uint8_t>(complex::RgbColor::dGreen(argb));
        m_CellIPFColors[index + 2] = static_cast<uint8_t>(complex::RgbColor::dBlue(argb));
      }
    }
  }

  void operator()(const ComplexRange& range) const
  {
    convert(range.min(), range.max());
  }

private:
  GenerateIPFColors* m_Filter = nullptr;
  FloatVec3Type m_ReferenceDir;
  complex::Float32Array& m_CellEulerAngles;
  complex::Int32Array& m_CellPhases;
  complex::UInt32Array& m_CrystalStructures;
  int32_t m_NumPhases = 0;
  complex::BoolArray* m_GoodVoxels = nullptr;
  complex::UInt8Array& m_CellIPFColors;
};
} // namespace

// -----------------------------------------------------------------------------
GenerateIPFColors::GenerateIPFColors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateIPFColorsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GenerateIPFColors::~GenerateIPFColors() noexcept = default;

// -----------------------------------------------------------------------------
Result<> GenerateIPFColors::operator()()
{

  std::vector<LaueOps::Pointer> orientationOps = LaueOps::GetAllOrientationOps();

  complex::Float32Array& eulers = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->cellEulerAnglesArrayPath);
  complex::Int32Array& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);

  auto* goodVoxelsArray = m_DataStructure.getDataAs<BoolArray>(m_InputValues->goodVoxelsArrayPath);

  complex::UInt32Array& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  complex::UInt8Array& ipfColors = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->cellIpfColorsArrayPath);

  m_PhaseWarningCount = 0;
  size_t totalPoints = eulers.getNumberOfTuples();

  int32_t numPhases = static_cast<int32_t>(crystalStructures.getNumberOfTuples());

  // Make sure we are dealing with a unit 1 vector.
  FloatVec3Type normRefDir = m_InputValues->referenceDirection; // Make a copy of the reference Direction

  MatrixMath::Normalize3x1(normRefDir[0], normRefDir[1], normRefDir[2]);

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, totalPoints);
  dataAlg.execute(GenerateIPFColorsImpl(this, normRefDir, eulers, phases, crystalStructures, numPhases, goodVoxelsArray, ipfColors));

  if(m_PhaseWarningCount > 0)
  {
    std::string message = fmt::format("The Ensemble Phase information only references {} phase(s) but {} cell(s) had a phase value greater than {}. \
This indicates a problem with the input cell phase data. DREAM.3D will give INCORRECT RESULTS.",(numPhases - 1), m_PhaseWarningCount, (numPhases - 1));

    return complex::MakeErrorResult(-48000, message);
  }

  return {};
}

// -----------------------------------------------------------------------------
void GenerateIPFColors::incrementPhaseWarningCount()
{
  ++m_PhaseWarningCount;
}
