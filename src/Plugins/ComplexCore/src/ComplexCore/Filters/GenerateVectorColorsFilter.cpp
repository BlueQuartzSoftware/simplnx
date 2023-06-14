#include "GenerateVectorColorsFilter.hpp"

#include "ComplexCore/Filters/Algorithms/GenerateVectorColors.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateVectorColorsFilter::name() const
{
  return FilterTraits<GenerateVectorColorsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateVectorColorsFilter::className() const
{
  return FilterTraits<GenerateVectorColorsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateVectorColorsFilter::uuid() const
{
  return FilterTraits<GenerateVectorColorsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateVectorColorsFilter::humanName() const
{
  return "Generate Vector Colors";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateVectorColorsFilter::defaultTags() const
{
  return {"Generic", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters GenerateVectorColorsFilter::parameters() const
{
  Parameters params;

  /**
   * Please separate the parameters into groups generally of the following:
   *
   * params.insertSeparator(Parameters::Separator{"Input Parameters"});
   * params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Required Input Feature Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
   *
   * .. or create appropriate separators as needed. The UI in COMPLEX no longer
   * does this for the developer by using catgories as in SIMPL
   */

  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Apply to Good Voxels Only (Bad Voxels Will Be Black)", "", false #error Check default values));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_VectorsArrayPath_Key, "Vector Attribute Array", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellVectorColorsArrayName_Key, "Vector Colors", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateVectorColorsFilter::clone() const
{
  return std::make_unique<GenerateVectorColorsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateVectorColorsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pVectorsArrayPathValue = filterArgs.value<DataPath>(k_VectorsArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCellVectorColorsArrayNameValue = filterArgs.value<DataPath>(k_CellVectorColorsArrayName_Key);



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
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GenerateVectorColorsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{

  GenerateVectorColorsInputValues inputValues;

    inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.VectorsArrayPath = filterArgs.value<DataPath>(k_VectorsArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.CellVectorColorsArrayName = filterArgs.value<DataPath>(k_CellVectorColorsArrayName_Key);


  return GenerateVectorColors(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
