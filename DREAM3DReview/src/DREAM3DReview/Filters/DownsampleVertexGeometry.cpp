#include "DownsampleVertexGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string DownsampleVertexGeometry::name() const
{
  return FilterTraits<DownsampleVertexGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string DownsampleVertexGeometry::className() const
{
  return FilterTraits<DownsampleVertexGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid DownsampleVertexGeometry::uuid() const
{
  return FilterTraits<DownsampleVertexGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string DownsampleVertexGeometry::humanName() const
{
  return "Downsample Vertex Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> DownsampleVertexGeometry::defaultTags() const
{
  return {"#Sampling", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters DownsampleVertexGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(
      k_DownsampleType_Key, "Downsample Type", "", 0, ChoicesParameter::Choices{"Remove Every Nth Point", "Remove a Fixed Random Fraction of Points", "Downsample the Geometry on a Grid"}));
  params.insert(std::make_unique<Int32Parameter>(k_DecimationFreq_Key, "Decimation Frequency", "", 1234356));
  params.insert(std::make_unique<Float32Parameter>(k_DecimationFraction_Key, "Fraction to Remove", "", 1.23345f));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_GridResolution_Key, "Grid Resolution", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_VertexAttrMatPath_Key, "Vertex Attribute Matrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_DownsampleType_Key, k_DecimationFreq_Key, 0);
  params.linkParameters(k_DownsampleType_Key, k_DecimationFraction_Key, 1);
  params.linkParameters(k_DownsampleType_Key, k_GridResolution_Key, 2);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer DownsampleVertexGeometry::clone() const
{
  return std::make_unique<DownsampleVertexGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult DownsampleVertexGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pDownsampleTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_DownsampleType_Key);
  auto pDecimationFreqValue = filterArgs.value<int32>(k_DecimationFreq_Key);
  auto pDecimationFractionValue = filterArgs.value<float32>(k_DecimationFraction_Key);
  auto pGridResolutionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_GridResolution_Key);
  auto pVertexAttrMatPathValue = filterArgs.value<DataPath>(k_VertexAttrMatPath_Key);

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
Result<> DownsampleVertexGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDownsampleTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_DownsampleType_Key);
  auto pDecimationFreqValue = filterArgs.value<int32>(k_DecimationFreq_Key);
  auto pDecimationFractionValue = filterArgs.value<float32>(k_DecimationFraction_Key);
  auto pGridResolutionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_GridResolution_Key);
  auto pVertexAttrMatPathValue = filterArgs.value<DataPath>(k_VertexAttrMatPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
