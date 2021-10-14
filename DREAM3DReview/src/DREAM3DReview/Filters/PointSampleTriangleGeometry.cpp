#include "PointSampleTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string PointSampleTriangleGeometry::name() const
{
  return FilterTraits<PointSampleTriangleGeometry>::name.str();
}

Uuid PointSampleTriangleGeometry::uuid() const
{
  return FilterTraits<PointSampleTriangleGeometry>::uuid;
}

std::string PointSampleTriangleGeometry::humanName() const
{
  return "Point Sample Triangle Geometry";
}

Parameters PointSampleTriangleGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SamplesNumberType_Key, "Source for Number of Samples", "", 0, ChoicesParameter::Choices{"Manual", "Other Geometry"}));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfSamples_Key, "Number of Sample Points", "", 1234356));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleGeometry_Key, "Triangle Geometry to Sample", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_ParentGeometry_Key, "Source Geometry for Number of Sample Points", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_TriangleAreasArrayPath_Key, "Face Areas", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Transfer", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<StringParameter>(k_VertexGeometry_Key, "Vertex Geometry", "", "SomeString"));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SamplesNumberType_Key, k_NumberOfSamples_Key, 0);
  params.linkParameters(k_SamplesNumberType_Key, k_ParentGeometry_Key, 1);
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

IFilter::UniquePointer PointSampleTriangleGeometry::clone() const
{
  return std::make_unique<PointSampleTriangleGeometry>();
}

Result<OutputActions> PointSampleTriangleGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSamplesNumberTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_SamplesNumberType_Key);
  auto pNumberOfSamplesValue = filterArgs.value<int32>(k_NumberOfSamples_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pTriangleGeometryValue = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  auto pParentGeometryValue = filterArgs.value<DataPath>(k_ParentGeometry_Key);
  auto pTriangleAreasArrayPathValue = filterArgs.value<DataPath>(k_TriangleAreasArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pVertexGeometryValue = filterArgs.value<StringParameter::ValueType>(k_VertexGeometry_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<PointSampleTriangleGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> PointSampleTriangleGeometry::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSamplesNumberTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_SamplesNumberType_Key);
  auto pNumberOfSamplesValue = filterArgs.value<int32>(k_NumberOfSamples_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pTriangleGeometryValue = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  auto pParentGeometryValue = filterArgs.value<DataPath>(k_ParentGeometry_Key);
  auto pTriangleAreasArrayPathValue = filterArgs.value<DataPath>(k_TriangleAreasArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pVertexGeometryValue = filterArgs.value<StringParameter::ValueType>(k_VertexGeometry_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
