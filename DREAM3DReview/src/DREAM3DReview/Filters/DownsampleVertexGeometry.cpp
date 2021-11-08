#include "DownsampleVertexGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
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
  auto pDownsampleTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_DownsampleType_Key);
  auto pDecimationFreqValue = filterArgs.value<int32>(k_DecimationFreq_Key);
  auto pDecimationFractionValue = filterArgs.value<float32>(k_DecimationFraction_Key);
  auto pGridResolutionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_GridResolution_Key);
  auto pVertexAttrMatPathValue = filterArgs.value<DataPath>(k_VertexAttrMatPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<DownsampleVertexGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
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
