#include "EMsoftSO3SamplerFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/EMsoftSO3Sampler.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/EnsembleInfoParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "EbsdLib/IO/TSL/AngConstants.h"

using namespace nx::core;

namespace
{

}

namespace nx::core
{
//------------------------------------------------------------------------------
std::string EMsoftSO3SamplerFilter::name() const
{
  return FilterTraits<EMsoftSO3SamplerFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string EMsoftSO3SamplerFilter::className() const
{
  return FilterTraits<EMsoftSO3SamplerFilter>::className;
}

//------------------------------------------------------------------------------
Uuid EMsoftSO3SamplerFilter::uuid() const
{
  return FilterTraits<EMsoftSO3SamplerFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string EMsoftSO3SamplerFilter::humanName() const
{
  return "SO3 Orientation Sampler";
}

//------------------------------------------------------------------------------
std::vector<std::string> EMsoftSO3SamplerFilter::defaultTags() const
{
  return {className(), "Sampling", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters EMsoftSO3SamplerFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Int32Parameter>(k_NumberSamples_Key, "Number of sampling points along cube semi-axis", "The number of sampling points along the cube semi-axis", 101));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SampleModeSelector_Key, "Select the desired SO(3) sampling mode",
                                                                    "This is the mode that is ued to sample the orientation space. [0-2]", orientation_sampling::k_FZModeIndex,
                                                                    orientation_sampling::k_Choices));

  params.insert(std::make_unique<ChoicesParameter>(k_CrystalStructure_Index, "Laue Class", "Laue Class Index. See documentation for list.", 1, EnsembleInfoParameter::k_CrystalStructures));

  params.insert(std::make_unique<BoolParameter>(k_OffsetGrid_Key, "Offset sampling grid from origin?", "Offset the cubo-choric grid from the origin by half a grid unit.", false));
  params.insert(std::make_unique<Float64Parameter>(k_Mode1Misorientation_Key, "Misorientation angle (degree)", "Reference Misorientation in Degrees", 1.0));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Mode1EulerAngle_Key, "Reference orientation (Euler, °)", "Reference Orientation as a Bunge Euler Triplet in Degrees",
                                                         std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"phi-1", "PHI", "phi-2"}));
  params.insert(std::make_unique<Float64Parameter>(k_Mode2Misorientation_Key, "Misorientation angle (degree)", "Reference Misorientation in Degrees", 1.0));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Mode2EulerAngle_Key, "Reference orientation (Euler, °)", "Reference Orientation as a Bunge Euler Triplet in Degrees",
                                                         std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"phi-1", "PHI", "phi-2"}));

  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_EulerAnglesArrayPath_Key, "Euler Angles", "Output array to hold the generated Euler Angles", DataPath({"Euler Angles"})));

  params.insert(std::make_unique<DataGroupCreationParameter>(k_CellAttributeMatrixName_Key, "Output Attribute Matrix", "The name of the cell data attribute matrix for the created Image Geometry",
                                                             DataGroupCreationParameter::ValueType({"Sampling Data"})));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_EnsembleAttributeMatrixPath_Key, "Ensemble Attribute Matrix", "The Attribute Matrix where the phase information is stored.",
                                                             DataGroupCreationParameter::ValueType({"Ensemble Attribute Matrix"})));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SampleModeSelector_Key, k_CrystalStructure_Index, static_cast<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_SampleModeSelector_Key, k_OffsetGrid_Key, static_cast<ChoicesParameter::ValueType>(0));

  params.linkParameters(k_SampleModeSelector_Key, k_Mode1Misorientation_Key, static_cast<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_SampleModeSelector_Key, k_Mode1EulerAngle_Key, static_cast<ChoicesParameter::ValueType>(1));

  params.linkParameters(k_SampleModeSelector_Key, k_Mode2Misorientation_Key, static_cast<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_SampleModeSelector_Key, k_Mode2EulerAngle_Key, static_cast<ChoicesParameter::ValueType>(2));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType EMsoftSO3SamplerFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer EMsoftSO3SamplerFilter::clone() const
{
  return std::make_unique<EMsoftSO3SamplerFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EMsoftSO3SamplerFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto psampleModeSelectorValue = filterArgs.value<ChoicesParameter::ValueType>(k_SampleModeSelector_Key);
  auto pCrystalStructureIndex = filterArgs.value<ChoicesParameter::ValueType>(k_CrystalStructure_Index);
  auto pOffsetGridValue = filterArgs.value<bool>(k_OffsetGrid_Key);
  auto pMisOrValue = filterArgs.value<float64>(k_Mode1Misorientation_Key);
  auto pRefOrValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Mode1EulerAngle_Key);
  auto pMisOrFullValue = filterArgs.value<float64>(k_Mode2Misorientation_Key);
  auto pRefOrFullValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Mode2EulerAngle_Key);
  auto pNumspValue = filterArgs.value<int32>(k_NumberSamples_Key);
  auto pEulerAnglesArrayPath = filterArgs.value<DataPath>(k_EulerAnglesArrayPath_Key);
  // auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto ensembleAttributeMatrixPath = filterArgs.value<DataPath>(k_EnsembleAttributeMatrixPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // check on the point group index; must be between 1 and 32.
  if(psampleModeSelectorValue == 0)
  {
    if(pCrystalStructureIndex > 10)
    {
      return MakePreflightErrorResult(-78760, "Crystal Structure Index must be in range [0,10]. See documentation for table of values");
    }
  }

  // make sure that the misorientation angle lies in [0,90], and the Euler angles in [0,360], [0,180], [0,360]
  if(psampleModeSelectorValue == 1)
  {
    if((pMisOrValue < 0.0) || (pMisOrValue > 90.0))
    {
      return MakePreflightErrorResult(-70002, "Misorientation angle must fall in interval [0,90]");
    }
    if((pRefOrValue[0] < 0.0) || (pRefOrValue[0] > 360.0f) || (pRefOrValue[1] < 0.0f) || (pRefOrValue[1] > 180.0f) || (pRefOrValue[2] < 0.0f) || (pRefOrValue[2] > 360.0f))
    {
      return MakePreflightErrorResult(-70003, "Euler angles must be positive and less than [360°,180°,360°]");
    }
  }
  if(psampleModeSelectorValue == 2)
  {
    if((pMisOrFullValue < 0.0) || (pMisOrFullValue > 90.0))
    {
      return MakePreflightErrorResult(-70004, "Misorientation angle must fall in interval [0,90]");
    }
    if((pRefOrFullValue[0] < 0.0f) || (pRefOrFullValue[0] > 360.0f) || (pRefOrFullValue[1] < 0.0f) || (pRefOrFullValue[1] > 180.0f) || (pRefOrFullValue[2] < 0.0f) || (pRefOrFullValue[2] > 360.0f))
    {
      return MakePreflightErrorResult(-70005, "Euler angles must be positive and less than [360°,180°,360°]");
    }
  }

  // check on the number of sampling intervals (>1)
  if(pNumspValue < 1)
  {
    return MakePreflightErrorResult(-70002, "Number of sampling intervals must be at least 1 ");
  }

  // create the destination array for the calculated results
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{1}, std::vector<usize>{3}, pEulerAnglesArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Create the Ensemble AttributeMatrix
  nx::core::ShapeType tupleDims = {2}; // Always create 1 extra slot for the phases.
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(ensembleAttributeMatrixPath, tupleDims);
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }

  // Create the Crystal Structures Array
  nx::core::ShapeType cDims = {1};
  {
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::CrystalStructures);
    auto action = std::make_unique<CreateArrayAction>(nx::core::DataType::uint32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the Material Names Array
  {
    DataPath dataArrayPath = ensembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::MaterialName);
    auto action = std::make_unique<CreateStringArrayAction>(tupleDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> EMsoftSO3SamplerFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{

  EMsoftSO3SamplerInputValues inputValues;

  inputValues.sampleModeSelector = filterArgs.value<ChoicesParameter::ValueType>(k_SampleModeSelector_Key);
  inputValues.CrystalStructureIndex = filterArgs.value<ChoicesParameter::ValueType>(k_CrystalStructure_Index);
  inputValues.OffsetGrid = filterArgs.value<bool>(k_OffsetGrid_Key);
  inputValues.MisOr = filterArgs.value<float64>(k_Mode1Misorientation_Key);
  inputValues.RefOr = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Mode1EulerAngle_Key);
  inputValues.MisOrFull = filterArgs.value<float64>(k_Mode2Misorientation_Key);
  inputValues.RefOrFull = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Mode2EulerAngle_Key);
  inputValues.Numsp = filterArgs.value<int32>(k_NumberSamples_Key);
  inputValues.EulerAnglesArrayName = filterArgs.value<DataPath>(k_EulerAnglesArrayPath_Key);

  inputValues.EnsembleAttrMatrixPath = filterArgs.value<DataPath>(k_EnsembleAttributeMatrixPath_Key);

  return EMsoftSO3Sampler(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{

} // namespace SIMPL
} // namespace

//------------------------------------------------------------------------------
Result<Arguments> EMsoftSO3SamplerFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = EMsoftSO3SamplerFilter().getDefaultArguments();

  std::vector<Result<>> results;

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
