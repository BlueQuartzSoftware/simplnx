#include "ComputeFZQuaternionsFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/EbsdMacros.h"
#include "EbsdLib/LaueOps/LaueOps.h"

using namespace nx::core;

namespace
{
/**
 * @brief The GenerateFZQuatsImpl class implements a threaded algorithm that computes the Fundamental Zone Quaternion
 * for a given Quaternion and Laue Class (which is based from the crystalStructures array
 */
class GenerateMaskedFZQuatsImpl
{
public:
  GenerateMaskedFZQuatsImpl(const Float32AbstractDataStore& quats, const Int32AbstractDataStore& phases, const UInt32AbstractDataStore& crystalStructures, const int32 numPhases,
                            std::unique_ptr<MaskCompareUtilities::MaskCompare>& goodVoxels, Float32AbstractDataStore& fzQuats, const std::atomic_bool& shouldCancel, std::atomic_int32_t& warningCount)
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

  ~GenerateMaskedFZQuatsImpl() = default;

  /**
   * @brief convert
   * @param start
   * @param end
   */
  void convert(size_t start, size_t end) const
  {
    std::vector<LaueOps::Pointer> ops = LaueOps::GetAllOrientationOps();
    int32 phase = 0;
    size_t index = 0;

    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      phase = m_CellPhases.getValue(i);

      // Sanity check the phase data to make sure we do not walk off the end of the array
      if(phase >= m_NumPhases)
      {
        m_WarningCount++;
      }

      // Output initialized to zero by default
      index = i * 4;
      if(phase < m_NumPhases && m_GoodVoxels->isTrue(i) && m_CrystalStructures.getValue(phase) < EbsdLib::CrystalStructure::LaueGroupEnd)
      {
        QuatD quatD = QuatD(m_Quats.getValue(index), m_Quats.getValue(index + 1), m_Quats.getValue(index + 2), m_Quats.getValue(index + 3)); // Makes a copy into q
        auto xtal = static_cast<int32_t>(m_CrystalStructures.getValue(phase));                                                               // get the Laue Group
        quatD = ops[xtal]->getFZQuat(quatD);
        m_FZQuats.setValue(index, static_cast<float32>(quatD.x()));
        m_FZQuats.setValue(index + 1, static_cast<float32>(quatD.y()));
        m_FZQuats.setValue(index + 2, static_cast<float32>(quatD.z()));
        m_FZQuats.setValue(index + 3, static_cast<float32>(quatD.w()));
      }
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const Float32AbstractDataStore& m_Quats;
  const Int32AbstractDataStore& m_CellPhases;
  const UInt32AbstractDataStore& m_CrystalStructures;
  const int32 m_NumPhases = 0;
  std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_GoodVoxels;
  Float32AbstractDataStore& m_FZQuats;
  const std::atomic_bool& m_ShouldCancel;
  std::atomic_int32_t& m_WarningCount;
};

class GenerateFZQuatsImpl
{
public:
  GenerateFZQuatsImpl(const Float32AbstractDataStore& quats, const Int32AbstractDataStore& phases, const UInt32AbstractDataStore& crystalStructures, const int32 numPhases,
                      Float32AbstractDataStore& fzQuats, const std::atomic_bool& shouldCancel, std::atomic_int32_t& warningCount)
  : m_Quats(quats)
  , m_CellPhases(phases)
  , m_CrystalStructures(crystalStructures)
  , m_NumPhases(numPhases)
  , m_FZQuats(fzQuats)
  , m_ShouldCancel(shouldCancel)
  , m_WarningCount(warningCount)
  {
  }

  ~GenerateFZQuatsImpl() = default;

  /**
   * @brief convert
   * @param start
   * @param end
   */
  void convert(size_t start, size_t end) const
  {
    std::vector<LaueOps::Pointer> ops = LaueOps::GetAllOrientationOps();
    int32 phase = 0;
    size_t index = 0;

    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      phase = m_CellPhases.getValue(i);

      // Sanity check the phase data to make sure we do not walk off the end of the array
      if(phase >= m_NumPhases)
      {
        m_WarningCount++;
      }

      // Output initialized to zero by default
      index = i * 4;
      if(phase < m_NumPhases && m_CrystalStructures.getValue(phase) < EbsdLib::CrystalStructure::LaueGroupEnd)
      {
        QuatD quatD = QuatD(m_Quats.getValue(index), m_Quats.getValue(index + 1), m_Quats.getValue(index + 2), m_Quats.getValue(index + 3)); // Makes a copy into q
        auto xtal = static_cast<int32_t>(m_CrystalStructures.getValue(phase));                                                               // get the Laue Group
        quatD = ops[xtal]->getFZQuat(quatD);
        m_FZQuats.setValue(index, static_cast<float32>(quatD.x()));
        m_FZQuats.setValue(index + 1, static_cast<float32>(quatD.y()));
        m_FZQuats.setValue(index + 2, static_cast<float32>(quatD.z()));
        m_FZQuats.setValue(index + 3, static_cast<float32>(quatD.w()));
      }
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const Float32AbstractDataStore& m_Quats;
  const Int32AbstractDataStore& m_CellPhases;
  const UInt32AbstractDataStore& m_CrystalStructures;
  const int32 m_NumPhases = 0;
  Float32AbstractDataStore& m_FZQuats;
  const std::atomic_bool& m_ShouldCancel;
  std::atomic_int32_t& m_WarningCount;
};
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFZQuaternionsFilter::name() const
{
  return FilterTraits<ComputeFZQuaternionsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFZQuaternionsFilter::className() const
{
  return FilterTraits<ComputeFZQuaternionsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFZQuaternionsFilter::uuid() const
{
  return FilterTraits<ComputeFZQuaternionsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFZQuaternionsFilter::humanName() const
{
  return "Compute Fundamental Zone Orientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFZQuaternionsFilter::defaultTags() const
{
  return {className(), "Processing", "OrientationAnalysis", "Quaternion", "Generate"};
}

//------------------------------------------------------------------------------
Parameters ComputeFZQuaternionsFilter::parameters() const
{
  Parameters params;

  std::vector<std::string> names = LaueOps::GetLaueNames();
  names.pop_back();

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Input Quaternions", "The input quaternions to convert.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Input Phases", "The phases of the data. The data should be the indices into the Crystal Structures Data Array.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMask_Key, "Apply to Good Elements Only (Bad Elements Will Be Black)", "Whether to assign a black color to 'bad' Elements", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Input Element Mask", "Optional Mask array where valid data is TRUE or 1.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint8, DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FZQuatsArrayName_Key, "Created FZ Quaternions",
                                                          "The name of the array containing the Quaternion that represents an orientation within the fundamental zone for each Element", ""));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFZQuaternionsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFZQuaternionsFilter::clone() const
{
  return std::make_unique<ComputeFZQuaternionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFZQuaternionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFZQuatsArrayPathValue = pQuatsArrayPathValue.replaceName(filterArgs.value<std::string>(k_FZQuatsArrayName_Key));

  const auto& phaseData = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);
  const auto& quatArray = dataStructure.getDataRefAs<Float32Array>(pQuatsArrayPathValue);
  if(phaseData.getNumberOfTuples() != quatArray.getNumberOfTuples())
  {
    return {MakeErrorResult<OutputActions>(-49001,
                                           fmt::format("Quaternion and Phase Arrays must have the same number of tuples. '{} != {}'", quatArray.getNumberOfTuples(), phaseData.getNumberOfTuples()))};
  }

  if(pUseGoodVoxelsValue)
  {
    const auto& maskArray = dataStructure.getDataRefAs<IDataArray>(pGoodVoxelsArrayPathValue);
    if(maskArray.getNumberOfTuples() != quatArray.getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-49002,
                                             fmt::format("Quaternion and Mask arrays must have the same number of tuples. '{} != {}'", quatArray.getNumberOfTuples(), maskArray.getNumberOfTuples()))};
    }
  }

  nx::core::Result<OutputActions> resultOutputActions;

  auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, quatArray.getDataStore()->getTupleShape(), quatArray.getDataStore()->getComponentShape(),
                                                               pFZQuatsArrayPathValue, CreateArrayAction::k_DefaultDataFormat, "0.0");
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeFZQuaternionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pFZQuatsArrayPathValue = pQuatsArrayPathValue.replaceName(filterArgs.value<std::string>(k_FZQuatsArrayName_Key));

  auto& phaseArray = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);
  auto& quatArray = dataStructure.getDataRefAs<Float32Array>(pQuatsArrayPathValue);
  auto& xtalArray = dataStructure.getDataRefAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  auto* maskArray = dataStructure.getDataAs<IDataArray>(pGoodVoxelsArrayPathValue);
  auto& fzQuatArray = dataStructure.getDataRefAs<Float32Array>(pFZQuatsArrayPathValue);

  std::atomic_int32_t warningCount = 0;
  auto numPhases = static_cast<int32>(xtalArray.getNumberOfTuples());

  typename IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(&phaseArray);
  algArrays.push_back(&quatArray);
  algArrays.push_back(&xtalArray);
  algArrays.push_back(&fzQuatArray);

  if(pUseGoodVoxelsValue)
  {
    algArrays.push_back(maskArray);
  }

  try
  {
    // Parallel algorithm
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0ULL, static_cast<size_t>(quatArray.getNumberOfTuples()));
    dataAlg.requireArraysInMemory(algArrays);

    if(pUseGoodVoxelsValue)
    {
      std::unique_ptr<MaskCompareUtilities::MaskCompare> maskArrayPtr = nullptr;
      try
      {
        maskArrayPtr = MaskCompareUtilities::InstantiateMaskCompare(dataStructure, pGoodVoxelsArrayPathValue);
      } catch(const std::out_of_range& exception)
      {
        // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
        // some other context that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
        return MakeErrorResult(-49003, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", pGoodVoxelsArrayPathValue.toString()));
      }

      dataAlg.execute(::GenerateMaskedFZQuatsImpl(quatArray.getDataStoreRef(), phaseArray.getDataStoreRef(), xtalArray.getDataStoreRef(), numPhases, maskArrayPtr, fzQuatArray.getDataStoreRef(),
                                                  shouldCancel, warningCount));
    }
    else
    {
      dataAlg.execute(
          ::GenerateFZQuatsImpl(quatArray.getDataStoreRef(), phaseArray.getDataStoreRef(), xtalArray.getDataStoreRef(), numPhases, fzQuatArray.getDataStoreRef(), shouldCancel, warningCount));
    }

    if(warningCount > 0)
    {
      std::string errorMessage = fmt::format("The Ensemble Phase information only references {} phase(s) but {} cell(s) had a phase value greater than {}. This indicates a problem with the input "
                                             "cell phase data. DREAM3D-NX may have given INCORRECT RESULTS.",
                                             numPhases - 1, warningCount.load(), numPhases - 1);

      return {MakeErrorResult<>(-49004, errorMessage)};
    }
  } catch(const EbsdLib::method_not_implemented& e)
  {
    return {MakeErrorResult<>(-49005, fmt::format("EbsdLib threw an exception when computing the fundamental zone data. {}", e.what()))};
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteral k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_FZQuatsArrayPathKey = "FZQuatsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeFZQuaternionsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFZQuaternionsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseGoodVoxelsKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FZQuatsArrayPathKey, k_FZQuatsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
