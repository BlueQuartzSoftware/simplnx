#include "ComputeCAxisReferenceMisalignmentsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeCAxisReferenceMisalignments.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <string_view>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Builds the algorithm inputs from the filter arguments, so preflight and execute read the same paths.
 * @param filterArgs Filter arguments.
 * @return Input values, including the output path beside the quaternion array.
 */
ComputeCAxisReferenceMisalignmentsInputValues CreateInputValues(const Arguments& filterArgs)
{
  ComputeCAxisReferenceMisalignmentsInputValues inputValues;
  inputValues.ReferenceDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(ComputeCAxisReferenceMisalignmentsFilter::k_ReferenceDir_Key);
  inputValues.UseMask = filterArgs.value<bool>(ComputeCAxisReferenceMisalignmentsFilter::k_UseMask_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(ComputeCAxisReferenceMisalignmentsFilter::k_QuatsArrayPath_Key);
  inputValues.PhasesArrayPath = filterArgs.value<DataPath>(ComputeCAxisReferenceMisalignmentsFilter::k_PhasesArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(ComputeCAxisReferenceMisalignmentsFilter::k_MaskArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(ComputeCAxisReferenceMisalignmentsFilter::k_CrystalStructuresArrayPath_Key);
  inputValues.OutputArrayPath = inputValues.QuatsArrayPath.replaceName(filterArgs.value<std::string>(ComputeCAxisReferenceMisalignmentsFilter::k_CAxisReferenceMisalignmentsArrayName_Key));
  return inputValues;
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeCAxisReferenceMisalignmentsFilter::name() const
{
  return FilterTraits<ComputeCAxisReferenceMisalignmentsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeCAxisReferenceMisalignmentsFilter::className() const
{
  return FilterTraits<ComputeCAxisReferenceMisalignmentsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeCAxisReferenceMisalignmentsFilter::uuid() const
{
  return FilterTraits<ComputeCAxisReferenceMisalignmentsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeCAxisReferenceMisalignmentsFilter::humanName() const
{
  return "Compute C-Axis Reference Misalignments";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeCAxisReferenceMisalignmentsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography", "C-Axis", "Misalignment", "Hexagonal"};
}

//------------------------------------------------------------------------------
Parameters ComputeCAxisReferenceMisalignmentsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ReferenceDir_Key, "Reference Direction", "Sample-frame direction to compare with the crystal c-axis. Must be finite and nonzero.",
                                                         std::vector<float32>{0.0F, 0.0F, 1.0F}, std::vector<std::string>{"X", "Y", "Z"}));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Whether to assign NaN to tuples where the mask is false or zero", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Boolean or uint8 array. True or nonzero tuples are included.", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Input Quaternions (<x, y, z>, w)",
                                                          "Quaternions in (<x, y, z>, w) order. Select cell Quats, feature AvgQuats, or another quaternion array.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PhasesArrayPath_Key, "Phases", "Ensemble index for each quaternion tuple. Must have the same tuple count as Input Quaternions.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CAxisReferenceMisalignmentsArrayName_Key, "C-Axis Reference Misalignments",
                                                          "Output angles in degrees, in [0, 90]. Created beside Input Quaternions.", "CAxisReferenceMisalignments"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeCAxisReferenceMisalignmentsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeCAxisReferenceMisalignmentsFilter::clone() const
{
  return std::make_unique<ComputeCAxisReferenceMisalignmentsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeCAxisReferenceMisalignmentsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const ComputeCAxisReferenceMisalignmentsInputValues inputValues = CreateInputValues(filterArgs);
  Result<> validationResult = ComputeCAxisReferenceMisalignments::ValidateInputs(dataStructure, inputValues);
  if(validationResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(validationResult), {})};
  }
  std::vector<std::pair<std::string_view, DataPath>> inputPaths = {
      {"Input Quaternions", inputValues.QuatsArrayPath}, {"Phases", inputValues.PhasesArrayPath}, {"Crystal Structures", inputValues.CrystalStructuresArrayPath}};
  if(inputValues.UseMask)
  {
    inputPaths.emplace_back("Mask", inputValues.MaskArrayPath);
  }
  for(const auto& inputRef : inputPaths)
  {
    if(inputValues.OutputArrayPath == inputRef.second)
    {
      return {MakeErrorResult<OutputActions>(ComputeCAxisReferenceMisalignments::k_OutputPathCollisionError,
                                             fmt::format("The output array path '{}' is the path of the selected {} array. Change the C-Axis Reference Misalignments name '{}' to a "
                                                         "name that no input array uses.",
                                                         inputValues.OutputArrayPath.toString(), inputRef.first, inputValues.OutputArrayPath.getTargetName()))};
    }
  }
  const auto& quatsRef = dataStructure.getDataRefAs<Float32Array>(inputValues.QuatsArrayPath);
  OutputActions actions;
  actions.appendAction(std::make_unique<CreateArrayAction>(DataType::float32, quatsRef.getIDataStoreRef().getTupleShape(), ShapeType{1}, inputValues.OutputArrayPath));
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ComputeCAxisReferenceMisalignmentsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const ComputeCAxisReferenceMisalignmentsInputValues inputValues = CreateInputValues(filterArgs);
  return ComputeCAxisReferenceMisalignments(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
