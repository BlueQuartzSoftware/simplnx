#include "EMsoftSO3Sampler.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string EMsoftSO3Sampler::name() const
{
  return FilterTraits<EMsoftSO3Sampler>::name.str();
}

//------------------------------------------------------------------------------
std::string EMsoftSO3Sampler::className() const
{
  return FilterTraits<EMsoftSO3Sampler>::className;
}

//------------------------------------------------------------------------------
Uuid EMsoftSO3Sampler::uuid() const
{
  return FilterTraits<EMsoftSO3Sampler>::uuid;
}

//------------------------------------------------------------------------------
std::string EMsoftSO3Sampler::humanName() const
{
  return "SO3 Angle Sampler";
}

//------------------------------------------------------------------------------
std::vector<std::string> EMsoftSO3Sampler::defaultTags() const
{
  return {"#Sampling", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters EMsoftSO3Sampler::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_sampleModeSelector_Key, "Select the desired SO(3) sampling mode", "", 0,
                                         ChoicesParameter::Choices{"- Rodrigues fundamental zone    ", "- Constant misorientation       ", "- Less than given misorientation"}));
  params.insert(std::make_unique<Int32Parameter>(k_PointGroup_Key, "Point group number (see documentation for list)", "", 1234356));
  params.insert(std::make_unique<BoolParameter>(k_OffsetGrid_Key, "Offset sampling grid from origin?", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_MisOr_Key, "Misorientation angle (degree)", "", 2.3456789));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RefOr_Key, "Reference orientation (Euler, °)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<Float64Parameter>(k_MisOrFull_Key, "Misorientation angle (degree)", "", 2.3456789));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RefOrFull_Key, "Reference orientation (Euler, °)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<Int32Parameter>(k_Numsp_Key, "Number of sampling points along cube semi-axis", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_EulerAnglesArrayName_Key, "Euler Angles", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_sampleModeSelector_Key, k_PointGroup_Key, 0);
  params.linkParameters(k_sampleModeSelector_Key, k_OffsetGrid_Key, 0);
  params.linkParameters(k_sampleModeSelector_Key, k_MisOr_Key, 1);
  params.linkParameters(k_sampleModeSelector_Key, k_RefOr_Key, 1);
  params.linkParameters(k_sampleModeSelector_Key, k_MisOrFull_Key, 2);
  params.linkParameters(k_sampleModeSelector_Key, k_RefOrFull_Key, 2);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer EMsoftSO3Sampler::clone() const
{
  return std::make_unique<EMsoftSO3Sampler>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EMsoftSO3Sampler::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto psampleModeSelectorValue = filterArgs.value<ChoicesParameter::ValueType>(k_sampleModeSelector_Key);
  auto pPointGroupValue = filterArgs.value<int32>(k_PointGroup_Key);
  auto pOffsetGridValue = filterArgs.value<bool>(k_OffsetGrid_Key);
  auto pMisOrValue = filterArgs.value<float64>(k_MisOr_Key);
  auto pRefOrValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RefOr_Key);
  auto pMisOrFullValue = filterArgs.value<float64>(k_MisOrFull_Key);
  auto pRefOrFullValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RefOrFull_Key);
  auto pNumspValue = filterArgs.value<int32>(k_Numsp_Key);
  auto pEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_EulerAnglesArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> EMsoftSO3Sampler::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto psampleModeSelectorValue = filterArgs.value<ChoicesParameter::ValueType>(k_sampleModeSelector_Key);
  auto pPointGroupValue = filterArgs.value<int32>(k_PointGroup_Key);
  auto pOffsetGridValue = filterArgs.value<bool>(k_OffsetGrid_Key);
  auto pMisOrValue = filterArgs.value<float64>(k_MisOr_Key);
  auto pRefOrValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RefOr_Key);
  auto pMisOrFullValue = filterArgs.value<float64>(k_MisOrFull_Key);
  auto pRefOrFullValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RefOrFull_Key);
  auto pNumspValue = filterArgs.value<int32>(k_Numsp_Key);
  auto pEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_EulerAnglesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
