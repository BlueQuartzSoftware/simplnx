#include "ComputeFZQuaternionsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeFZQuaternions.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

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

  std::vector<std::string> names = ebsdlib::LaueOps::GetLaueNames();
  names.pop_back();

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Input Quaternions", "The input quaternions to convert.", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Input Phases", "The phases of the data. The data should be the indices into the Crystal Structures Data Array.",
                                                          DataPath{}, GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMask_Key, "Apply to Good Elements Only (Bad Elements Will Be Black)", "Whether to assign a black color to 'bad' Elements", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Input Element Mask", "Optional Mask array where valid data is TRUE or 1.", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::uint8, DataType::boolean}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
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
IFilter::UniquePointerType ComputeFZQuaternionsFilter::clone() const
{
  return std::make_unique<ComputeFZQuaternionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFZQuaternionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                   const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFZQuatsArrayPathValue = pQuatsArrayPathValue.replaceName(filterArgs.value<std::string>(k_FZQuatsArrayName_Key));

  const auto& phaseData = dataStructure.getDataRefAs<Int32ArrayType>(pCellPhasesArrayPathValue);
  const auto& quatArray = dataStructure.getDataRefAs<Float32ArrayType>(pQuatsArrayPathValue);
  if(phaseData.getNumberOfTuples() != quatArray.getNumberOfTuples())
  {
    return {.output_actions = MakeErrorResult<OutputActions>(
                -49001, fmt::format("Quaternion and Phase Arrays must have the same number of tuples. '{} != {}'", quatArray.getNumberOfTuples(), phaseData.getNumberOfTuples()))};
  }

  if(pUseGoodVoxelsValue)
  {
    const auto& maskArray = dataStructure.getDataRefAs<IDataArray>(pGoodVoxelsArrayPathValue);
    if(maskArray.getNumberOfTuples() != quatArray.getNumberOfTuples())
    {
      return {.output_actions = MakeErrorResult<OutputActions>(
                  -49002, fmt::format("Quaternion and Mask arrays must have the same number of tuples. '{} != {}'", quatArray.getNumberOfTuples(), maskArray.getNumberOfTuples()))};
    }
  }

  nx::core::Result<OutputActions> resultOutputActions;

  // An empty format lets the destination resolver select storage while the
  // output retains the input tuple and component shapes.
  auto createArrayAction =
      std::make_unique<CreateArrayAction>(nx::core::DataType::float32, quatArray.getDataStore()->getTupleShape(), quatArray.getDataStore()->getComponentShape(), pFZQuatsArrayPathValue, "", "0.0");
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeFZQuaternionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{

  ComputeFZQuaternionsInputValues inputValues;
  inputValues.use_mask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.input_quats_array_path = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.mask_array_path = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.output_fz_quats_array_name = filterArgs.value<std::string>(k_FZQuatsArrayName_Key);

  return ComputeFZQuaternions(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteralType k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteralType k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteralType k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteralType k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteralType k_FZQuatsArrayPathKey = "FZQuatsArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> ComputeFZQuaternionsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFZQuaternionsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseGoodVoxelsKey, k_UseMask_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, simpl::k_FZQuatsArrayPathKey, k_FZQuatsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
