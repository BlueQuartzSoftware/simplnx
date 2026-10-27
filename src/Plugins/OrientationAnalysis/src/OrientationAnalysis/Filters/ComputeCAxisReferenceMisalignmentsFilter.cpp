#include "ComputeCAxisReferenceMisalignmentsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeCAxisReferenceMisalignments.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

using namespace nx::core;

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
  return {className(), "Processing", "Crystallography", "C-Axis", "Misalignment", "Hexagonal"};
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

  const auto referenceDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ReferenceDir_Key);
  if(auto result = ComputeCAxisReferenceMisalignments::ValidateReferenceDirection(referenceDirection); result.invalid())
  {
    return {ConvertInvalidResult<OutputActions>(std::move(result))};
  }
  const auto quatsPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  const auto phasesPath = filterArgs.value<DataPath>(k_PhasesArrayPath_Key);
  const auto outputPath = quatsPath.replaceName(filterArgs.value<std::string>(k_CAxisReferenceMisalignmentsArrayName_Key));
  std::vector<DataPath> paths = {quatsPath, phasesPath};
  if(filterArgs.value<bool>(k_UseMask_Key))
  {
    paths.push_back(filterArgs.value<DataPath>(k_MaskArrayPath_Key));
  }
  if(auto tupleCheck = dataStructure.validateNumberOfTuples(paths); !tupleCheck)
  {
    return {MakeErrorResult<OutputActions>(
        -98711, fmt::format("Input Quaternions, Phases, and the enabled Mask must have the same number of tuples. Select matching arrays at the same data level.\n{}", tupleCheck.error()))};
  }
  const auto& quats = dataStructure.getDataRefAs<Float32Array>(quatsPath);
  OutputActions actions;
  actions.appendAction(std::make_unique<CreateArrayAction>(DataType::float32, quats.getIDataStoreRef().getTupleShape(), std::vector<usize>{1}, outputPath));
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ComputeCAxisReferenceMisalignmentsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeCAxisReferenceMisalignmentsInputValues inputValues;
  inputValues.ReferenceDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ReferenceDir_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.PhasesArrayPath = filterArgs.value<DataPath>(k_PhasesArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.OutputArrayPath = inputValues.QuatsArrayPath.replaceName(filterArgs.value<std::string>(k_CAxisReferenceMisalignmentsArrayName_Key));
  return ComputeCAxisReferenceMisalignments(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
