#include "ComputeFZQuaternions.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/LaueOps/LaueOps.h"

using namespace nx::core;

namespace
{
/**
 * @brief The GenerateFZQuatsImpl class implements a threaded algorithm that computes the Fundamental Zone Quaternion
 * for a given Quaternion and Laue Class (which is based from the crystalStructures array
 */
template <typename MaskArrayType>
class GenerateFZQuatsImpl
{
public:
  GenerateFZQuatsImpl(Float32Array& quats, Int32Array& phases, UInt32Array& crystalStructures, int32_t numPhases, MaskArrayType* goodVoxels, Float32Array& fzQuats,
                      const std::atomic_bool& shouldCancel, std::atomic_int32_t& warningCount)
  : m_Quats(quats)
  , m_CellPhases(phases)
  , m_CrystalStructures(crystalStructures)
  , m_NumPhases(numPhases)
  , m_GoodVoxels(goodVoxels)
  , m_FZQuats(fzQuats)
  , m_ShouldCancel(shouldCancel)
  , m_WarningCount(warningCount)
  {
  }

  virtual ~GenerateFZQuatsImpl() = default;

  /**
   * @brief convert
   * @param start
   * @param end
   */
  void convert(size_t start, size_t end) const
  {
    std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();
    int32_t phase = 0;
    bool generateFZQuat = false;
    size_t index = 0;

    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      phase = m_CellPhases[i];

      generateFZQuat = true;
      if(nullptr != m_GoodVoxels)
      {
        generateFZQuat = static_cast<bool>((*m_GoodVoxels)[i]);
      }

      // Sanity check the phase data to make sure we do not walk off the end of the array
      if(phase >= m_NumPhases)
      {
        m_WarningCount++;
      }

      // Initialize the output to zero. There really isn't a good value to use.
      index = i * 4;
      m_FZQuats[index] = 0.0f;
      m_FZQuats[index + 1] = 0.0f;
      m_FZQuats[index + 2] = 0.0f;
      m_FZQuats[index + 3] = 0.0f;

      if(phase < m_NumPhases && generateFZQuat && m_CrystalStructures[phase] < ebsdlib::CrystalStructure::LaueGroupEnd)
      {
        ebsdlib::QuatD quatD = ebsdlib::QuatD(m_Quats[index], m_Quats[index + 1], m_Quats[index + 2], m_Quats[index + 3]); // Makes a copy into q
        int32_t xtal = static_cast<int32_t>(m_CrystalStructures[phase]);                                                   // get the Laue Group
        quatD = ops[xtal]->getFZQuat(quatD);
        m_FZQuats[index] = quatD.x();
        m_FZQuats[index + 1] = quatD.y();
        m_FZQuats[index + 2] = quatD.z();
        m_FZQuats[index + 3] = quatD.w();
      }
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  Float32Array& m_Quats;
  Int32Array& m_CellPhases;
  UInt32Array& m_CrystalStructures;
  int32_t m_NumPhases = 0;
  MaskArrayType* m_GoodVoxels;
  Float32Array& m_FZQuats;
  const std::atomic_bool& m_ShouldCancel;
  std::atomic_int32_t& m_WarningCount;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeFZQuaternions::ComputeFZQuaternions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFZQuaternionsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFZQuaternions::~ComputeFZQuaternions() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFZQuaternions::operator()()
{

  Int32Array& phaseArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  Float32Array& quatArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputQuatsArrayPath);
  UInt32Array& xtalArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  AbstractDataArray* maskArray = m_DataStructure.getDataAs<AbstractDataArray>(m_InputValues->MaskArrayPath);
  Float32Array& fzQuatArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputQuatsArrayPath.replaceName(m_InputValues->OutputFzQuatsArrayName));

  std::atomic_int32_t warningCount = 0;
  int32_t numPhases = static_cast<int32_t>(xtalArray.getNumberOfTuples());

  typename ParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(&phaseArray);
  algArrays.push_back(&quatArray);
  algArrays.push_back(&xtalArray);
  algArrays.push_back(&fzQuatArray);

  if(m_InputValues->UseMask)
  {
    algArrays.push_back(maskArray);
  }

  // Parallel algorithm
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(quatArray.getNumberOfTuples()));
  dataAlg.requireArraysInMemory(algArrays);

  if(m_InputValues->UseMask)
  {
    if(maskArray->getDataType() == DataType::boolean)
    {
      BoolArray* goodVoxelsArray = m_DataStructure.getDataAs<BoolArray>(m_InputValues->MaskArrayPath);
      dataAlg.execute(::GenerateFZQuatsImpl<BoolArray>(quatArray, phaseArray, xtalArray, numPhases, goodVoxelsArray, fzQuatArray, m_ShouldCancel, warningCount));
    }
    else if(maskArray->getDataType() == DataType::uint8)
    {
      UInt32Array* goodVoxelsArray = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->MaskArrayPath);
      dataAlg.execute(::GenerateFZQuatsImpl<UInt32Array>(quatArray, phaseArray, xtalArray, numPhases, goodVoxelsArray, fzQuatArray, m_ShouldCancel, warningCount));
    }
    else if(maskArray->getDataType() == DataType::int8)
    {
      Int8Array* goodVoxelsArray = m_DataStructure.getDataAs<Int8Array>(m_InputValues->MaskArrayPath);
      dataAlg.execute(::GenerateFZQuatsImpl<Int8Array>(quatArray, phaseArray, xtalArray, numPhases, goodVoxelsArray, fzQuatArray, m_ShouldCancel, warningCount));
    }
  }
  else
  {
    dataAlg.execute(::GenerateFZQuatsImpl<Int8Array>(quatArray, phaseArray, xtalArray, numPhases, nullptr, fzQuatArray, m_ShouldCancel, warningCount));
  }

  if(warningCount > 0)
  {
    std::string errorMessage = fmt::format("The Ensemble Phase information only references {} phase(s) but {} cell(s) had a phase value greater than {}. \
This indicates a problem with the input cell phase data. DREAM3D-NX may have given INCORRECT RESULTS.",
                                           numPhases - 1, warningCount.load(), numPhases - 1);

    return {MakeErrorResult<>(-49008, errorMessage)};
  }

  return {};
}
