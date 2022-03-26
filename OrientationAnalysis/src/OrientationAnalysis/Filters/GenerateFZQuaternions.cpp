#include "GenerateFZQuaternions.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/LaueOps/LaueOps.h"

using namespace complex;

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
    std::vector<LaueOps::Pointer> ops = LaueOps::GetAllOrientationOps();
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

      if(phase < m_NumPhases && generateFZQuat && m_CrystalStructures[phase] < EbsdLib::CrystalStructure::LaueGroupEnd)
      {
        QuatD quatD = QuatD(m_Quats[index], m_Quats[index + 1], m_Quats[index + 2], m_Quats[index + 3]); // Makes a copy into q
        int32_t xtal = static_cast<int32_t>(m_CrystalStructures[phase]);                                 // get the Laue Group
        quatD = ops[xtal]->getFZQuat(quatD);
        m_FZQuats[index] = quatD.x();
        m_FZQuats[index + 1] = quatD.y();
        m_FZQuats[index + 2] = quatD.z();
        m_FZQuats[index + 3] = quatD.w();
      }
    }
  }

  void operator()(const ComplexRange& range) const
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

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateFZQuaternions::name() const
{
  return FilterTraits<GenerateFZQuaternions>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateFZQuaternions::className() const
{
  return FilterTraits<GenerateFZQuaternions>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateFZQuaternions::uuid() const
{
  return FilterTraits<GenerateFZQuaternions>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateFZQuaternions::humanName() const
{
  return "Reduce Orientations to Fundamental Zone";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateFZQuaternions::defaultTags() const
{
  return {"#Processing", "#OrientationAnalysis"};
}

//------------------------------------------------------------------------------
Parameters GenerateFZQuaternions::parameters() const
{
  Parameters params;

  std::vector<std::string> names = LaueOps::GetLaueNames();
  names.pop_back();

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Input Quaternions", "The input quaternions to convert.", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Input Phases", "The phases of the data. The data should be the indices into the Crystal Structures Data Array.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Apply to Good Elements Only (Bad Elements Will Be Black)", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Input Mask [Optional]", "Optional Mask array where valid data is TRUE or 1.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int8, DataType::uint8, DataType::boolean}, true));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::uint32}));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FZQuatsArrayPath_Key, "Created FZ Quaternions", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateFZQuaternions::clone() const
{
  return std::make_unique<GenerateFZQuaternions>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateFZQuaternions::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{

  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pFZQuatsArrayPathValue = filterArgs.value<DataPath>(k_FZQuatsArrayPath_Key);

  const Int32Array& phaseData = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);

  if(phaseData.getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(-49000, fmt::format("Phase Array number of components is not 1: '{}'", phaseData.getNumberOfComponents()))};
  }

  const Float32Array& quatArray = dataStructure.getDataRefAs<Float32Array>(pQuatsArrayPathValue);
  if(quatArray.getNumberOfComponents() != 4)
  {
    return {MakeErrorResult<OutputActions>(-49001, fmt::format("Quaternion Array number of components is not 4: '{}'", quatArray.getNumberOfComponents()))};
  }

  if(phaseData.getNumberOfTuples() != quatArray.getNumberOfTuples())
  {
    return {MakeErrorResult<OutputActions>(-49002,
                                           fmt::format("Quaternion and Phase Arrays must have the same number of tuples. '{} != {}'", quatArray.getNumberOfTuples(), phaseData.getNumberOfTuples()))};
  }

  const UInt32Array& xtalArray = dataStructure.getDataRefAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  if(xtalArray.getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(-49003, fmt::format("Crystal Structure Array number of components is not 1: '{}'", xtalArray.getNumberOfComponents()))};
  }

  if(pUseGoodVoxelsValue)
  {
    const IDataArray& maskArray = dataStructure.getDataRefAs<IDataArray>(pGoodVoxelsArrayPathValue);
    if(maskArray.getNumberOfComponents() != 1)
    {
      return {MakeErrorResult<OutputActions>(-49004, fmt::format("Mask Array number of components is not 1: '{}'", maskArray.getNumberOfComponents()))};
    }
    if(maskArray.getDataType() != complex::DataType::boolean && maskArray.getDataType() != complex::DataType::uint8 && maskArray.getDataType() != complex::DataType::int8)
    {
      return {MakeErrorResult<OutputActions>(-49005, fmt::format("Mask Array is not [BOOL (10) | UINT8 (2) | INT8 (1)]: '{}'", maskArray.getDataType()))};
    }

    if(maskArray.getNumberOfTuples() != quatArray.getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-49006,
                                             fmt::format("Quaternion and Mask arrays must have the same number of tuples. '{} != {}'", quatArray.getNumberOfTuples(), maskArray.getNumberOfTuples()))};
    }
  }

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  auto createArrayAction =
      std::make_unique<CreateArrayAction>(complex::DataType::float32, quatArray.getDataStore()->getTupleShape(), quatArray.getDataStore()->getComponentShape(), pFZQuatsArrayPathValue);
  resultOutputActions.value().actions.push_back(std::move(createArrayAction));

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GenerateFZQuaternions::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pFZQuatsArrayPathValue = filterArgs.value<DataPath>(k_FZQuatsArrayPath_Key);

  Int32Array& phaseArray = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);
  Float32Array& quatArray = dataStructure.getDataRefAs<Float32Array>(pQuatsArrayPathValue);
  UInt32Array& xtalArray = dataStructure.getDataRefAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  IDataArray* maskArray = dataStructure.getDataAs<IDataArray>(pGoodVoxelsArrayPathValue);
  Float32Array& fzQuatArray = dataStructure.getDataRefAs<Float32Array>(pFZQuatsArrayPathValue);

  std::atomic_int32_t warningCount = 0;
  int32_t numPhases = static_cast<int32_t>(xtalArray.getNumberOfTuples());

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(quatArray.getNumberOfTuples()));
  dataAlg.setParallelizationEnabled(false);

  if(pUseGoodVoxelsValue)
  {
    if(maskArray->getDataType() == DataType::boolean)
    {
      BoolArray* goodVoxelsArray = dataStructure.getDataAs<BoolArray>(pGoodVoxelsArrayPathValue);
      dataAlg.execute(::GenerateFZQuatsImpl<BoolArray>(quatArray, phaseArray, xtalArray, numPhases, goodVoxelsArray, fzQuatArray, shouldCancel, warningCount));
    }
    else if(maskArray->getDataType() == DataType::uint8)
    {
      UInt32Array* goodVoxelsArray = dataStructure.getDataAs<UInt32Array>(pGoodVoxelsArrayPathValue);
      dataAlg.execute(::GenerateFZQuatsImpl<UInt32Array>(quatArray, phaseArray, xtalArray, numPhases, goodVoxelsArray, fzQuatArray, shouldCancel, warningCount));
    }
    else if(maskArray->getDataType() == DataType::int8)
    {
      Int8Array* goodVoxelsArray = dataStructure.getDataAs<Int8Array>(pGoodVoxelsArrayPathValue);
      dataAlg.execute(::GenerateFZQuatsImpl<Int8Array>(quatArray, phaseArray, xtalArray, numPhases, goodVoxelsArray, fzQuatArray, shouldCancel, warningCount));
    }
  }
  else
  {
    dataAlg.execute(::GenerateFZQuatsImpl<Int8Array>(quatArray, phaseArray, xtalArray, numPhases, nullptr, fzQuatArray, shouldCancel, warningCount));
  }

  if(warningCount > 0)
  {
    std::string errorMessage = fmt::format("The Ensemble Phase information only references {} phase(s) but {} cell(s) had a phase value greater than {}. \
This indicates a problem with the input cell phase data. DREAM.3D may have given INCORRECT RESULTS.",
                                           numPhases - 1, warningCount, numPhases - 1);

    return {MakeErrorResult<>(-49008, errorMessage)};
  }
  return {};
}

} // namespace complex
