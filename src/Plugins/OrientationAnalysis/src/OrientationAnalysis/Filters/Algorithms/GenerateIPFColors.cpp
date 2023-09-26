#include "GenerateIPFColors.hpp"

#include "complex/Common/RgbColor.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

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
                        const complex::IDataArray* goodVoxels, complex::UInt8Array& colors)
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

  virtual ~GenerateIPFColorsImpl() = default;

  template <typename T>
  void convert(size_t start, size_t end) const
  {
    using MaskArrayType = DataArray<T>;
    const MaskArrayType* maskArray = nullptr;
    if(nullptr != m_GoodVoxels)
    {
      maskArray = dynamic_cast<const MaskArrayType*>(m_GoodVoxels);
    }

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
      m_CellIPFColors.setValue(index, 0);
      m_CellIPFColors.setValue(index + 1, 0);
      m_CellIPFColors.setValue(index + 2, 0);
      dEuler[0] = m_CellEulerAngles.getValue(index);
      dEuler[1] = m_CellEulerAngles.getValue(index + 1);
      dEuler[2] = m_CellEulerAngles.getValue(index + 2);

      // Make sure we are using a valid Euler Angles with valid crystal symmetry
      calcIPF = true;
      if(nullptr != maskArray)
      {
        calcIPF = (*maskArray)[i];
      }
      // Sanity check the phase data to make sure we do not walk off the end of the array
      if(phase >= m_NumPhases)
      {
        m_Filter->incrementPhaseWarningCount();
      }

      if(phase < m_NumPhases && calcIPF && m_CrystalStructures[phase] < EbsdLib::CrystalStructure::LaueGroupEnd)
      {
        argb = ops[m_CrystalStructures[phase]]->generateIPFColor(dEuler.data(), refDir.data(), false);
        m_CellIPFColors.setValue(index, static_cast<uint8_t>(complex::RgbColor::dRed(argb)));
        m_CellIPFColors.setValue(index + 1, static_cast<uint8_t>(complex::RgbColor::dGreen(argb)));
        m_CellIPFColors.setValue(index + 2, static_cast<uint8_t>(complex::RgbColor::dBlue(argb)));
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
      convert<bool>(start, end);
    }
  }

  void operator()(const Range& range) const
  {
    run(range.min(), range.max());
  }

private:
  GenerateIPFColors* m_Filter = nullptr;
  FloatVec3Type m_ReferenceDir;
  complex::Float32AbstractDataStore& m_CellEulerAngles;
  complex::Int32AbstractDataStore& m_CellPhases;
  complex::UInt32AbstractDataStore& m_CrystalStructures;
  int32_t m_NumPhases = 0;
  const complex::IDataArray* m_GoodVoxels = nullptr;
  complex::UInt8AbstractDataStore& m_CellIPFColors;
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

  complex::UInt32Array& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  complex::UInt8Array& ipfColors = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->cellIpfColorsArrayPath);

  m_PhaseWarningCount = 0;
  size_t totalPoints = eulers.getNumberOfTuples();

  int32_t numPhases = static_cast<int32_t>(crystalStructures.getNumberOfTuples());

  // Make sure we are dealing with a unit 1 vector.
  FloatVec3Type normRefDir = m_InputValues->referenceDirection; // Make a copy of the reference Direction

  MatrixMath::Normalize3x1(normRefDir[0], normRefDir[1], normRefDir[2]);

  complex::IDataArray* goodVoxelsArray = nullptr;
  if(m_InputValues->useGoodVoxels)
  {
    goodVoxelsArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->goodVoxelsArrayPath);
  }
  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, totalPoints);
  dataAlg.setParallelizationEnabled(false);
  dataAlg.execute(GenerateIPFColorsImpl(this, normRefDir, eulers, phases, crystalStructures, numPhases, goodVoxelsArray, ipfColors));

  if(m_PhaseWarningCount > 0)
  {
    std::string message = fmt::format("The Ensemble Phase information only references {} phase(s) but {} cell(s) had a phase value greater than {}. \
This indicates a problem with the input cell phase data. DREAM.3D will give INCORRECT RESULTS.",
                                      (numPhases - 1), m_PhaseWarningCount, (numPhases - 1));

    return complex::MakeErrorResult(-48000, message);
  }

  return {};
}

// -----------------------------------------------------------------------------
void GenerateIPFColors::incrementPhaseWarningCount()
{
  ++m_PhaseWarningCount;
}
